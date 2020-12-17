/***************************************************************************
File: drivers\starwars.c

STARWARS HARDWARE FILE

This file is Copyright 1997, Steve Baines.
Modified by Frank Palazzolo for sound support

Current e-mail contact address:  sjb@ohm.york.ac.uk

Release 2.0 (6 August 1997)

***************************************************************************

Comments:

Release 2.0:

My first contribution to MAME as well.  Added Pokey Sound Support.
Seems to cut out some music prematurely, when there are other sounds going,
but otherwise ok.  Attract mode music may not work quite right either.
No speech yet.

Release 1.0:

This is the first release of my Star Wars driver for MAME. This is also my
first ever driver for MAME!

Known Bugs/Issues:

1) The vector generator I'm using sometimes draws lines in the wrong place
when in the trench sequence (you'll see what I mean).  I think this is due to
a wraparound effect somewhere in the vector code and should be fixed soon.

2) On the Death Star surface run the ship banks in the correct direction
in response to left/right movements, but the surface moves in the other
direction.  I'm a bit puzzled by this, maybe I've got buggy ROMS?

3) No mouse support yet

4) No sound support yet

5) DIP setting in MAME doesn't work, but the game has internal DIP switch
setting which does work, so this doesn't really matter.  Press and hold
the SELF TEST key, and accounting info is shown. Still holding that key, press
AUX COIN, and the settings menu appears.

6) NovRAM, Mathbox self tests fail.  This doesn't matter, they work
properley for the purposes of everything but the self-test.

7) PAUSE doesn't clear itself when un-paused

That's all I can think of at the moment

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/avgdvg.h"
#include "sndhrdw/pokyintf.h"
#include "sndhrdw/starwars.h"
#include "machine/swmathbx.h"
#include "machine/starwars.h"
#include "machine/atari_vg.h"

#define EMPIRE 0

/* Star Wars READ memory map */
static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x2fff, MRA_RAM, &vectorram, &vectorram_size },   /* vector_ram */
	{ 0x3000, 0x3fff, MRA_ROM },		/* vector_rom */
/*	{ 0x4800, 0x4fff, MRA_RAM }, */		/* cpu_ram */
/*	{ 0x5000, 0x5fff, MRA_RAM }, */		/* (math_ram_r) math_ram */
/*	{ 0x0000, 0x3fff, MRA_RAM, &vectorram}, *//* Vector RAM and ROM */
	{ 0x4800, 0x5fff, MRA_RAM },		/* CPU and Math RAM */
	{ 0x6000, 0x7fff, MRA_BANK1 },	/* banked ROM */
	{ 0x8000, 0xffff, MRA_ROM },		/* rest of main_rom */
	{ 0x4300, 0x431f, input_port_0_r }, /* Memory mapped input port 0 */
	{ 0x4320, 0x433f, input_bank_1_r }, /* Memory mapped input port 1 */
	{ 0x4340, 0x435f, input_port_2_r },	/* DIP switches bank 0 */
	{ 0x4360, 0x437f, input_port_3_r },	/* DIP switches bank 1 */
	{ 0x4380, 0x439f, control_r },		/* a-d control result */
	{ 0x4400, 0x4400, main_read_r },
	{ 0x4401, 0x4401, main_ready_flag_r },
	{ 0x4500, 0x45ff, MRA_RAM },		/* nov_ram */
	{ 0x4700, 0x4700, reh },
	{ 0x4701, 0x4701, rel },
	{ 0x4703, 0x4703, prng },			/* pseudo random number generator */
	{ -1 }	/* end of table */
};

/* Star Wars Sound READ memory map */
static struct MemoryReadAddress readmem2[] =
{
	{ 0x0800, 0x0fff, sin_r },		/* SIN Read */

	{ 0x1000, 0x107f, MRA_RAM },	/* 6532 RAM */
	{ 0x1080, 0x109f, m6532_r },

	{ 0x2000, 0x27ff, MRA_RAM },	/* program RAM */
	{ 0x4000, 0x7fff, MRA_ROM },	/* sound roms */
	{ 0xc000, 0xffff, MRA_ROM },	/* load last rom twice */
									/* for proper int vec operation */

	{ -1 }  /* end of table */
};

