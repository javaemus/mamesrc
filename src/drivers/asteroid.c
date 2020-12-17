/***************************************************************************

Asteroids Memory Map (preliminary)

Asteroids settings:

0 = OFF  1 = ON  X = Don't Care  $ = Atari suggests


8 SWITCH DIP
87654321
--------
XXXXXX11   English
XXXXXX10   German
XXXXXX01   French
XXXXXX00   Spanish
XXXXX1XX   4-ship game
XXXXX0XX   3-ship game
11XXXXXX   Free Play
10XXXXXX   1 Coin  for 2 Plays
01XXXXXX   1 Coin  for 1 Play
00XXXXXX   2 Coins for 1 Play

Asteroids Deluxe settings:

0 = OFF  1 = ON  X = Don't Care  $ = Atari suggests


8 SWITCH DIP (R5)
87654321
--------
XXXXXX11   English $
XXXXXX10   German
XXXXXX01   French
XXXXXX00   Spanish
XXXX11XX   2-4 ships
XXXX10XX   3-5 ships $
XXXX01XX   4-6 ships
XXXX00XX   5-7 ships
XXX1XXXX   1-play minimum $
XXX0XXXX   2-play minimum
11XXXXXX   Bonus ship every 10,000 points $
10XXXXXX   Bonus ship every 12,000 points
01XXXXXX   Bonus ship every 15,000 points
00XXXXXX   No bonus ships (adds one ship at game start)


8 SWITCH DIP (L8)
87654321
--------
XXXXXX11   Free Play
XXXXXX10   1 Coin = 2 Plays
XXXXXX01   1 Coin = 1 Play
XXXXXX00   2 Coins = 1 Play $
XXXX11XX   Right coin mech * 1 $
XXXX10XX   Right coin mech * 4
XXXX01XX   Right coin mech * 5
XXXX00XX   Right coin mech * 6
XXX1XXXX   Center coin mech * 1 $
XXX0XXXX   Center coin mech * 2
111XXXXX   No bonus coins
110XXXXX   For every 2 coins inserted, game logic adds 1 more coin
101XXXXX   For every 4 coins inserted, game logic adds 1 more coin
100XXXXX   For every 4 coins inserted, game logic adds 2 more coins $
011XXXXX   For every 5 coins inserted, game logic adds 1 more coin
***************************************************************************/

