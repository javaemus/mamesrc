/***************************************************************************

Moon Patrol memory map (preliminary)

0000-3fff ROM
8000-83ff Video RAM
8400-87ff Color RAM
e000-e7ff RAM


read:
8800      protection
d000      IN0
d001      IN1
d002      IN2
d003      DSW1
d004      DSW2

write:
c820-c87f sprites
c8a0-c8ff sprites
d000-d001 ?

I/O ports
write:
1c-1f     scroll registers
40        background #1 x position
60        background #1 y position
80        background #2 x position
a0        background #2 y position
c0        background control?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



int mpatrol_protection_r(int offset);

void mpatrol_scroll_w(int offset,int data);
void mpatrol_bg1xpos_w(int offset,int data);
void mpatrol_bg1ypos_w(int offset,int data);
void mpatrol_bg2xpos_w(int offset,int data);
void mpatrol_bg2ypos_w(int offset,int data);
void mpatrol_bgcontrol_w(int offset,int data);
int mpatrol_vh_start(void);
void mpatrol_vh_stop(void);
void mpatrol_vh_screenrefresh(struct osd_bitmap *bitmap);



static struct MemoryReadAddress readmem[] =
{
	{ 0xe000, 0xe7ff, MRA_RAM },
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xd000, 0xd000, input_port_0_r },	/* IN0 */
	{ 0xd001, 0xd001, input_port_1_r },	/* IN1 */
	{ 0xd002, 0xd002, input_port_2_r },	/* IN2 */
	{ 0xd003, 0xd003, input_port_3_r },	/* DSW1 */
	{ 0xd004, 0xd004, input_port_4_r },	/* DSW2 */
	{ 0x8800, 0x8800, mpatrol_protection_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0xe000, 0xe7ff, MWA_RAM },
	{ 0x8000, 0x83ff, videoram_w, &videoram, &videoram_size },
	{ 0x8400, 0x87ff, colorram_w, &colorram },
	{ 0xc820, 0xc87f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xc8a0, 0xc8ff, MWA_RAM, &spriteram_2 },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct IOWritePort writeport[] =
{
	{ 0x1c, 0x1f, mpatrol_scroll_w },
	{ 0x40, 0x40, mpatrol_bg1xpos_w },
	{ 0x60, 0x60, mpatrol_bg1ypos_w },
	{ 0x80, 0x80, mpatrol_bg2xpos_w },
	{ 0xa0, 0xa0, mpatrol_bg2ypos_w },
	{ 0xc0, 0xc0, mpatrol_bgcontrol_w },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_1, OSD_KEY_2, 0, OSD_KEY_3, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN1 */
		0xff,
		{ OSD_KEY_RIGHT, OSD_KEY_LEFT, 0, 0, 0, OSD_KEY_ALT, 0, OSD_KEY_CONTROL },
		{ OSD_JOY_RIGHT, OSD_JOY_LEFT, 0, 0, 0, OSD_JOY_FIRE2, 0, OSD_JOY_FIRE1 },
	},
	{	/* IN2 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0xfd,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW2 */
		0xfd,
		{ 0, 0, 0, 0, 0, 0, 0, OSD_KEY_F2 },
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
        { 1, 1, "MOVE LEFT"  },
        { 1, 0, "MOVE RIGHT" },
        { 1, 5, "JUMP" },
        { 1, 7, "FIRE" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 3, 0x03, "LIVES", { "2", "3", "4", "5" } },
	{ 3, 0x0c, "BONUS", { "NONE", "10000", "20 40 60000", "10 30 50000" } },
	{ 4, 0x20, "SECTOR SELECTION", { "YES", "NO" }, 1 },
	{ 4, 0x40, "DEMO MODE", { "YES", "NO" }, 1 },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 0, 512*8*8 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 128*16*16 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};
