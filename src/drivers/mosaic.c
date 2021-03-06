/***************************************************************************

Mosaic (c) 1990 Space

Notes:
- the ROM OK / RAM OK message in service mode is fake: ROM and RAM are not tested.

***************************************************************************/

#include "driver.h"
#include "cpu/z180/z180.h"


extern data8_t *mosaic_fgvideoram;
extern data8_t *mosaic_bgvideoram;
WRITE_HANDLER( mosaic_fgvideoram_w );
WRITE_HANDLER( mosaic_bgvideoram_w );
VIDEO_START( mosaic );
VIDEO_UPDATE( mosaic );



static int prot_val;

static WRITE_HANDLER( protection_w )
{
	if ((data & 0x80) == 0)
	{
		/* simply increment given value */
		prot_val = (data + 1) << 8;
	}
	else
	{
		static int jumptable[] =
		{
			0x02be, 0x0314, 0x0475, 0x0662, 0x0694, 0x08f3, 0x0959, 0x096f,
			0x0992, 0x09a4, 0x0a50, 0x0d69, 0x0eee, 0x0f98, 0x1040, 0x1075,
			0x10d8, 0x18b4, 0x1a27, 0x1a4a, 0x1ac6, 0x1ad1, 0x1ae2, 0x1b68,
			0x1c95, 0x1fd5, 0x20fc, 0x212d, 0x213a, 0x21b6, 0x2268, 0x22f3,
			0x231a, 0x24bb, 0x286b, 0x295f, 0x2a7f, 0x2fc6, 0x3064, 0x309f,
			0x3118, 0x31e1, 0x32d0, 0x35f7, 0x3687, 0x38ea, 0x3b86, 0x3c9a,
			0x411f, 0x473f
		};

		prot_val = jumptable[data & 0x7f];
	}
}

static READ_HANDLER( protection_r )
{
	int res = (prot_val >> 8) & 0xff;

	prot_val <<= 8;

	return res;
}



static MEMORY_READ_START( readmem )
	{ 0x00000, 0x0ffff, MRA_ROM },
	{ 0x20000, 0x21fff, MRA_RAM },
	{ 0x22000, 0x23fff, MRA_RAM },
	{ 0x24000, 0x241ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x00000, 0x0ffff, MWA_ROM },
	{ 0x20000, 0x21fff, MWA_RAM },
	{ 0x22000, 0x22fff, mosaic_bgvideoram_w, &mosaic_bgvideoram },
	{ 0x23000, 0x23fff, mosaic_fgvideoram_w, &mosaic_fgvideoram },
	{ 0x24000, 0x241ff, paletteram_xRRRRRGGGGGBBBBB_w, &paletteram },
MEMORY_END

static PORT_READ_START( readport )
	{ 0x70, 0x70, YM2203_status_port_0_r },
	{ 0x71, 0x71, YM2203_read_port_0_r },
	{ 0x72, 0x72, protection_r },
	{ 0x74, 0x74, input_port_0_r },
	{ 0x76, 0x76, input_port_1_r },
PORT_END

static PORT_WRITE_START( writeport )
	{ 0x00, 0x3f, IOWP_NOP },	/* Z180 internal registers */
	{ 0x70, 0x70, YM2203_control_port_0_w },
	{ 0x71, 0x71, YM2203_write_port_0_w },
	{ 0x72, 0x72, protection_w },
PORT_END



INPUT_PORTS_START( mosaic )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* DSW1 */
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x40, 0x00, "Bombs" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x40, "5" )
	PORT_DIPNAME( 0x20, 0x20, "Speed" )
	PORT_DIPSETTING(    0x20, "Low" )
	PORT_DIPSETTING(    0x00, "High" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x02, 0x00, "Music" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x00, "Sound" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,4),
	8,
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{	RGN_FRAC(3,4)+0, RGN_FRAC(2,4)+0, RGN_FRAC(1,4)+0, RGN_FRAC(0,4)+0,
		RGN_FRAC(3,4)+8, RGN_FRAC(2,4)+8, RGN_FRAC(1,4)+8, RGN_FRAC(0,4)+8 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 1 },
	{ REGION_GFX2, 0, &charlayout, 0, 1 },
	{ -1 } /* end of array */
};



static struct YM2203interface ym2203_interface =
{
	1,
	3000000,	/* ??? */
	{ YM2203_VOL(50,50) },
	{ input_port_2_r },
	{ 0 },
	{ 0	},
	{ 0 },
	{ 0 }
};



static MACHINE_DRIVER_START( mosaic )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z180, 7000000)	/* ??? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(8*8, 48*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(mosaic)
	MDRV_VIDEO_UPDATE(mosaic)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( mosaic )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 1024k for Z180 address space */
	ROM_LOAD( "mosaic.9", 0x00000, 0x10000, 0x5794dd39 )

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mosaic.1", 0x00000, 0x10000, 0x05f4cc70 )
	ROM_LOAD( "mosaic.2", 0x10000, 0x10000, 0x78907875 )
	ROM_LOAD( "mosaic.3", 0x20000, 0x10000, 0xf81294cd )
	ROM_LOAD( "mosaic.4", 0x30000, 0x10000, 0xfff72536 )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mosaic.5", 0x00000, 0x10000, 0x28513fbf )
	ROM_LOAD( "mosaic.6", 0x10000, 0x10000, 0x1b8854c4 )
	ROM_LOAD( "mosaic.7", 0x20000, 0x10000, 0x35674ac2 )
	ROM_LOAD( "mosaic.8", 0x30000, 0x10000, 0x6299c376 )
ROM_END

ROM_START( mosaica )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 1024k for Z180 address space */
	ROM_LOAD( "mosaic_9.a02", 0x00000, 0x10000, 0xecb4f8aa )

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mosaic.1", 0x00000, 0x10000, 0x05f4cc70 )
	ROM_LOAD( "mosaic.2", 0x10000, 0x10000, 0x78907875 )
	ROM_LOAD( "mosaic.3", 0x20000, 0x10000, 0xf81294cd )
	ROM_LOAD( "mosaic.4", 0x30000, 0x10000, 0xfff72536 )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mosaic.5", 0x00000, 0x10000, 0x28513fbf )
	ROM_LOAD( "mosaic.6", 0x10000, 0x10000, 0x1b8854c4 )
	ROM_LOAD( "mosaic.7", 0x20000, 0x10000, 0x35674ac2 )
	ROM_LOAD( "mosaic.8", 0x30000, 0x10000, 0x6299c376 )
ROM_END



GAME( 1990, mosaic,  0,      mosaic, mosaic, 0, ROT0, "Space", "Mosaic" )
GAME( 1990, mosaica, mosaic, mosaic, mosaic, 0, ROT0, "Space (Fuuki license)", "Mosaic (Fuuki)" )