/***************************************************************************

Lunar Lander Memory Map (preliminary)

Lunar Lander settings:

0 = OFF  1 = ON  x = Don't Care  $ = Atari suggests


8 SWITCH DIP (P8) with -01 ROMs on PCB
87654321
--------
11xxxxxx   450 fuel units per coin
10xxxxxx   600 fuel units per coin
01xxxxxx   750 fuel units per coin  $
00xxxxxx   900 fuel units per coin
xxx0xxxx   Free play
xxx1xxxx   Coined play as determined by toggles 7 & 8  $
xxxx00xx   German instructions
xxxx01xx   Spanish instructions
xxxx10xx   French instructions
xxxx11xx   English instructions  $
xxxxxx11   Right coin == 1 credit/coin  $
xxxxxx10   Right coin == 4 credit/coin
xxxxxx01   Right coin == 5 credit/coin
xxxxxx00   Right coin == 6 credit/coin
           (Left coin always registers 1 credit/coin)


8 SWITCH DIP (P8) with -02 ROMs on PCB
87654321
--------
11x1xxxx   450 fuel units per coin
10x1xxxx   600 fuel units per coin
01x1xxxx   750 fuel units per coin  $
00x1xxxx   900 fuel units per coin
11x0xxxx   1100 fuel units per coin
10x0xxxx   1300 fuel units per coin
01x0xxxx   1550 fuel units per coin
00x0xxxx   1800 fuel units per coin
xx0xxxxx   Free play
xx1xxxxx   Coined play as determined by toggles 5, 7, & 8  $
xxxx00xx   German instructions
xxxx01xx   Spanish instructions
xxxx10xx   French instructions
xxxx11xx   English instructions  $
xxxxxx11   Right coin == 1 credit/coin  $
xxxxxx10   Right coin == 4 credit/coin
xxxxxx01   Right coin == 5 credit/coin
xxxxxx00   Right coin == 6 credit/coin
           (Left coin always registers 1 credit/coin)

Notes:

$3200 - LED control?
		$08 during gameplay
		$10, $1f coin light?
		$18, $28, $30, $38, $3f start led?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/avgdvg.h"
#include "sndhrdw/pokyintf.h"
#include "machine/atari_vg.h"

void asteroid_init_machine(void);
void astdelux_init_machine(void);
int asteroid_interrupt(void);
void llander_init_machine(void);
int llander_interrupt(void);

void asteroid_bank_switch_w(int offset, int data);
void astdelux_bank_switch_w (int offset,int data);

void asteroid_explode_w(int offset,int data);
void asteroid_thump_w(int offset,int data);
void asteroid_sounds_w(int offset,int data);
void asteroid_sh_update(void);
int astdelux_sh_start (void);
void astdelux_sounds_w(int offset,int data);
void llander_sounds_w(int offset,int data);

int asteroid_IN0_r(int offset);
int asteroid_IN1_r(int offset);
int asteroid_DSW1_r(int offset);
int llander_IN0_r(int offset);

int asteroid_catch_busyloop(int offset);

int llander_zeropage_r (int offset);
void llander_zeropage_w (int offset, int data);

static struct MemoryReadAddress asteroid_readmem[] =
{
	{ 0x0000, 0x03ff, MRA_RAM },
	{ 0x6800, 0x7fff, MRA_ROM },
	{ 0x5000, 0x57ff, MRA_ROM }, /* vector rom */
	{ 0xf800, 0xffff, MRA_ROM }, /* for the reset / interrupt vectors */
	{ 0x4000, 0x47ff, MRA_RAM, &vectorram, &vectorram_size },
	{ 0x2000, 0x2007, asteroid_IN0_r }, /* IN0 */
	{ 0x2400, 0x2407, asteroid_IN1_r }, /* IN1 */
	{ 0x2800, 0x2803, asteroid_DSW1_r }, /* DSW1 */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress asteroid_writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x3000, 0x3000, avgdvg_go },
	{ 0x3200, 0x3200, asteroid_bank_switch_w },
	{ 0x3400, 0x3400, MWA_NOP }, /* watchdog clear */
	{ 0x3600, 0x3600, asteroid_explode_w },
	{ 0x3a00, 0x3a00, asteroid_thump_w },
	{ 0x3c00, 0x3c05, asteroid_sounds_w },
	{ 0x6800, 0x7fff, MWA_ROM },
	{ 0x5000, 0x57ff, MWA_ROM }, /* vector rom */
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress astdelux_readmem[] =
{
	{ 0x0000, 0x03ff, MRA_RAM },
	{ 0x6000, 0x7fff, MRA_ROM },
	{ 0x4800, 0x57ff, MRA_ROM }, /* vector rom */
	{ 0xf800, 0xffff, MRA_ROM }, /* for the reset / interrupt vectors */
	{ 0x4000, 0x47ff, MRA_RAM, &vectorram, &vectorram_size },
	{ 0x2000, 0x2007, asteroid_IN0_r }, /* IN0 */
	{ 0x2400, 0x2407, asteroid_IN1_r }, /* IN1 */
	{ 0x2800, 0x2803, asteroid_DSW1_r }, /* DSW1 */
	{ 0x2c00, 0x2c0f, pokey1_r },
	{ 0x2c40, 0x2c7f, atari_vg_earom_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress astdelux_writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x3000, 0x3000, avgdvg_go },
	{ 0x2c00, 0x2c0f, pokey1_w },
	{ 0x3200, 0x323f, atari_vg_earom_w },
	{ 0x3400, 0x3400, MWA_NOP }, /* watchdog clear */
	{ 0x3600, 0x3600, asteroid_explode_w },
	{ 0x2405, 0x2405, astdelux_sounds_w }, /* thrust sound */
	{ 0x3a00, 0x3a00, atari_vg_earom_ctrl },
	{ 0x3c00, 0x3c03, MWA_NOP }, /* P1 LED, P2 LED, unknown, thrust? */
	{ 0x3c04, 0x3c04, astdelux_bank_switch_w },
	{ 0x3c05, 0x3c07, MWA_NOP }, /* Coin LED left, right, middle */
	{ 0x6000, 0x7fff, MWA_ROM },
	{ 0x4800, 0x57ff, MWA_ROM }, /* vector rom */
	{ -1 }	/* end of table */
};