static struct GfxLayout bgcharlayout =
{
	64,64,	/* 64*64 characters */
	4,	/* 4 characters (actually, it is just 1 big 256x64 image) */
	2,	/* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3, 2*8+0, 2*8+1, 2*8+2, 2*8+3, 3*8+0, 3*8+1, 3*8+2, 3*8+3,
			4*8+0, 4*8+1, 4*8+2, 4*8+3, 5*8+0, 5*8+1, 5*8+2, 5*8+3, 6*8+0, 6*8+1, 6*8+2, 6*8+3, 7*8+0, 7*8+1, 7*8+2, 7*8+3,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 9*8+0, 9*8+1, 9*8+2, 9*8+3, 10*8+0, 10*8+1, 10*8+2, 10*8+3, 11*8+0, 11*8+1, 11*8+2, 11*8+3,
			12*8+0, 12*8+1, 12*8+2, 12*8+3, 13*8+0, 13*8+1, 13*8+2, 13*8+3, 14*8+0, 14*8+1, 14*8+2, 14*8+3, 15*8+0, 15*8+1, 15*8+2, 15*8+3 },
	{ 0*512, 1*512, 2*512, 3*512, 4*512, 5*512, 6*512, 7*512, 8*512, 9*512, 10*512, 11*512, 12*512, 13*512, 14*512, 15*512,
			16*512, 17*512, 18*512, 19*512, 20*512, 21*512, 22*512, 23*512, 24*512, 25*512, 26*512, 27*512, 28*512, 29*512, 30*512, 31*512,
			32*512, 33*512, 34*512, 35*512, 36*512, 37*512, 38*512, 39*512, 40*512, 41*512, 42*512, 43*512, 44*512, 45*512, 46*512, 47*512,
			48*512, 49*512, 50*512, 51*512, 52*512, 53*512, 54*512, 55*512, 56*512, 57*512, 58*512, 59*512, 60*512, 61*512, 62*512, 63*512 },
	128
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,      0, 32 },
	{ 1, 0x2000, &spritelayout, 32*4, 16 },
	{ 1, 0x4000, &bgcharlayout, 48*4,  1 },
	{ 1, 0x5000, &bgcharlayout, 49*4,  1 },
	{ 1, 0x6000, &bgcharlayout, 50*4,  1 },
	{ -1 } /* end of array */
};



static unsigned char palette[] =
{
        0x00,0x00,0x00,   /* black      */
        0x94,0x00,0xd8,   /* darkpurple */
        0xd8,0x00,0x00,   /* darkred    */
        0xf8,0x64,0xd8,   /* pink       */
        0x00,0xd8,0x00,   /* darkgreen  */
        0x00,0x00,0x80,   /* darkblue   */
        0xd8,0xd8,0x94,   /* darkyellow */
        0xd8,0xf8,0xd8,   /* darkwhite  */
        0xf8,0x94,0x44,   /* orange     */
        0x00,0x00,0xd8,   /* blue   */
        0xf8,0x00,0x00,   /* red    */
        0xff,0x00,0xff,   /* purple */
        0x00,0xf8,0x00,   /* green  */
        0x00,0xff,0xff,   /* cyan   */
        0xf8,0xf8,0x00,   /* yellow */
        0xff,0xff,0xff,    /* white  */
        255,183,115,    /* LTBROWN */
        167,3,3,        /* DKBROWN */
};

enum
{
        black, darkpurple, darkred, pink, darkgreen, darkblue, darkyellow,
                darkwhite, orange, blue, red, purple, green, cyan, yellow, white,
                ltbrown,dkbrown
};

