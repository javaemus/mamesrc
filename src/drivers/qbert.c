/***************************************************************************

Q*bert's driver : dedicated to Jeff Lee, Warren Davis & David Thiel

****************************************************************************

Q*bert machine's memory map (from my understanding of the schematics... FF )

Main processor (8088 minimum mode)  memory map.
0000-0fff RAM
1000-1fff RAM
2000-2fff RAM
3000-37ff sprite programmation (64 sprites)
3800-3fff background ram (32x30 chars)
4000-4fff background object ram ?
5000-57ff palette ram (palette of 16 colors)
5800-5fff i/o ports (see below)
6000-7fff (empty rom slot)
8000-9fff (empty rom slot)
a000-ffff ROM (qbert's prog)

memory mapped ports:

read:
5800    Dip switch
5801    Inputs 10-17
5802    trackball input (optional)
5803    trackball input (optional)
5804    Inputs 40-47

write:
5800    watchdog timer clear
5801    trackball output (optional)
5802    Outputs 20-27
5803    Flipflop outputs:
		b7: F/B priority
		b6: horiz. flipflop
		b5: vert. flipflop
		b4: Output 33
		b3: coin meter
		b2: knocker
		b1: coin 1
		b0: coin lockout
5804    Outputs 40-47

interrupts:
INTR not connected
NMI connected to vertical blank

Sound processor (6502) memory map:
0000-0fff RIOT (6532)
1000-1fff amplitude DAC
2000-2fff SC01 voice chip
3000-3fff voice clock DAC
4000-4fff socket expansion
5000-5fff socket expansion
6000-6fff socket expansion
7000-7fff PROM
(repeated in 8000-ffff, A15 only used in socket expansion)

Use of I/Os on the RIOT:
both ports A and B are programmed as inputs, A is connected to the main
motherboard, and B has SW1 (test) connected on bit 6.

interrupts:
INTR is connected to the RIOT, so an INTR can be generated by a variety
of sources, e.g active edge detection on PA7, or timer countdown.
It seems that all gottlieb games program the interrupt conditions so that
a positive active edge on PA7 triggers an interrupt, so the
main board ensures a command is correctly received by sending nul (0)
commands between two commands. Also, the timer interrupt is enabled but
doesn't seem to serve any purpose...(?)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

int qbert_vh_start(void);
void gottlieb_vh_init_color_palette(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void gottlieb_sh_w(int offset, int data);
void gottlieb_output(int offset, int data);
int qbert_IN1_r(int offset);
extern unsigned char *gottlieb_paletteram;
void gottlieb_paletteram_w(int offset,int data);
void gottlieb_vh_screenrefresh(struct osd_bitmap *bitmap);

int gottlieb_sh_start(void);
void gottlieb_sh_stop(void);
void gottlieb_sh_update(void);
int gottlieb_sh_interrupt(void);
int riot_ram_r(int offset);
int gottlieb_riot_r(int offset);
int gottlieb_sound_expansion_socket_r(int offset);
void riot_ram_w(int offset, int data);
void gottlieb_riot_w(int offset, int data);
void gottlieb_amplitude_DAC_w(int offset, int data);
void gottlieb_speech_w(int offset, int data);
void gottlieb_speech_clock_DAC_w(int offset, int data);
void gottlieb_sound_expansion_socket_w(int offset, int data);


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x57ff, MRA_RAM },
	{ 0x5800, 0x5800, input_port_0_r },     /* DSW */
	{ 0x5801, 0x5801, qbert_IN1_r },     /* buttons */
	{ 0x5802, 0x5802, input_port_2_r },     /* trackball: not used */
	{ 0x5803, 0x5803, input_port_3_r },     /* trackball: not used */
	{ 0x5804, 0x5804, input_port_4_r },     /* joystick */
	{ 0xA000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x2fff, MWA_RAM },
	{ 0x3000, 0x30ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x3800, 0x3bff, videoram_w, &videoram, &videoram_size },
	{ 0x4000, 0x4fff, MWA_RAM }, /* bg object ram... ? not used ? */
	{ 0x5000, 0x501f, gottlieb_paletteram_w, &gottlieb_paletteram },
	{ 0x5800, 0x5800, MWA_RAM },    /* watchdog timer clear */
	{ 0x5801, 0x5801, MWA_RAM },    /* trackball: not used */
	{ 0x5802, 0x5802, gottlieb_sh_w }, /* sound/speech command */
	{ 0x5803, 0x5803, gottlieb_output },       /* OUT1 */
	{ 0x5804, 0x5804, MWA_RAM },    /* OUT2 */
	{ 0xa000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

struct MemoryReadAddress gottlieb_sound_readmem[] =
{
	{ 0x0000, 0x01ff, riot_ram_r },
	{ 0x0200, 0x03ff, gottlieb_riot_r },
	{ 0x4000, 0x6fff, gottlieb_sound_expansion_socket_r },
	{ 0x7000, 0x7fff, MRA_ROM },
			 /* A15 not decoded except in socket expansion */
	{ 0x8000, 0x81ff, riot_ram_r },
	{ 0x8200, 0x83ff, gottlieb_riot_r },
	{ 0xc000, 0xefff, gottlieb_sound_expansion_socket_r },
	{ 0xf000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

struct MemoryWriteAddress gottlieb_sound_writemem[] =
{
	{ 0x0000, 0x01ff, riot_ram_w },
	{ 0x0200, 0x03ff, gottlieb_riot_w },
	{ 0x1000, 0x1000, gottlieb_amplitude_DAC_w },
	{ 0x2000, 0x2000, gottlieb_speech_w },
	{ 0x3000, 0x3000, gottlieb_speech_clock_DAC_w },
	{ 0x4000, 0x6fff, gottlieb_sound_expansion_socket_w },
	{ 0x7000, 0x7fff, MWA_ROM },
			 /* A15 not decoded except in socket expansion */
	{ 0x8000, 0x81ff, riot_ram_w },
	{ 0x8200, 0x83ff, gottlieb_riot_w },
	{ 0x9000, 0x9000, gottlieb_amplitude_DAC_w },
	{ 0xa000, 0xa000, gottlieb_speech_w },
	{ 0xb000, 0xb000, gottlieb_speech_clock_DAC_w },
	{ 0xc000, 0xefff, gottlieb_sound_expansion_socket_w },
	{ 0xf000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



static struct InputPort input_ports[] =
{
	{       /* DSW */
		0x0,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* buttons */
		0x40,   /* test mode off */
		{ OSD_KEY_1, OSD_KEY_2, OSD_KEY_3, 0 /* coin 2 */,
				0, 0, 0, OSD_KEY_F2 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* trackball: not used */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* trackball: not used */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* 2 joysticks (cocktail mode) mapped to one */
		0x00,
		{ OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_UP, OSD_KEY_DOWN,
			OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_UP, OSD_KEY_DOWN },
		{ OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_UP, OSD_JOY_DOWN,
			OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_UP, OSD_JOY_DOWN },
	},
	{ -1 }  /* end of table */
};

static struct TrakPort trak_ports[] =
{
	{ -1 }
};


static struct KEYSet keys[] =
{
	{ 4, 0, "MOVE DOWN RIGHT" },
	{ 4, 1, "MOVE UP LEFT"  },
	{ 4, 2, "MOVE UP RIGHT" },
	{ 4, 3, "MOVE DOWN LEFT" },
	{ -1 }
};


static struct DSW dsw[] =
{
	{ 0, 0x08, "AUTO ROUND ADVANCE", { "OFF","ON" } },
	{ 0, 0x01, "ATTRACT MODE SOUND", { "ON", "OFF" } },
	{ 0, 0x10, "FREE PLAY", { "OFF" , "ON" } },
	{ 0, 0x04, "", { "UPRIGHT", "COCKTAIL" } },
	{ 0, 0x02, "KICKER", { "OFF", "ON" } },
/* the following switch must be connected to the IP16 line */
/*      { 1, 0x40, "TEST MODE", {"ON", "OFF"} },*/
	{ -1 }
};


static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	256,    /* 256 characters */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8    /* every char takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	256,    /* 256 sprites */
	4,      /* 4 bits per pixel */
	{ 0, 0x2000*8, 0x4000*8, 0x6000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	32*8    /* every sprite takes 32 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,   0, 1 },
	{ 1, 0x2000, &spritelayout, 0, 1 },
	{ -1 } /* end of array */
};



static const struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_I86,
			5000000,        /* 5 Mhz */
			0,
			readmem,writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU ,
			3579545/4,
			2,             /* memory region #2 */
			gottlieb_sound_readmem,gottlieb_sound_writemem,0,0,
			gottlieb_sh_interrupt,1
		}
	},
	60,     /* frames / second */
	0,      /* init machine */

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	1+16, 16,
	gottlieb_vh_init_color_palette,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,      /* init vh */
	qbert_vh_start,
	generic_vh_stop,
	gottlieb_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	gottlieb_sh_start,
	gottlieb_sh_stop,
	gottlieb_sh_update
};



ROM_START( qbert_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "qb-rom2.bin", 0xa000, 0x2000, 0xd1c1dad7 )
	ROM_LOAD( "qb-rom1.bin", 0xc000, 0x2000, 0xdc2bbad9 )
	ROM_LOAD( "qb-rom0.bin", 0xe000, 0x2000, 0xc23a8cfe )

	ROM_REGION(0xA000)      /* temporary space for graphics */
	ROM_LOAD( "qb-bg0.bin", 0x0000, 0x1000, 0x035735a1 )
	ROM_LOAD( "qb-bg1.bin", 0x1000, 0x1000, 0xaac748c5 )
	ROM_LOAD( "qb-fg3.bin", 0x2000, 0x2000, 0x54bd5daf )       /* sprites */
	ROM_LOAD( "qb-fg2.bin", 0x4000, 0x2000, 0x200a62ae )       /* sprites */
	ROM_LOAD( "qb-fg1.bin", 0x6000, 0x2000, 0x7a17df07 )       /* sprites */
	ROM_LOAD( "qb-fg0.bin", 0x8000, 0x2000, 0x0ca72f4f )       /* sprites */

	ROM_REGION(0x10000)      /* 64k for sound cpu */
	ROM_LOAD( "qb-snd1.bin", 0xf000, 0x800, 0x469952eb )
	ROM_RELOAD(0x7000, 0x800) /* A15 is not decoded */
	ROM_LOAD( "qb-snd2.bin", 0xf800, 0x800, 0x200e1d22 )
	ROM_RELOAD(0x7800, 0x800) /* A15 is not decoded */
ROM_END

ROM_START( qbertjp_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "qb-rom2.bin", 0xa000, 0x2000, 0x22b59259 )
	ROM_LOAD( "qb-rom1.bin", 0xc000, 0x2000, 0xa9ffed43 )
	ROM_LOAD( "qb-rom0.bin", 0xe000, 0x2000, 0xf20e301e )

	ROM_REGION(0xA000)      /* temporary space for graphics */
	ROM_LOAD( "qb-bg0.bin", 0x0000, 0x1000, 0x035735a1 )
	ROM_LOAD( "qb-bg1.bin", 0x1000, 0x1000, 0xaac748c5 )
	ROM_LOAD( "qb-fg3.bin", 0x2000, 0x2000, 0x54bd5daf )       /* sprites */
	ROM_LOAD( "qb-fg2.bin", 0x4000, 0x2000, 0x200a62ae )       /* sprites */
	ROM_LOAD( "qb-fg1.bin", 0x6000, 0x2000, 0x7a17df07 )       /* sprites */
	ROM_LOAD( "qb-fg0.bin", 0x8000, 0x2000, 0x0ca72f4f )       /* sprites */

	ROM_REGION(0x10000)      /* 64k for sound cpu */
	ROM_LOAD( "qb-sq1.bin", 0xf000, 0x1000, 0x66a74fc9 )
	ROM_RELOAD(0x7000, 0x1000) /* A15 is not decoded */
ROM_END



const char *gottlieb_sample_names[] =
{
	"FX_00.SAM",
	"FX_01.SAM",
	"FX_02.SAM",
	"FX_03.SAM",
	"FX_04.SAM",
	"FX_05.SAM",
	"FX_06.SAM",
	"FX_07.SAM",
	"FX_08.SAM",
	"FX_09.SAM",
	"FX_10.SAM",
	"FX_11.SAM",
	"FX_12.SAM",
	"FX_13.SAM",
	"FX_14.SAM",
	"FX_15.SAM",
	"FX_16.SAM",
	"FX_17.SAM",
	"FX_18.SAM",
	"FX_19.SAM",
	"FX_20.SAM",
	"FX_21.SAM",
	"FX_22.SAM",
	"FX_23.SAM",
	"FX_24.SAM",
	"FX_25.SAM",
	"FX_26.SAM",
	"FX_27.SAM",
	"FX_28.SAM",
	"FX_29.SAM",
	"FX_30.SAM",
	"FX_31.SAM",
	"FX_32.SAM",
	"FX_33.SAM",
	"FX_34.SAM",
	"FX_35.SAM",
	"FX_36.SAM",
	"FX_37.SAM",
	"FX_38.SAM",
	"FX_39.SAM",
	"FX_40.SAM",
	"FX_41.SAM",
	0       /* end of array */
};



static int hiload(const char *name)
{
	FILE *f=fopen(name,"rb");
	unsigned char *RAM=Machine->memory_region[0];

	/* no need to wait for anything: Q*bert doesn't touch the tables
	if the checksum is correct */
	if (f) {
		fread(RAM+0xA00,1,2,f); /* hiscore table checksum */
		fread(RAM+0xA02,23,10,f); /* 23 hiscore ascending entries: name (3 chars) + score (7 figures) */
		fread(RAM+0xBB0,12,1,f); /* operator parameters : coins/credits, lives, extra-lives points */
		fclose(f);
	}
	return 1;
}

static void hisave(const char *name)
{
	FILE *f=fopen(name,"wb");
	unsigned char *RAM=Machine->memory_region[0];

	if (f) {
	/* not saving distributions tables : does anyone really want them ? */
		fwrite(RAM+0xA00,1,2,f); /* hiscore table checksum */
		fwrite(RAM+0xA02,23,10,f); /* 23 hiscore ascending entries: name (3 chars) + score (7 figures) */
		fwrite(RAM+0xBB0,12,1,f); /* operator parameters : coins/credits, lives, extra-lives points */
		fclose(f);
	}
}

static int hiload_jp(const char *name)
{
	FILE *f=fopen(name,"rb");
	unsigned char *RAM=Machine->memory_region[0];

	if (f) {
		fread(RAM+0xA00,1,2,f); /* hiscore table checksum */
		fread(RAM+0xA02,23,10,f); /* 23 hiscore ascending entries: name (3 chars) + score (7 figures) */
		fread(RAM+0xC0C,12,1,f); /* operator parameters : coins/credits, lives, extra-lives points */
		fclose(f);
	}
	return 1;
}

static void hisave_jp(const char *name)
{
	FILE *f=fopen(name,"wb");
	unsigned char *RAM=Machine->memory_region[0];

	if (f) {
		fwrite(RAM+0xA00,1,2,f); /* hiscore table checksum */
		fwrite(RAM+0xA02,23,10,f); /* 23 hiscore ascending entries: name (3 chars) + score (7 figures) */
		fwrite(RAM+0xC0C,12,1,f); /* operator parameters : coins/credits, lives, extra-lives points */
		fclose(f);
	}
}

struct GameDriver qbert_driver =
{
	"Q*Bert (US version)",
	"qbert",
	"FABRICE FRANCES\n\nDEDICATED TO:\nJEFF LEE\nWARREN DAVIES\nDAVID THIEL",
	&machine_driver,

	qbert_rom,
	0, 0,   /* rom decode and opcode decode functions */
	0,

	input_ports, 0, trak_ports, dsw, keys,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	hiload,hisave     /* hi-score load and save */
};

struct GameDriver qbertjp_driver =
{
	"Q*Bert (Japanese version)",
	"qbertjp",
	"FABRICE FRANCES\n\nDEDICATED TO:\nJEFF LEE\nWARREN DAVIES\nDAVID THIEL",
	&machine_driver,

	qbertjp_rom,
	0, 0,   /* rom decode and opcode decode functions */
	0,

	input_ports, 0, trak_ports, dsw, keys,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	hiload_jp,hisave_jp     /* hi-score load and save */
};