/* Lunar Lander mirrors page 0 and page 1. Unfortunately,
   the 6502 could only handle that with a severe performance hit.
   It only seems to affect the selftest */
static struct MemoryReadAddress llander_readmem[] =
{
	{ 0x0000, 0x01ff, MRA_RAM },
//	{ 0x0000, 0x00ff, llander_zeropage_r },
//	{ 0x0100, 0x01ff, MRA_RAM },
	{ 0x6000, 0x7fff, MRA_ROM },
	{ 0x4800, 0x5fff, MRA_ROM }, /* vector rom */
	{ 0xf800, 0xffff, MRA_ROM }, /* for the reset / interrupt vectors */
	{ 0x4000, 0x47ff, MRA_RAM, &vectorram, &vectorram_size },
	{ 0x2000, 0x2000, llander_IN0_r }, /* IN0 */
	{ 0x2400, 0x2407, asteroid_IN1_r }, /* IN1 */
	{ 0x2800, 0x2803, asteroid_DSW1_r }, /* DSW1 */
	{ 0x2c00, 0x2c00, input_port_3_r }, /* IN3 */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress llander_writemem[] =
{
	{ 0x0000, 0x01ff, MWA_RAM },
//	{ 0x0000, 0x00ff, llander_zeropage_w },
//	{ 0x0100, 0x01ff, MWA_RAM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x3000, 0x3000, avgdvg_go },
	{ 0x3400, 0x3400, MWA_NOP }, /* watchdog clear */
	{ 0x3c00, 0x3c00, llander_sounds_w },
/*	{ 0x3e00, 0x3e00, llander_snd_reset }, */
	{ 0x6000, 0x7fff, MWA_ROM },
	{ 0x4800, 0x5fff, MWA_ROM }, /* vector rom */
	{ -1 }  /* end of table */
};

INPUT_PORTS_START ( asteroid_input_ports )
	PORT_START /* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	/* Bit 2 and 3 are handled in the machine dependent part. */
        /* Bit 2 is the 3 KHz source and Bit 3 the VG_HALT bit    */
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BITX ( 0x20, IP_ACTIVE_HIGH, IPT_SERVICE, "Diagnostic Step", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_BITX ( 0x80, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING ( 0x00, "Off" )
	PORT_DIPSETTING ( 0x80, "On" )
	PORT_START /* IN1 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_START /* DSW1 */
	PORT_DIPNAME ( 0x03, 0x00, "Language", IP_KEY_NONE )
	PORT_DIPSETTING (    0x00, "English" )
	PORT_DIPSETTING (    0x01, "German" )
	PORT_DIPSETTING (    0x02, "French" )
	PORT_DIPSETTING (    0x03, "Spanish" )
	PORT_DIPNAME ( 0x04, 0x00, "Ships", IP_KEY_NONE )
	PORT_DIPSETTING (    0x00, "4" )
	PORT_DIPSETTING (    0x04, "3" )
	PORT_DIPNAME ( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING (    0x00, "Off" )
	PORT_DIPSETTING (    0x08, "On" )
	PORT_DIPNAME ( 0x10, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING (    0x00, "Off" )
	PORT_DIPSETTING (    0x10, "On" )
	PORT_DIPNAME ( 0x20, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING (    0x00, "Off" )
	PORT_DIPSETTING (    0x20, "On" )
	PORT_DIPNAME ( 0xc0, 0x80, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING (    0x00, "Free Play" )
	PORT_DIPSETTING (    0x40, "1 Coin / 2 Plays" )
	PORT_DIPSETTING (    0x80, "1 Coin / 1 Play" )
	PORT_DIPSETTING (    0xc0, "2 Coins / 1 Play" )
INPUT_PORTS_END

INPUT_PORTS_START ( astdelux_input_ports )
	PORT_START /* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	/* Bit 2 and 3 are handled in the machine dependent part. */
	/* Bit 2 is the 3 KHz source and Bit 3 the VG_HALT bit    */
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BITX ( 0x20, IP_ACTIVE_HIGH, IPT_SERVICE, "Diagnostic Step", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_BITX ( 0x80, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING ( 0x00, "Off" )
	PORT_DIPSETTING ( 0x80, "On" )
	PORT_START /* IN1 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_START /* DSW 1 */
	PORT_DIPNAME ( 0x03, 0x00, "Language", IP_KEY_NONE )
	PORT_DIPSETTING (    0x00, "English" )
	PORT_DIPSETTING (    0x01, "German" )
	PORT_DIPSETTING (    0x02, "French" )
	PORT_DIPSETTING (    0x03, "Spanish" )
	PORT_DIPNAME ( 0x0c, 0x04, "Ships", IP_KEY_NONE )
	PORT_DIPSETTING (    0x00, "2-4" )
	PORT_DIPSETTING (    0x04, "3-5" )
	PORT_DIPSETTING (    0x08, "4-6" )
	PORT_DIPSETTING (    0x0c, "5-7" )
	PORT_DIPNAME ( 0x10, 0x00, "Minimum plays", IP_KEY_NONE )
	PORT_DIPSETTING (    0x00, "1" )
	PORT_DIPSETTING (    0x10, "2" )
	PORT_DIPNAME ( 0x20, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING (    0x00, "Off" )
	PORT_DIPSETTING (    0x20, "On" )
	PORT_DIPNAME ( 0xc0, 0x00, "Bonus ship", IP_KEY_NONE )
	PORT_DIPSETTING (    0x00, "10000" )
	PORT_DIPSETTING (    0x40, "12000" )
	PORT_DIPSETTING (    0x80, "15000" )
	PORT_DIPSETTING (    0xc0, "None" )
	PORT_START /* DSW 2 */
	PORT_DIPNAME ( 0x03, 0x01, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING (    0x00, "2 Coins / 1 Play" )
	PORT_DIPSETTING (    0x01, "1 Coin / 1 Play" )
	PORT_DIPSETTING (    0x02, "1 Coin / 2 Plays" )
	PORT_DIPSETTING (    0x03, "Free Play" )
	PORT_DIPNAME ( 0x0c, 0x0c, "Right Coin", IP_KEY_NONE )
	PORT_DIPSETTING (    0x00, "*6" )
	PORT_DIPSETTING (    0x04, "*5" )
	PORT_DIPSETTING (    0x08, "*4" )
	PORT_DIPSETTING (    0x0c, "*1" )
	PORT_DIPNAME ( 0x10, 0x10, "Center Coin", IP_KEY_NONE)
	PORT_DIPSETTING (    0x00, "*2" )
	PORT_DIPSETTING (    0x10, "*1" )
	PORT_DIPNAME ( 0xe0, 0x80, "Bonus Coins", IP_KEY_NONE)
	PORT_DIPSETTING (    0x60, "1 each 5" )
	PORT_DIPSETTING (    0x80, "2 each 4" )
	PORT_DIPSETTING (    0xa0, "1 each 4" )
	PORT_DIPSETTING (    0xc0, "1 each 2" )
	PORT_DIPSETTING (    0xe0, "None" )
INPUT_PORTS_END

INPUT_PORTS_START ( llander_input_ports )
	PORT_START /* IN0 */
	/* Bit 0 is VG_HALT, handled in the machine dependant part */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BITX ( 0x02, 0x02, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING ( 0x02, "Off" )
	PORT_DIPSETTING ( 0x00, "On" )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	/* Of the rest, Bit 6 is the 3KHz source. 3,4 and 5 are unknown */
	PORT_BIT ( 0x78, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_SERVICE, "Diagnostic Step", OSD_KEY_F1, IP_JOY_NONE, 0 )

	PORT_START /* IN1 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1, "Abort", OSD_KEY_A, OSD_JOY_FIRE, 0 )
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )

	PORT_START /* DSW1 */
	PORT_DIPNAME ( 0x03, 0x01, "Right Coin", IP_KEY_NONE )
	PORT_DIPSETTING (    0x00, "*1" )
	PORT_DIPSETTING (    0x01, "*4" )
	PORT_DIPSETTING (    0x02, "*5" )
	PORT_DIPSETTING (    0x03, "*6" )
	PORT_DIPNAME ( 0x0c, 0x00, "Language", IP_KEY_NONE )
	PORT_DIPSETTING (    0x00, "English" )
	PORT_DIPSETTING (    0x04, "French" )
	PORT_DIPSETTING (    0x08, "Spanish" )
	PORT_DIPSETTING (    0x0c, "German" )
	PORT_DIPNAME ( 0x20, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING (    0x00, "Normal" )
	PORT_DIPSETTING (    0x20, "Free Play" )
	PORT_DIPNAME ( 0xd0, 0x80, "Fuel units", IP_KEY_NONE )
	PORT_DIPSETTING (    0x00, "450" )
	PORT_DIPSETTING (    0x10, "1100" )
	PORT_DIPSETTING (    0x40, "600" )
	PORT_DIPSETTING (    0x50, "1300" )
	PORT_DIPSETTING (    0x80, "750" )
	PORT_DIPSETTING (    0x90, "1550" )
	PORT_DIPSETTING (    0xc0, "900" )
	PORT_DIPSETTING (    0xd0, "1800" )

	/* The next one is a potentiometer */
	PORT_START /* IN3 */
	PORT_ANALOG ( 0xff, 0x00, IPT_AD_STICK_Y, 100, 7, 0, 255 )

INPUT_PORTS_END

static struct GfxLayout fakelayout =
{
	1,1,
	0,
	1,
	{ 0 },
	{ 0 },
	{ 0 },
	0
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0,      &fakelayout,     0, 256 },
	{ -1 } /* end of array */
};

static unsigned char color_prom[] =
{
	0x00,0x00,0x00,	/* BLACK */
	0x00,0x00,0x01, /* BLUE */
	0x00,0x01,0x00, /* GREEN */
	0x00,0x01,0x02, /* CYAN */
	0x01,0x00,0x00, /* RED */
	0x01,0x00,0x01, /* MAGENTA */
	0x01,0x01,0x00, /* YELLOW */
	0x01,0x01,0x01,	/* WHITE */
	0x00,0x00,0x00,	/* BLACK */
	0x00,0x00,0x01, /* BLUE */
	0x00,0x01,0x00, /* GREEN */
	0x00,0x01,0x02, /* CYAN */
	0x01,0x00,0x00, /* RED */
	0x01,0x00,0x01, /* MAGENTA */
	0x01,0x01,0x00, /* YELLOW */
	0x01,0x01,0x01	/* WHITE */};

static int asteroid_hiload(void)
{
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x001d],"\x00\x00",2) == 0 &&
			memcmp(&RAM[0x0050],"\x00\x00",2) == 0 &&
			memcmp(&RAM[0x0032],"\xff\xff",2) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x001d],2*10+3*11);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void asteroid_hisave(void)
{
	void *f;


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x001d],2*10+3*11);
		osd_fclose(f);
	}
}

/* Asteroids Deluxe now uses the earom routines
 * However, we keep the highscore location, just in case
 *		osd_fwrite(f,&RAM[0x0023],3*10+3*11);
 */

static struct MachineDriver asteroid_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1500000,	/* 1.5 Mhz */
			0,
			asteroid_readmem,asteroid_writemem,0,0,
			asteroid_interrupt,6 /* approx 250Hz? */
		}
	},
	45, /* frames per second */
	1,
	asteroid_init_machine,

	/* video hardware */
	288, 224, { 0, 1020, 70, 950 },
	gfxdecodeinfo,
	256, 256,
	avg_init_colors,

	VIDEO_TYPE_VECTOR,
	0,
	dvg_start,
	dvg_stop,
	dvg_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	asteroid_sh_update
};

static struct MachineDriver astdelux_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1500000,	/* 1.5 Mhz */
			0,
			astdelux_readmem,astdelux_writemem,0,0,
			asteroid_interrupt,6 /* approx 250Hz? */
		}
	},
	45, /* frames per second */
	1,
	astdelux_init_machine,