/* Star Wars WRITE memory map */
static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x2fff, MWA_RAM, &vectorram }, /* vector_ram */
	{ 0x3000, 0x3fff, MWA_ROM },		/* vector_rom */
/*	{ 0x4800, 0x4fff, MWA_RAM }, */		/* cpu_ram */
/*	{ 0x5000, 0x5fff, MWA_RAM }, */		/* (math_ram_w) math_ram */
	{ 0x4800, 0x5fff, MWA_RAM },		/* CPU and Math RAM */
	{ 0x6000, 0xffff, MWA_ROM },		/* main_rom */
	{ 0x4400, 0x4400, main_wr_w },
	{ 0x4500, 0x45ff, MWA_RAM },		/* nov_ram */
	{ 0x4600, 0x461f, avgdvg_go },		/* evggo(mine) or vg2_go */
	{ 0x4620, 0x463f, avgdvg_reset },	/* evgres(mine) or vg_reset */
	{ 0x4640, 0x465f, MWA_NOP },		/* (wdclr) Watchdog clear */
	{ 0x4660, 0x467f, irqclr },			/* clear periodic interrupt */
	{ 0x4680, 0x4681, MWA_NOP },		/* Coin counters */
/*	{ 0x4680, 0x4680, MWA_NOP }, */		/*   (coin_ctr2) Coin counter 1 */
/*	{ 0x4681, 0x4681, MWA_NOP }, */		/*   (coin_ctr1) Coin counter 2 */
	{ 0x4682, 0x4682, led3 },			/* led3 */
	{ 0x4683, 0x4683, led2 },			/* led2 */
	{ 0x4684, 0x4684, mpage },			/* Page select for ROM0 */
	{ 0x4685, 0x4685, prngclr },		/* Reset PRNG */
	{ 0x4686, 0x4686, led1 },			/* led1 */
	{ 0x4687, 0x4687, recall },
	{ 0x46a0, 0x46bf, nstore },
	{ 0x46c0, 0x46c2, control_w },		/* Selects which a-d control port (0-3) will be read */
	{ 0x46e0, 0x46e0, soundrst },
	{ 0x4700, 0x4707, swmathbx },
	{ -1 }	/* end of table */
};

/* Star Wars sound WRITE memory map */
static struct MemoryWriteAddress writemem2[] =
{
	{ 0x0000, 0x07ff, sout_w },

	{ 0x1000, 0x107f, MWA_RAM }, /* 6532 ram */
	{ 0x1080, 0x109f, m6532_w },

	{ 0x1800, 0x181f, starwars_pokey_sound_w },
	{ 0x1820, 0x183f, starwars_pokey_ctl_w },

	{ 0x2000, 0x27ff, MWA_RAM }, /* program RAM */
	{ 0x4000, 0x7fff, MWA_ROM }, /* sound rom */
	{ 0xc000, 0xffff, MWA_ROM }, /* sound rom again, for intvecs */

	{ -1 }  /* end of table */
};

INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_COIN1)
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_COIN2)
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_COIN3)
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_TILT, "Slam", OSD_KEY_S, IP_JOY_NONE, 0 )
	PORT_BITX(    0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_START2)
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_START1)

	PORT_START	/* IN1 */
	PORT_BIT ( 0x03, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_SERVICE, "Diagnostic Step", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2)
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1)
	/* Bit 6 is MATH_RUN - see machine/starwars.c */
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED)
	/* Bit 7 is VG_HALT - see machine/starwars.c */
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START	/* DSW0 */
	PORT_DIPNAME (0x03, 0x00, "Shields", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "6" )
	PORT_DIPSETTING (   0x01, "7" )
	PORT_DIPSETTING (   0x02, "8" )
	PORT_DIPSETTING (   0x03, "9" )
	PORT_DIPNAME (0x0c, 0x04, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "Easy" )
	PORT_DIPSETTING (   0x04, "Moderate" )
	PORT_DIPSETTING (   0x08, "Hard" )
	PORT_DIPSETTING (   0x0c, "Hardest" )
	PORT_DIPNAME (0x30, 0x00, "Bonus Shields", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "0" )
	PORT_DIPSETTING (   0x10, "1" )
	PORT_DIPSETTING (   0x20, "2" )
	PORT_DIPSETTING (   0x30, "3" )
	PORT_DIPNAME (0x40, 0x00, "Attract Music", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "On" )
	PORT_DIPSETTING (   0x40, "Off" )
	PORT_DIPNAME (0x80, 0x80, "Game Mode", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "Freeze" )
	PORT_DIPSETTING (   0x80, "Normal" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME (0x03, 0x02, "Credits/Coin", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "Free Play" )
	PORT_DIPSETTING (   0x01, "2" )
	PORT_DIPSETTING (   0x02, "1" )
	PORT_DIPSETTING (   0x03, "1/2" )
	PORT_BIT ( 0xfc, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START	/* IN4 */
	PORT_ANALOG ( 0xff, 0x7f, IPT_AD_STICK_Y, 100, 0, 0, 255 )

	PORT_START	/* IN5 */
	PORT_ANALOG ( 0xff, 0x7f, IPT_AD_STICK_X, 100, 0, 0, 255 )
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
	0x00,0x00,0x00,
	0x00,0x00,0x01,
	0x00,0x01,0x00,
	0x00,0x01,0x01,
	0x01,0x00,0x00,
	0x01,0x00,0x01,
	0x01,0x01,0x00,
	0x01,0x01,0x01
};

/*********************************/

/* Memory regions relate to the ROM loader memory definitions later */
/* in this file.  Note that main machine has region 0      */
/* and I've not put a sound board in yet                  */

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		/* Main CPU */
		{
			CPU_M6809,
			1500000,					/* 1.5 Mhz CPU clock (Don't know what speed it should be) */
			0,							/* Memory region #0 */
			readmem,writemem,0,0,
			starwars_interrupt,6		/* Interrupt handler, and interrupts per frame (usually 1) */
			/* Starwars should be 183Hz interrupts */
			/* Increasing number of interrupts per frame speeds game up */
		},
		/* Sound CPU */
		{
			CPU_M6809 | CPU_AUDIO_CPU,
			1500000,					/* 1.5 Mhz CPU clock (Don't know what speed it should be) */
			2,							/* Memory region #2 */
			readmem2,writemem2,0,0,
			starwars_snd_interrupt,24	/* Interrupt handler, and interrupts per frame (usually 1) */
			/* Interrupts are to attempt to get */
			/* resolution for the PIA Timer */
			/* Approx. 2048 PIA clocks (@1.5 Mhz) */
		}

	},
	30,	/* Target Frames per Second */
	10,
	0,  /* Name of initialisation handler */

	/* video hardware */
	288, 224, { 0, 240, 0, 280 },
	gfxdecodeinfo,
	256,256, /* Number of colours, length of colour lookup table */
	sw_avg_init_colors,

	VIDEO_TYPE_VECTOR,
	0,							/* Handler to initialise video handware */
	avg_start_starwars,			/* Start video hardware */
	avg_stop,					/* Stop video hardware */
	avg_screenrefresh,			/* Do a screen refresh */

	/* sound hardware */
	0,
	0,							/* Initialise audio hardware */
	starwars_sh_start,			/* Start audio  */
	pokey_sh_stop,				/* Stop audio   */
	pokey_sh_update				/* Update audio */
};



/***************************************************************************

  Game driver

***************************************************************************/

ROM_START( starwars_rom )

/* ROM_REGION(x) allocates a chunk of memory of size x to be used as ROM */
/* ROM_LOAD("bibble",a,b) loads ROM file called 'bibble' starting at offset 'a' */
/* within this chunk, and of length 'b' */