static unsigned char colortable[] =
{
        /* chars */
        0,cyan,white,3, /* MOON PATROL on title screen */
        blue,red,white,1,       /* score beginner course */
        cyan,black,red,blue,    /* point / time beginner course */
        cyan,3,red,2,   /* lit caution led on champion course */
        green,orange,15,red,        /* ground */
        0,1,3,5,
        blue,black,1,2, /* high score point letter on beginner course */
        cyan,pink,1,2,  /* point letter on champion course */
        0,4,6,8,
        blue,black,red,2,       /* high score beginner course */
        0,9,12,15,
        pink,black,1,2, /* high score point letter on champion course */
        pink,red,yellow,1,      /* score champion course */
        cyan,black,red,pink,    /* point / time champion course */
        pink,black,red,2,       /* high score champion course */
        0,7,10,13,
        0,1,2,3,
        0,4,5,6,
        0,orange,orange,9,       /* starting ramp */
        0,10,11,12,
        0,13,14,15,
        0,1,3,5,
        0,7,9,11,
        0,13,15,2,
        0,4,6,8,
        0,10,12,14,
        0,1,4,7,
        0,10,13,2,
        0,5,8,11,
        0,14,3,6,
        0,9,12,15,
        0,7,10,13,

        /* sprites */
        0,black,pink,cyan,      /* moon patrol on beginner course */
        0,4,5,6,                /* buggy-shot, droping bomb, explosion 1 */
        0,7,8,9,                /* grabbing plant */
        0,10,11,12,             /* explosion 2 (on ground) */
        0,ltbrown,14,dkbrown,   /* boulders */
        0,1,3,5,
        0,7,9,11,
        0,13,15,2,              /* space crafts */
        0,4,6,8,                /* bottom part of plant */
        0,10,12,14,             /* any enemy  cars */

        0,1,4,7,                /* tri star bomb 1 */

        0,10,13,2,              /* tri star bomb 1 */

        0,black,red,cyan,       /* moon patrol on champion course */

        0,14,3,6,
        0,9,12,15,
        0,7,10,13,

        /* backgrounds */
        0,yellow,blue,green,
        0,1,darkgreen,green,
/*        0,white,darkblue,blue,*/
        0,0,darkblue,blue,
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
	32*8, 32*8, { 1*8, 31*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,

	VIDEO_TYPE_RASTER,
	0,
	mpatrol_vh_start,
	mpatrol_vh_stop,
	mpatrol_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( mpatrol_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "mp-a.3m", 0x0000, 0x1000, 0x138439c6 )
	ROM_LOAD( "mp-a.3l", 0x1000, 0x1000, 0x0c1bc43b )
	ROM_LOAD( "mp-a.3k", 0x2000, 0x1000, 0x56b4c738 )
	ROM_LOAD( "mp-a.3j", 0x3000, 0x1000, 0x9e598a75 )

	ROM_REGION(0x7000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "mp-e.3e", 0x0000, 0x1000, 0xc8e818a2 )	/* chars */
	ROM_LOAD( "mp-e.3f", 0x1000, 0x1000, 0x080d9163 )
	ROM_LOAD( "mp-b.3m", 0x2000, 0x1000, 0xfe518a23 )	/* sprites */
	ROM_LOAD( "mp-b.3n", 0x3000, 0x1000, 0x974b35c3 )
	ROM_LOAD( "mp-e.3h", 0x4000, 0x1000, 0x89ce19a8 )	/* background graphics */
	ROM_LOAD( "mp-e.3k", 0x5000, 0x1000, 0x48d8cace )
	ROM_LOAD( "mp-e.3l", 0x6000, 0x1000, 0x48b86bb0 )
ROM_END

ROM_START( mranger_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "mr-a.3m", 0x0000, 0x1000, 0x0440639c )
	ROM_LOAD( "mr-a.3l", 0x1000, 0x1000, 0xf25e07f8 )
	ROM_LOAD( "mr-a.3k", 0x2000, 0x1000, 0x6e686d92 )
	ROM_LOAD( "mr-a.3j", 0x3000, 0x1000, 0x39412cd3 )

	ROM_REGION(0x7000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "mr-e.3e", 0x0000, 0x1000, 0xefe9bb1d )	/* chars */
	ROM_LOAD( "mr-e.3f", 0x1000, 0x1000, 0x796d8525 )
	ROM_LOAD( "mr-b.3m", 0x2000, 0x1000, 0xfe518a23 )	/* sprites */
	ROM_LOAD( "mr-b.3n", 0x3000, 0x1000, 0x974b35c3 )
	ROM_LOAD( "mr-e.3h", 0x4000, 0x1000, 0x89ce19a8 )	/* background graphics */
	ROM_LOAD( "mr-e.3k", 0x5000, 0x1000, 0x48d8cace )
	ROM_LOAD( "mr-e.3l", 0x6000, 0x1000, 0x48b86bb0 )
ROM_END



struct GameDriver mpatrol_driver =
{
	"Moon Patrol",
	"mpatrol",
	"NICOLA SALMORIA\nCHRIS HARDY",
	&machine_driver,

	mpatrol_rom,
	0, 0,
	0,

	input_ports, 0, trak_ports, dsw, keys,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver mranger_driver =
{
	"Moon Ranger (bootleg Moon Patrol)",
	"mranger",
	"NICOLA SALMORIA\nCHRIS HARDY",
	&machine_driver,

	mranger_rom,
	0, 0,
	0,

	input_ports, 0, trak_ports, dsw, keys,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	0, 0
};