	/* video hardware */
	288, 224, { 0, 1020, 70, 950 },
	gfxdecodeinfo,
	256, 256,
	avg_init_colors,

	VIDEO_TYPE_VECTOR,
	0,
	dvg_start,
	dvg_stop,
	dvg_screenrefresh,

	/* sound hardware */
	0,
	0,
	astdelux_sh_start,
	pokey_sh_stop,
	pokey_sh_update
};

static struct MachineDriver llander_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1500000,	/* 1.5 Mhz */
			0,
			llander_readmem, llander_writemem,0,0,
			llander_interrupt,6 /* approx 250Hz? */
		}
	},
	45, /* frames per second */
	1,
	llander_init_machine,

	/* video hardware */
	288, 224, { -30, 1050, 0, 900 },
	gfxdecodeinfo,
	256, 256,
	avg_init_colors,

	VIDEO_TYPE_VECTOR,
	0,
	dvg_start,
	dvg_stop,
	dvg_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	asteroid_sh_update
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

static const char *asteroid_sample_names[] =
{
	"explode1.sam",
	"explode2.sam",
	"explode3.sam",
	"thrust.sam",
	"thumphi.sam",
	"thumplo.sam",
	"fire.sam",
	"lsaucer.sam",
	"ssaucer.sam",
	"sfire.sam",
	"life.sam",
    0	/* end of array */
};

