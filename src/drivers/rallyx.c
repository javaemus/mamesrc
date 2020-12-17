/***************************************************************************

Rally X memory map (preliminary)

0000-3fff ROM
8000-83ff Radar video RAM + other
8400-87ff video RAM
8800-8bff Radar color RAM + other
8c00-8fff color RAM
9800-9fff RAM

memory mapped ports:

read:
a000      IN0
a080      IN1
a100      DSW1

*
 * IN0 (all bits are inverted)
 * bit 7 : ?
 * bit 6 : START 1
 * bit 5 : UP player 1
 * bit 4 : DOWN player 1
 * bit 3 : RIGHT player 1
 * bit 2 : LEFT player 1
 * bit 1 : SMOKE player 1
 * bit 0 : CREDIT
 *
*
 * IN1 (all bits are inverted)
 * bit 7 : ?
 * bit 6 : START 2
 * bit 5 : UP player 2 (TABLE only)
 * bit 4 : DOWN player 2 (TABLE only)
 * bit 3 : RIGHT player 2 (TABLE only)
 * bit 2 : LEFT player 2 (TABLE only)
 * bit 1 : SMOKE player 2 (TABLE only)
 * bit 0 : COCKTAIL or UPRIGHT cabinet 1 = UPRIGHT
 *
*
 * DSW1 (all bits are inverted)
 * bit 7 :\ 00 = free play      01 = 2 coins 1 play
 * bit 6 :/ 10 = 1 coin 2 play  11 = 1 coin 1 play
 * bit 5 :\ xxx = cars,rank:
 * bit 4 :| 000 = 2,A  001 = 3,A  010 = 1,B  011 = 2,B
 * bit 3 :/ 100 = 3,B  101 = 1,C  110 = 2,C  111 = 3,C
 * bit 2 :  0 = bonus at 10(1 car)/15(2 cars)/20(3 cars)  1 = bonus at 30/40/60
 * bit 1 :  1 = bonus at xxx points  0 = no bonus
 * bit 0 :  TEST
 *

write:
8014-801f sprites - 6 pairs: code (including flipping) and X position
8814-881f sprites - 6 pairs: Y position and color
8034-880c radar car indicators x position
8834-883c radar car indicators y position
a004-a00c radar car indicators color and x position MSB
a080      watchdog reset?
a105      sound voice 1 waveform (nibble)
a111-a113 sound voice 1 frequency (nibble)
a115      sound voice 1 volume (nibble)
a10a      sound voice 2 waveform (nibble)
a116-a118 sound voice 2 frequency (nibble)
a11a      sound voice 2 volume (nibble)
a10f      sound voice 3 waveform (nibble)
a11b-a11d sound voice 3 frequency (nibble)
a11f      sound voice 3 volume (nibble)
a130      virtual screen X scroll position
a140      virtual screen Y scroll position
a170      ? this is written to A LOT of times every frame
a180      explosion sound trigger
a181      interrupt enable
a182-a183 ?
a184      1 player start lamp
a185      2 players start lamp
a186      ?

I/O ports:
OUT on port $0 sets the interrupt vector/instruction (the game uses both
IM 2 and IM 0)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


void pengo_sound_w(int offset,int data);
int rallyx_sh_start(void);
void pengo_sh_update(void);
extern unsigned char *pengo_soundregs;

extern unsigned char *rallyx_videoram2,*rallyx_colorram2;
extern unsigned char *rallyx_radarcarx,*rallyx_radarcary,*rallyx_radarcarcolor;
extern unsigned char *rallyx_scrollx,*rallyx_scrolly;
void rallyx_videoram2_w(int offset,int data);
void rallyx_colorram2_w(int offset,int data);
int rallyx_vh_start(void);
void rallyx_vh_stop(void);
void rallyx_vh_screenrefresh(struct osd_bitmap *bitmap);


static void rallyx_play_sound_w(int offset, int data)
{
        static counter=0;

	if (play_sound == 0 || Machine->samples->sample[0] == 0)
		return;

        if (data == 0 && counter > 0)
        {
          osd_play_sample(4,Machine->samples->sample[0]->data,
                          Machine->samples->sample[0]->length,
                          Machine->samples->sample[0]->smpfreq,
                          Machine->samples->sample[0]->volume,0);
        }
        counter = data;
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9800, 0x9fff, MRA_RAM },
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0xa000, 0xa000, input_port_0_r },	/* IN0 */
	{ 0xa080, 0xa080, input_port_1_r },	/* IN1 */
	{ 0xa100, 0xa100, input_port_2_r },	/* DSW1 */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x8000, 0x83ff, videoram_w, &videoram, &videoram_size },
	{ 0x8400, 0x87ff, rallyx_videoram2_w, &rallyx_videoram2 },
	{ 0x8800, 0x8bff, colorram_w, &colorram },
	{ 0x8c00, 0x8fff, rallyx_colorram2_w, &rallyx_colorram2 },
	{ 0x9800, 0x9fff, MWA_RAM },
	{ 0xa080, 0xa080, MWA_NOP },
	{ 0xa130, 0xa130, MWA_RAM, &rallyx_scrollx },
	{ 0xa140, 0xa140, MWA_RAM, &rallyx_scrolly },
	{ 0xa004, 0xa00c, MWA_RAM, &rallyx_radarcarcolor },
	{ 0xa100, 0xa11f, pengo_sound_w, &pengo_soundregs },
	{ 0xa170, 0xa170, MWA_NOP },	/* ????? */
	{ 0xa180, 0xa180, rallyx_play_sound_w },
	{ 0xa181, 0xa181, interrupt_enable_w },
	{ 0xa182, 0xa186, MWA_NOP },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x8014, 0x801f, MWA_RAM, &spriteram },	/* these are here just to initialize */
	{ 0x8814, 0x881f, MWA_RAM, &spriteram_2 },	/* the pointers. */
	{ 0x8034, 0x803c, MWA_RAM, &rallyx_radarcarx },	/* ditto */
	{ 0x8834, 0x883c, MWA_RAM, &rallyx_radarcary },
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0, 0, interrupt_vector_w },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_3, OSD_KEY_CONTROL, OSD_KEY_LEFT, OSD_KEY_RIGHT,
				OSD_KEY_DOWN, OSD_KEY_UP, OSD_KEY_1, 0 },
		{ 0, OSD_JOY_FIRE, OSD_JOY_LEFT, OSD_JOY_RIGHT,
				OSD_JOY_DOWN, OSD_JOY_UP, 0, 0 }
	},
	{	/* IN1 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, OSD_KEY_2, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0xcb,
		{ OSD_KEY_F2, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};

static struct TrakPort trak_ports[] =
{
        { -1 }
};

static struct KEYSet keys[] =
{
        { 0, 5, "MOVE UP" },
        { 0, 2, "MOVE LEFT"  },
        { 0, 3, "MOVE RIGHT" },
        { 0, 4, "MOVE DOWN" },
        { 0, 1, "SMOKE" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 2, 0x38, "DIFFICULTY", { "2 CARS, RANK A", "3 CARS, RANK A", "1 CAR , RANK B", "2 CARS, RANK B",
			"3 CARS, RANK B", "1 CAR , RANK C", "2 CARS, RANK C", "3 CARS, RANK C" } },
	{ 2, 0x02, "BONUS", { "OFF", "ON" } },
	{ 2, 0x04, "BONUS SCORE", { "LOW", "HIGH" } },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 8*8+0, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3 },	/* bits are packed in groups of four */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 16 bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 8*8+0, 8*8+1, 8*8+2, 8*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3,	/* bits are packed in groups of four */
			 24*8+0, 24*8+1, 24*8+2, 24*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8	/* every sprite takes 64 bytes */
};
static struct GfxLayout radardotlayout =
{
	/* there is no gfx ROM for this one, it is generated by the hardware */
	2,2,	/* 2*2 square */
	1,	/* just one */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 0, 0 },	/* I "know" that this bit is 1 */
	{ 0, 0 },	/* I "know" that this bit is 1 */
	0	/* no use */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,   0, 32 },
	{ 1, 0x0000, &spritelayout, 0, 32 },
	{ 1, 0x0000, &radardotlayout, 0, 128 },
	{ -1 } /* end of array */
};



