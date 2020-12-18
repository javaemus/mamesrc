/***************************************************************************

Atari Flyball Driver

***************************************************************************/

#include "driver.h"

extern VIDEO_START( flyball );
extern VIDEO_UPDATE( flyball );

extern UINT8 flyball_pitcher_pic;
extern UINT8 flyball_pitcher_vert;
extern UINT8 flyball_pitcher_horz;
extern UINT8 flyball_ball_vert;
extern UINT8 flyball_ball_horz;

extern UINT8* flyball_playfield_ram;

static UINT8 flyball_potmask;
static UINT8 flyball_potsense;

static UINT8* flyball_zero_page;


static void flyball_joystick_callback(int potsense)
{
	if (potsense & ~flyball_potmask)
	{
		cpu_set_irq_line(0, 0, PULSE_LINE);
	}

	flyball_potsense |= potsense;
}


static void flyball_quarter_callback(int scanline)
{
	int potsense[64], i;

	memset(potsense, 0, sizeof potsense);

	potsense[readinputport(1)] |= 1;
	potsense[readinputport(2)] |= 2;
	potsense[readinputport(3)] |= 4;
	potsense[readinputport(4)] |= 8;

	for (i = 0; i < 64; i++)
	{
		if (potsense[i] != 0)
		{
			timer_set(cpu_getscanlinetime(scanline + i), potsense[i], flyball_joystick_callback);
		}
	}

	scanline += 0x40;
	scanline &= 0xff;

	timer_set(cpu_getscanlinetime(scanline), scanline, flyball_quarter_callback);

	flyball_potsense = 0;
	flyball_potmask = 0;
}


static MACHINE_INIT( flyball )
{
	int i;

	/* address bits 0 through 8 are inverted */

	UINT8* ROM = memory_region(REGION_CPU1);

	for (i = 0; i < 0x1000; i++)
	{
		ROM[0x1000 + i] = ROM[0x10000 + (i ^ 0x1ff)];
		ROM[0xF000 + i] = ROM[0x10000 + (i ^ 0x1ff)];
	}

	timer_set(cpu_getscanlinetime(0), 0, flyball_quarter_callback);

	flyball_zero_page = auto_malloc(0x100);
}


/* two physical buttons (start game and stop runner) share the same port bit */

READ_HANDLER( flyball_input_r )
{
	return readinputport(0) & readinputport(5);
}

READ_HANDLER( flyball_scanline_r )
{
	return cpu_getscanline() & 0x3f;
}

READ_HANDLER( flyball_potsense_r )
{
	return flyball_potsense & ~flyball_potmask;
}

READ_HANDLER( flyball_ram_r )
{
	return flyball_zero_page[offset & 0xff];
}

WRITE_HANDLER( flyball_potmask_w )
{
	flyball_potmask |= data & 0xf;
}

WRITE_HANDLER( flyball_ram_w )
{
	flyball_zero_page[offset & 0xff] = data;
}

WRITE_HANDLER( flyball_pitcher_pic_w )
{
	flyball_pitcher_pic = data & 0xf;
}

WRITE_HANDLER( flyball_ball_vert_w )
{
	flyball_ball_vert = data;
}

WRITE_HANDLER( flyball_ball_horz_w )
{
	flyball_ball_horz = data;
}

WRITE_HANDLER( flyball_pitcher_vert_w )
{
	flyball_pitcher_vert = data;
}

WRITE_HANDLER( flyball_pitcher_horz_w )
{
	flyball_pitcher_horz = data;
}

WRITE_HANDLER( flyball_misc_w )
{
	int bit = ~data & 1;

	switch (offset)
	{
	case 0:
		set_led_status(0, bit);
		break;
	case 1:
		/* crowd very loud */
		break;
	case 2:
		/* footstep off-on */
		break;
	case 3:
		/* crowd off-on */
		break;
	case 4:
		/* crowd soft-loud */
		break;
	case 5:
		/* bat hit */
		break;
	}
}


static MEMORY_READ_START( flyball_readmem )
	{ 0x0000, 0x01ff, flyball_ram_r },
	{ 0x0800, 0x0800, MRA_NOP },
	{ 0x0802, 0x0802, flyball_scanline_r },
	{ 0x0803, 0x0803, flyball_potsense_r },
	{ 0x0b00, 0x0b00, flyball_input_r },
	{ 0x1000, 0x1fff, MRA_ROM }, /* program */
	{ 0xf000, 0xffff, MRA_ROM }, /* program mirror */
MEMORY_END

static MEMORY_WRITE_START( flyball_writemem )
	{ 0x0000, 0x01ff, flyball_ram_w },
	{ 0x0800, 0x0800, MWA_NOP },
	{ 0x0801, 0x0801, flyball_pitcher_pic_w },
	{ 0x0804, 0x0804, flyball_ball_vert_w },
	{ 0x0805, 0x0805, flyball_ball_horz_w },
	{ 0x0806, 0x0806, flyball_pitcher_vert_w },
	{ 0x0807, 0x0807, flyball_pitcher_horz_w },
	{ 0x0900, 0x0900, flyball_potmask_w },
	{ 0x0a00, 0x0a07, flyball_misc_w },
	{ 0x0d00, 0x0eff, MWA_RAM, &flyball_playfield_ram },
	{ 0x1000, 0x1fff, MWA_ROM }, /* program */
	{ 0xf000, 0xffff, MWA_ROM }, /* program mirror */