/* This is the first ROM region defined, i.e. memory region 0 */
#if(EMPIRE==0)

	ROM_REGION(0x12000)     /* 2 64k ROM spaces */
	ROM_LOAD( "136021.105", 0x3000, 0x1000, 0x46f9a4f9 ) /* 3000-3fff is 4k vector rom */
	ROM_LOAD( "136021.114", 0x6000, 0x2000, 0x8fddcf2b )   /* ROM 0 bank pages 0 and 1 */
	ROM_CONTINUE(          0x10000, 0x2000 )
	ROM_LOAD( "136021.102", 0x8000, 0x2000, 0x7ad298ba ) /*  8k ROM 1 bank */
	ROM_LOAD( "136021.203", 0xa000, 0x2000, 0x58c5d6a1 ) /*  8k ROM 2 bank */
	ROM_LOAD( "136021.104", 0xc000, 0x2000, 0x38d07312 ) /*  8k ROM 3 bank */
	ROM_LOAD( "136021.206", 0xe000, 0x2000, 0x199428a2 ) /*  8k ROM 4 bank */

	/* Load the Mathbox PROM's temporarily into the Vector RAM area */
	/* During initialisation they will be converted into useable form */
	/* and stored elsewhere. */
	ROM_LOAD( "136021.110",0x0000,0x0400, 0x3757bfd5 ) /* PROM 0 */
	ROM_LOAD( "136021.111",0x0400,0x0400, 0x465db5db ) /* PROM 1 */
	ROM_LOAD( "136021.112",0x0800,0x0400, 0xcb5ab0da ) /* PROM 2 */
	ROM_LOAD( "136021.113",0x0c00,0x0400, 0x4d20bcd2 ) /* PROM 3 */

	ROM_REGION(0x0400)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "136021.113",0x0000,0x0400, 0x4d20bcd2 ) /* not needed - could be removed */

	/* Sound ROMS */
	ROM_REGION(0x10000)     /* Really only 32k, but it looks like 64K */

	ROM_LOAD("136021.107",0x4000,0x2000, 0xc0006994) /* Sound ROM 0 */
	ROM_RELOAD(           0xc000,0x2000) /* Copied again for */
	ROM_LOAD("136021.208",0x6000,0x2000, 0xa000bc38) /* Sound ROM 0 */
	ROM_RELOAD(           0xe000,0x2000) /* proper int vecs */
ROM_END
#endif

/* ****************** EMPIRE *********************************** */
/* This doesn't work */
#if(EMPIRE==1)
        ROM_REGION(0x14000)     /* 64k for code, and another 16k on the end for banked ROMS */

        ROM_OBSOLETELOAD( "136031.111", 0x3000, 0x1000 )    /* 3000-3fff is 4k vector rom */

/* Expansion board location   ROM_LOAD( "136021.102", 0x8000, 0x2000 )   8k ROM 1 bank */
        ROM_OBSOLETELOAD( "136031.102", 0xa000, 0x2000 ) /*  8k ROM 2 bank */
        ROM_OBSOLETELOAD( "136021.103", 0xc000, 0x2000 ) /*  8k ROM 3 bank */
        ROM_OBSOLETELOAD( "136031.104", 0xe000, 0x2000 ) /*  8k ROM 4 bank */
        ROM_OBSOLETELOAD( "136031.101", 0x10000, 0x4000 )   /* Paged ROM 0 */
ROM_END
#endif


/* NovRAM Load/Save.  In-game DIP switch setting, and Hi-scores */

#if(EMPIRE==0)
static int novram_load(void)
{
/* get RAM pointer (if game is multiCPU, we can't assume the global */
/* RAM pointer is pointing to the right place) */
unsigned char *RAM = Machine->memory_region[0];

	void *f;
	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	{
		osd_fread(f,&RAM[0x4500],256);
		osd_fclose(f);
	}
	return 1;
}

static void novram_save(void)
{
	void *f;
	/* get RAM pointer (if game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x4500],256);
		osd_fclose(f);
	}
}
#endif
#if (EMPIRE==1)
static int novram_load(const char *name)
   {
   return 0;
   }
static void novram_save(const char *name)
   {
   }
#endif



struct GameDriver starwars_driver =
{
	"Star Wars",
	"starwars",
	"STEVE BAINES\nBRAD OLIVER\nFRANK PALAZZOLO",
	&machine_driver,

	starwars_rom,
	translate_proms, 0,  /* ROM decryption, Opcode decryption */
	0,     /* Sample Array (optional) */

	0/*TBR*/, input_ports, 0/*TBR*/, 0/*TBR*/, 0/*TBR*/,
	color_prom, /* Colour PROM */
	0,          /* palette */
	0,          /* colourtable */
	ORIENTATION_DEFAULT,

	novram_load, novram_save /* Highscore load, save */
};