ROM_START( asteroid_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "035145.02", 0x6800, 0x0800, 0xd84a7878 )
	ROM_LOAD( "035144.02", 0x7000, 0x0800, 0x15f39999 )
	ROM_LOAD( "035143.02", 0x7800, 0x0800, 0x93d25050 )
	ROM_RELOAD(            0xf800, 0x0800 )	/* for reset/interrupt vectors */
	/* Vector ROM */
	ROM_LOAD( "035127.02", 0x5000, 0x0800, 0xa144e0e0 )
ROM_END

ROM_START( asteroi2_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "035145.02", 0x6800, 0x0800, 0x8de50d0d )
	ROM_LOAD( "035144.02", 0x7000, 0x0800, 0xa0dbd7d7 )
	ROM_LOAD( "035143.02", 0x7800, 0x0800, 0x0454f0f0 )
	ROM_RELOAD(            0xf800, 0x0800 )	/* for reset/interrupt vectors */
	/* Vector ROM */
	ROM_LOAD( "035127.02", 0x5000, 0x0800, 0xed041c1c )
ROM_END



struct GameDriver asteroid_driver =
{
	"Asteroids",
	"asteroid",
	VECTOR_TEAM,

	&asteroid_machine_driver,

	asteroid_rom,
	0, 0,
	asteroid_sample_names,

