/***************************************************************************

Bomb Jack memory map (preliminary)
MAIN BOARD:

0000-1fff ROM 0
2000-3fff ROM 1
4000-5fff ROM 2
6000-7fff ROM 3
8000-83ff RAM 0
8400-87ff RAM 1
8800-8bff RAM 2
8c00-8fff RAM 3
9000-93ff Video RAM (RAM 4)
9400-97ff Color RAM (RAM 4)
c000-dfff ROM 4

memory mapped ports:
read:
b000      IN0
b001      IN1
b002      IN2
b003      watchdog reset?
b004      DSW1
b005      DSW2

write:
9820-987f sprites
9a00      ? number of small sprites for video controller
9c00-9cff palette
9e00      background image selector
b000      interrupt enable
b800      command to soundboard & trigger NMI on sound board



SOUND BOARD:
0x0000 0x1fff ROM
0x2000 0x4400 RAM

memory mapped ports:
read:
0x6000 command from soundboard
write :
none

IO ports:
write:
0x00 AY#1 control
0x01 AY#1 write
0x10 AY#2 control
0x11 AY#2 write
0x80 AY#3 control
0x81 AY#3 write

interrupts:
NMI triggered by the commands sent by MAIN BOARD (?)
NMI interrupts for music timing

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



extern unsigned char *bombjack_paletteram;
void bombjack_paletteram_w(int offset,int data);
void bombjack_background_w(int offset,int data);
void bombjack_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void bombjack_vh_screenrefresh(struct osd_bitmap *bitmap);

int bombjack_sh_intflag_r(int offset);
int bombjack_sh_start(void);
int bombjack_sh_interrupt(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0xb003, 0xb003, MRA_NOP },
	{ 0xb000, 0xb000, input_port_0_r },	/* player 1 input */
	{ 0xb001, 0xb001, input_port_1_r },	/* player 2 input */
	{ 0xb002, 0xb002, input_port_2_r },	/* coin */
	{ 0xb004, 0xb004, input_port_3_r },	/* DSW1 */
	{ 0xb005, 0xb005, input_port_4_r },	/* DSW2 */
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x97ff, MRA_RAM },	/* including video and color RAM */
	{ 0xc000, 0xdfff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x9820, 0x987f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x9c00, 0x9cff, bombjack_paletteram_w, &bombjack_paletteram },
	{ 0xb000, 0xb000, interrupt_enable_w },
	{ 0x9e00, 0x9e00, bombjack_background_w },
	{ 0xb800, 0xb800, sound_command_w },
	{ 0x9a00, 0x9a00, MWA_NOP },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x93ff, videoram_w, &videoram, &videoram_size },
	{ 0x9400, 0x97ff, colorram_w, &colorram },
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress bombjack_sound_readmem[] =
{
#if 0
	{ 0x4390, 0x4390, bombjack_sh_intflag_r },	/* kludge to speed up the emulation */
#endif
	{ 0x2000, 0x5fff, MRA_RAM },
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x6000, 0x6000, sound_command_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress bombjack_sound_writemem[] =
{
	{ 0x2000, 0x5fff, MWA_RAM },
	{ 0x0000, 0x1fff, MWA_ROM },
	{ -1 }  /* end of table */
};


static struct IOWritePort bombjack_sound_writeport[] =
{
	{ 0x00, 0x00, AY8910_control_port_0_w },
	{ 0x01, 0x01, AY8910_write_port_0_w },
	{ 0x10, 0x10, AY8910_control_port_1_w },
	{ 0x11, 0x11, AY8910_write_port_1_w },
	{ 0x80, 0x80, AY8910_control_port_2_w },
	{ 0x81, 0x81, AY8910_write_port_2_w },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0x00,
		{ OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_UP, OSD_KEY_DOWN,
				OSD_KEY_CONTROL, 0, 0, 0 },
		{ OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_UP, OSD_JOY_DOWN,
				OSD_JOY_FIRE, 0, 0, 0 }
	},
	{	/* IN1 */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0x00,
		{ OSD_KEY_4, OSD_KEY_3, OSD_KEY_1, OSD_KEY_2, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0xc0,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW2 */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }  /* end of table */
};

static struct TrakPort trak_ports[] =
{
        { -1 }
};


static struct KEYSet keys[] =
{
        { 0, 2, "MOVE UP" },
        { 0, 1, "MOVE LEFT"  },
        { 0, 0, "MOVE RIGHT" },
        { 0, 3, "MOVE DOWN" },
        { 0, 4, "JUMP" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 3, 0x30, "LIVES", { "3", "4", "5", "2" } },
	{ 4, 0x18, "DIFFICULTY 1", { "EASY", "MEDIUM", "HARD", "HARDEST" } },
	{ 4, 0x60, "DIFFICULTY 2", { "MEDIUM", "EASY", "HARD", "HARDEST" } },
		/* not a mistake, MEDIUM and EASY are swapped */
	{ 4, 0x80, "SPECIAL", { "EASY", "HARD" } },
	{ 3, 0x80, "DEMO SOUNDS", { "OFF", "ON" } },
	{ 4, 0x07, "INITIAL HIGH SCORE", { "10000", "100000", "30000", "50000", "100000", "50000", "100000", "50000" } },
	{ -1 }
};



static struct GfxLayout charlayout1 =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	3,	/* 3 bits per pixel */
	{ 0, 512*8*8, 2*512*8*8 },	/* the bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },	/* pretty straightforward layout */
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout charlayout2 =
{
	16,16,	/* 16*16 characters */
	256,	/* 256 characters */
	3,	/* 3 bits per pixel */
	{ 0, 1024*8*8, 2*1024*8*8 },	/* the bitplanes are separated */
	{ 23*8, 22*8, 21*8, 20*8, 19*8, 18*8, 17*8, 16*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,	/* pretty straightforward layout */
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	32*8	/* every character takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout1 =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	3,	/* 3 bits per pixel */
	{ 0, 1024*8*8, 2*1024*8*8 },	/* the bitplanes are separated */
	{ 23*8, 22*8, 21*8, 20*8, 19*8, 18*8, 17*8, 16*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,	/* pretty straightforward layout */
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout2 =
{
	32,32,	/* 32*32 sprites */
	32,	/* 32 sprites */
	3,	/* 3 bits per pixel */
	{ 0, 1024*8*8, 2*1024*8*8 },	/* the bitplanes are separated */
	{ 87*8, 86*8, 85*8, 84*8, 83*8, 82*8, 81*8, 80*8,
			71*8, 70*8, 69*8, 68*8, 67*8, 66*8, 65*8, 64*8,
			23*8, 22*8, 21*8, 20*8, 19*8, 18*8, 17*8, 16*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,	/* pretty straightforward layout */
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7,
			32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7,
			40*8+0, 40*8+1, 40*8+2, 40*8+3, 40*8+4, 40*8+5, 40*8+6, 40*8+7 },
	128*8	/* every sprite takes 128 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout1,      0, 16 },	/* characters */
	{ 1, 0x3000, &charlayout2,      0, 16 },	/* background tiles */
	{ 1, 0x9000, &spritelayout1,    0, 16 },	/* normal sprites */
	{ 1, 0xa000, &spritelayout2,    0, 16 },	/* large sprites */
	{ -1 } /* end of array */
};


static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz */
			0,
			readmem,writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3072000,	/* 3.072 Mhz????? */
			3,	/* memory region #3 */
			bombjack_sound_readmem,bombjack_sound_writemem,0,bombjack_sound_writeport,
			bombjack_sh_interrupt,10
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0, 32*8-1 },
	gfxdecodeinfo,
	128, 16*8,
	bombjack_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	bombjack_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	bombjack_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( bombjack_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "09_j01b.bin", 0x0000, 0x2000, 0xb263429f )
	ROM_LOAD( "10_l01b.bin", 0x2000, 0x2000, 0x8dd024fa )
	ROM_LOAD( "11_m01b.bin", 0x4000, 0x2000, 0x4464856c )
	ROM_LOAD( "12_n01b.bin", 0x6000, 0x2000, 0x0018672a )
	ROM_LOAD( "13_r01b.bin", 0xc000, 0x2000, 0x3500e950 )

	ROM_REGION(0xf000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "03_e08t.bin", 0x0000, 0x1000, 0xd79cfb42 )	/* chars */
	ROM_LOAD( "04_h08t.bin", 0x1000, 0x1000, 0x16a78377 )
	ROM_LOAD( "05_k08t.bin", 0x2000, 0x1000, 0x8aa5777d )
	ROM_LOAD( "06_l08t.bin", 0x3000, 0x2000, 0x4d9c13d2 )	/* background tiles */
	ROM_LOAD( "07_n08t.bin", 0x5000, 0x2000, 0x98f83bce )
	ROM_LOAD( "08_r08t.bin", 0x7000, 0x2000, 0x428b73d9 )
	ROM_LOAD( "16_m07b.bin", 0x9000, 0x2000, 0x5fea2266 )	/* sprites */
	ROM_LOAD( "15_l07b.bin", 0xb000, 0x2000, 0x2f490ac1 )
	ROM_LOAD( "14_j07b.bin", 0xd000, 0x2000, 0x9674c1bc )

	ROM_REGION(0x1000)	/* background graphics */
	ROM_LOAD( "02_p04t.bin", 0x0000, 0x1000, 0x1af71029 )

	ROM_REGION(0x10000)	/* 64k for sound board */
	ROM_LOAD( "01_h03t.bin", 0x0000, 0x2000, 0xe89c2656 )
ROM_END



static int hiload(const char *name)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x8100],&RAM[0x8124],4) == 0 &&
			memcmp(&RAM[0x8100],&RAM[0x80e2],4) == 0 &&	/* high score */
			(memcmp(&RAM[0x8100],"\x00\x00\x01\x00",4) == 0 ||
			memcmp(&RAM[0x8100],"\x00\x00\x03\x00",4) == 0 ||
			memcmp(&RAM[0x8100],"\x00\x00\x05\x00",4) == 0 ||
			memcmp(&RAM[0x8100],"\x00\x00\x10\x00",4) == 0))
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			char buf[10];
			int hi;


			fread(&RAM[0x8100],1,15*10,f);
			RAM[0x80e2] = RAM[0x8100];
			RAM[0x80e3] = RAM[0x8101];
			RAM[0x80e4] = RAM[0x8102];
			RAM[0x80e5] = RAM[0x8103];
			/* also copy the high score to the screen, otherwise it won't be */
			/* updated until a new game is started */
			hi = (RAM[0x8100] & 0x0f) +
					(RAM[0x8100] >> 4) * 10 +
					(RAM[0x8101] & 0x0f) * 100 +
					(RAM[0x8101] >> 4) * 1000 +
					(RAM[0x8102] & 0x0f) * 10000 +
					(RAM[0x8102] >> 4) * 100000 +
					(RAM[0x8103] & 0x0f) * 1000000 +
					(RAM[0x8103] >> 4) * 10000000;
			sprintf(buf,"%8d",hi);
			videoram_w(0x013f,buf[0]);
			videoram_w(0x011f,buf[1]);
			videoram_w(0x00ff,buf[2]);
			videoram_w(0x00df,buf[3]);
			videoram_w(0x00bf,buf[4]);
			videoram_w(0x009f,buf[5]);
			videoram_w(0x007f,buf[6]);
			videoram_w(0x005f,buf[7]);
			fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(const char *name)
{
	FILE *f;
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	if ((f = fopen(name,"wb")) != 0)
	{
		fwrite(&RAM[0x8100],1,15*10,f);
		fclose(f);
	}
}



struct GameDriver bombjack_driver =
{
	"Bomb Jack",
	"bombjack",
	"BRAD THOMAS\nJAKOB FRENDSEN\nCONNY MELIN\nMIRKO BUFFONI\nNICOLA SALMORIA\nJAREK BURCZYNSKI",
	&machine_driver,

	bombjack_rom,
	0, 0,
	0,

	input_ports, 0, trak_ports, dsw, keys,

	0,0,0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