MEMORY_END


INPUT_PORTS_START( flyball )
	PORT_START /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING( 0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING( 0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING( 0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x40, 0x40, "Innings Per Game" )
	PORT_DIPSETTING( 0x00, "1" )
	PORT_DIPSETTING( 0x40, "2" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x80, DEF_STR( On ) )

	PORT_START /* IN1 */
	PORT_ANALOG( 0x3f, 0x20, IPT_AD_STICK_Y | IPF_PLAYER2, 50, 10, 1, 63)

	PORT_START /* IN2 */
	PORT_ANALOG( 0x3f, 0x20, IPT_AD_STICK_X | IPF_PLAYER2, 50, 10, 1, 63)

	PORT_START /* IN3 */
	PORT_ANALOG( 0x3f, 0x20, IPT_AD_STICK_Y | IPF_PLAYER1, 50, 10, 1, 63)

	PORT_START /* IN4 */
	PORT_ANALOG( 0x3f, 0x20, IPT_AD_STICK_X | IPF_PLAYER1, 50, 10, 1, 63)

	PORT_START /* IN5 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xFE, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


static struct GfxLayout flyball_tiles_layout =
{
	8, 16,    /* width, height */
	128,      /* total         */
	1,        /* planes        */
	{ 0 },    /* plane offsets */
	{
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
	},
	{
		0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38,
		0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78
	},
	0x80      /* increment */
};

static struct GfxLayout flyball_sprites_layout =
{
	16, 16,   /* width, height */
	16,       /* total         */
	1,        /* planes        */
	{ 0 },    /* plane offsets */
	{
		0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
		0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF
	},
	{
		0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,
		0x80, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0
	},
	0x100     /* increment */
};

static struct GfxDecodeInfo flyball_gfx_decode_info[] =
{
	{ REGION_GFX1, 0, &flyball_tiles_layout, 0, 2 },
	{ REGION_GFX2, 0, &flyball_sprites_layout, 2, 2 },
	{ -1 } /* end of array */
};


PALETTE_INIT( flyball )
{
	static UINT8 palette_source[] =
	{
		0x3F, 0x3F, 0x3F,   /* tiles, ball */
		0xFF, 0xFF, 0xFF,
		0xFF ,0xFF, 0xFF,   /* sprites */
		0x00, 0x00, 0x00
	};

	memcpy(palette, palette_source, sizeof palette_source);
}


static MACHINE_DRIVER_START( flyball )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 12096000 / 16)
	MDRV_CPU_MEMORY(flyball_readmem, flyball_writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse, 1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION((int) ((22. * 1000000) / (262. * 60) + 0.5))

	MDRV_MACHINE_INIT(flyball)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 240)
	MDRV_VISIBLE_AREA(0, 255, 0, 239)
	MDRV_GFXDECODE(flyball_gfx_decode_info)
	MDRV_PALETTE_LENGTH(4)

	MDRV_PALETTE_INIT(flyball)
	MDRV_VIDEO_UPDATE(flyball)
	MDRV_VIDEO_START(flyball)

	/* sound hardware */
MACHINE_DRIVER_END


ROM_START( flyball )
	ROM_REGION( 0x11000, REGION_CPU1, 0 )                  /* program */
	ROM_LOAD( "6129.d5", 0x10000, 0x0200, 0x17EDA069 )
	ROM_LOAD( "6130.f5", 0x10200, 0x0200, 0xA756955B )
	ROM_LOAD( "6131.h5", 0x10400, 0x0200, 0xA9C7E858 )
	ROM_LOAD( "6132.j5", 0x10600, 0x0200, 0x31FEFD8A )
	ROM_LOAD( "6133.k5", 0x10800, 0x0200, 0x6FDB09B1 )
	ROM_LOAD( "6134.m5", 0x10A00, 0x0200, 0x7B526C73 )
	ROM_LOAD( "6135.n5", 0x10C00, 0x0200, 0xB352CB51 )
	ROM_LOAD( "6136.r5", 0x10E00, 0x0200, 0x1622D890 )

	ROM_REGION( 0x0C00, REGION_GFX1, ROMREGION_DISPOSE )   /* tiles */
	ROM_LOAD( "6142.l2", 0x0000, 0x0200, 0x65650CFA )
	ROM_LOAD( "6139.j2", 0x0200, 0x0200, 0xA5D1358E )
	ROM_LOAD( "6141.m2", 0x0400, 0x0200, 0x98B5F803 )
	ROM_LOAD( "6140.k2", 0x0600, 0x0200, 0x66AEEC61 )

	ROM_REGION( 0x0400, REGION_GFX2, ROMREGION_DISPOSE )   /* sprites */
	ROM_LOAD16_BYTE( "6137.e2", 0x0000, 0x0200, 0x68961FDA )
	ROM_LOAD16_BYTE( "6138.f2", 0x0001, 0x0200, 0xAAB314F6 )
ROM_END


GAMEX( 1976, flyball, 0, flyball, flyball, 0, 0, "Atari", "Flyball", GAME_NO_SOUND )