	0, asteroid_input_ports, 0, 0, 0,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	asteroid_hiload, asteroid_hisave
};

struct GameDriver asteroi2_driver =
{
	"Asteroids (alternate version)",
	"asteroi2",
	VECTOR_TEAM,

	&asteroid_machine_driver,

	asteroi2_rom,
	0, 0,
	asteroid_sample_names,

	0, asteroid_input_ports, 0, 0, 0,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	asteroid_hiload, asteroid_hisave
};

ROM_START( astdelux_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "036430.02", 0x6000, 0x0800, 0x4eb73d37 )
	ROM_LOAD( "036431.02", 0x6800, 0x0800, 0x859ee29c )
	ROM_LOAD( "036432.02", 0x7000, 0x0800, 0x49c7b38d )
	ROM_LOAD( "036433.03", 0x7800, 0x0800, 0x39b00100 )
	ROM_RELOAD(            0xf800, 0x0800 )	/* for reset/interrupt vectors */
	/* Vector ROM */
	ROM_LOAD( "036800.02", 0x4800, 0x0800, 0x6ea4e6c6 )
	ROM_LOAD( "036799.01", 0x5000, 0x0800, 0x5bcf256d )
ROM_END

ROM_START( astdelu1_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "036430.02", 0x6000, 0x0800, 0x4bca4e5e )
	ROM_LOAD( "036431.02", 0x6800, 0x0800, 0x2d01a733 )
	ROM_LOAD( "036432.02", 0x7000, 0x0800, 0xf432f48a )
	ROM_LOAD( "036433.03", 0x7800, 0x0800, 0x8c188eea )
	ROM_RELOAD(            0xf800, 0x0800 )	/* for reset/interrupt vectors */
	/* Vector ROM */
	ROM_LOAD( "036800.02", 0x4800, 0x0800, 0xc05aa9d8 )
	ROM_LOAD( "036799.01", 0x5000, 0x0800, 0x5bcf256d )