static unsigned char palette[] =
{
	0x00,0x00,0x00,	/* BLACK */
	0xff,0xff,0xff,	/* WHITE */
	0xff,0x00,0x00,	/* RED */
	0x00,0xff,0x00,	/* GREEN */
	0x00,0x00,0xff,	/* BLUE */
	0xff,0xff,0x00,	/* YELLOW */
	0xff,0x00,0xff,	/* PURPLE */
	0xaa,0xaa,0xaa,	/* LTGRAY */
	0x88,0x88,0x88	/* DKGRAY */
};

enum {BLACK,WHITE,RED,GREEN,BLUE,YELLOW,PURPLE,LTGRAY,DKGRAY};

static unsigned char colortable[] =
{
	BLACK,BLACK,BLACK,BLACK,
	BLACK,4,1,8,	/* enemy cars */
	BLACK,2,1,8,	/* player's car */
	BLACK,3,1,7,	/* radar */
	BLACK,5,7,1,	/* flag and score explanation on title screen */
	BLACK,2,7,1,	/* smoke */
	BLACK,2,7,1,	/* "HISCORE"; BANG */
	BLACK,7,7,7,	/* "MIDWAY" */
	BLACK,3,1,7,	/* left side of fuel bar */
	BLACK,7,1,3,	/* text on intro screen; fuel bar */
	BLACK,7,1,2,	/* "INSTRUCTIONS"; right side of fuel bar */
	BLACK,7,8,1,	/* "FUEL" */
	4,4,4,4,
	BLACK,3,7,8,	/* walls */
	BLACK,6,7,8,	/* 4th level walls */
	0,3,2,1,		/* blinking hi score after record */
	BLACK,7,7,7,		/* hi score */
	BLACK,8,2,3,	/* trees around the circuit */
	BLACK,1,2,3,	/* player's score; 4th level flowers */
	BLACK,3,3,3,	/* "(c) MIDWAY 1980" */
	0,4,4,4,		/* "THE HIGH SCORE OF THE DAY" */
	BLACK,5,7,6,	/* rocks */
	BLACK,5,7,2,	/* road, flags */
	7,7,7,7,
	7,7,7,7,
	1,1,1,1,
	2,2,2,2,
	3,3,3,3,
	4,4,4,4,
	5,5,5,5,
	6,6,6,6,
	7,7,7,7
};