ROM_END

static const char *astdelux_sample_names[] =
{
	"explode1.sam",
	"explode2.sam",
	"explode3.sam",
	"thrust.sam",
	0
};

struct GameDriver astdelux_driver =
{
	"Asteroids Deluxe",
	"astdelux",
	VECTOR_TEAM,

	&astdelux_machine_driver,

	astdelux_rom,
	0, 0,
	astdelux_sample_names,

	0, astdelux_input_ports, 0, 0, 0,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	atari_vg_earom_load, atari_vg_earom_save
};

struct GameDriver astdelu1_driver =
{
	"Asteroids Deluxe (alternate version)",
	"astdelu1",
	VECTOR_TEAM,

	&astdelux_machine_driver,

	astdelu1_rom,
	0, 0,
	asteroid_sample_names,

	0, astdelux_input_ports, 0, 0, 0,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	atari_vg_earom_load, atari_vg_earom_save
};

static const char *llander_sample_names[] =
{
	"thrust.sam",
	"beep.sam",
	"explode1.sam",
	0	/* end of array */
};

ROM_START( llander_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "034572.02", 0x6000, 0x0800, 0xee299da1 )
	ROM_LOAD( "034571.02", 0x6800, 0x0800, 0xed92222e )
	ROM_LOAD( "034570.02", 0x7000, 0x0800, 0xfe8b233f )
	ROM_LOAD( "034569.02", 0x7800, 0x0800, 0x5cdbe9e5 )
	ROM_RELOAD(            0xf800, 0x0800 )	/* for reset/interrupt vectors */
	/* Vector ROM */
	ROM_LOAD( "034599.01", 0x4800, 0x0800, 0x9e7084de )
	ROM_LOAD( "034598.01", 0x5000, 0x0800, 0xd006607c )
	ROM_LOAD( "034597.01", 0x5800, 0x0800, 0xfc000000 )
ROM_END

struct GameDriver llander_driver =
{
	"Lunar Lander",
	"llander",
	VECTOR_TEAM,

	&llander_machine_driver,

	llander_rom,
	0, 0,
	llander_sample_names,

	0, llander_input_ports, 0, 0, 0,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