/* waveforms for the audio hardware */
/* these are NOT the correct waveforms (I'm using Galaga ones) */
static unsigned char samples[8*32] =
{
	0xff,0x11,0x22,0x33,0x44,0x55,0x55,0x66,0x66,0x66,0x55,0x55,0x44,0x33,0x22,0x11,
	0xff,0xdd,0xcc,0xbb,0xaa,0x99,0x99,0x88,0x88,0x88,0x99,0x99,0xaa,0xbb,0xcc,0xdd,

	0xff,0x11,0x22,0x33,0xff,0x55,0x55,0xff,0x66,0xff,0x55,0x55,0xff,0x33,0x22,0x11,
	0xff,0xdd,0xff,0xbb,0xff,0x99,0xff,0x88,0xff,0x88,0xff,0x99,0xff,0xbb,0xff,0xdd,

	0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
	0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,

	0x33,0x55,0x66,0x55,0x44,0x22,0x00,0x00,0x00,0x22,0x44,0x55,0x66,0x55,0x33,0x00,
	0xcc,0xaa,0x99,0xaa,0xbb,0xdd,0xff,0xff,0xff,0xdd,0xbb,0xaa,0x99,0xaa,0xcc,0xff,

	0xff,0x22,0x44,0x55,0x66,0x55,0x44,0x22,0xff,0xcc,0xaa,0x99,0x88,0x99,0xaa,0xcc,
	0xff,0x33,0x55,0x66,0x55,0x33,0xff,0xbb,0x99,0x88,0x99,0xbb,0xff,0x66,0xff,0x88,

	0xff,0x66,0x44,0x11,0x44,0x66,0x22,0xff,0x44,0x77,0x55,0x00,0x22,0x33,0xff,0xaa,
	0x00,0x55,0x11,0xcc,0xdd,0xff,0xaa,0x88,0xbb,0x00,0xdd,0x99,0xbb,0xee,0xbb,0x99,

	0xff,0x00,0x22,0x44,0x66,0x55,0x44,0x44,0x33,0x22,0x00,0xff,0xdd,0xee,0xff,0x00,
	0x00,0x11,0x22,0x33,0x11,0x00,0xee,0xdd,0xcc,0xcc,0xbb,0xaa,0xcc,0xee,0x00,0x11,

	0x22,0x44,0x44,0x22,0xff,0xff,0x00,0x33,0x55,0x66,0x55,0x22,0xee,0xdd,0xdd,0xff,
	0x11,0x11,0x00,0xcc,0x99,0x88,0x99,0xbb,0xee,0xff,0xff,0xcc,0xaa,0xaa,0xcc,0xff,
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz ? */
			0,
			readmem,writemem,0,writeport,
			interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
	36*8, 28*8, { 0*8, 36*8-1, 0*8, 28*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,

	VIDEO_TYPE_RASTER,
	0,
	rallyx_vh_start,
	rallyx_vh_stop,
	rallyx_vh_screenrefresh,

	/* sound hardware */
	samples,
	0,
	rallyx_sh_start,
	0,
	pengo_sh_update
};


static const char *rallyx_sample_names[] =
{
	"BANG.SAM",
	0	/* end of array */
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( rallyx_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "rallyx.1b", 0x0000, 0x1000, 0xbea90675 )
	ROM_LOAD( "rallyx.1e", 0x1000, 0x1000, 0xe4021276 )
	ROM_LOAD( "rallyx.1h", 0x2000, 0x1000, 0x429161ad )
	ROM_LOAD( "rallyx.1k", 0x3000, 0x1000, 0x8551812b )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "rallyx.8e", 0x0000, 0x1000, 0xd007b15b )
ROM_END



struct GameDriver rallyx_driver =
{
	"Rally X",
	"rallyx",
	"NICOLA SALMORIA\nMIRKO BUFFONI",
	&machine_driver,

	rallyx_rom,
	0, 0,
	rallyx_sample_names,

	input_ports, 0, trak_ports, dsw, keys,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	0, 0
};
