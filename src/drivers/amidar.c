/***************************************************************************

Amidar memory map (preliminary)

0000-3fff ROM (Amidar US version and Turtles: 0000-4fff)
8000-87ff RAM
9000-93ff Video RAM
9800-983f Screen attributes
9840-985f sprites

read:
a800      watchdog reset
b000      IN0
b010      IN1
b020      IN2
b820      DSW (not Turtles)
see the input_ports definition below for details on the input bits

write:
a008      interrupt enable
a010/a018 flip screen X & Y (I don't know which is which)
a030      coin counter A
a038      coin counter B
b800      To AY-3-8910 port A (commands for the audio CPU)
b810      bit 3 = interrupt trigger on audio CPU


SOUND BOARD:
0000-1fff ROM
8000-83ff RAM

write:
9000      ?
9080      ?

I/0 ports:
read:
20      8910 #2  read
80      8910 #1  read

write
10      8910 #2  control
20      8910 #2  write
40      8910 #1  control
80      8910 #1  write

interrupts:
interrupt mode 1 triggered by the main CPU

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



extern unsigned char *amidar_attributesram;
void amidar_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void amidar_flipx_w(int offset,int data);
void amidar_flipy_w(int offset,int data);
void amidar_attributes_w(int offset,int data);
void amidar_vh_screenrefresh(struct osd_bitmap *bitmap);

int amidar_sh_interrupt(void);
int amidar_sh_start(void);



static struct MemoryReadAddress amidar_readmem[] =
{
	{ 0x0000, 0x4fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x9000, 0x93ff, MRA_RAM },
	{ 0x9800, 0x985f, MRA_RAM },
	{ 0xa800, 0xa800, MRA_NOP },
	{ 0xb000, 0xb000, input_port_0_r },	/* IN0 */
	{ 0xb010, 0xb010, input_port_1_r },	/* IN1 */
	{ 0xb020, 0xb020, input_port_2_r },	/* IN2 */
	{ 0xb820, 0xb820, input_port_3_r },	/* DSW */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x4fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x9000, 0x93ff, videoram_w, &videoram, &videoram_size },
	{ 0x9800, 0x983f, amidar_attributes_w, &amidar_attributesram },
	{ 0x9840, 0x985f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x9860, 0x987f, MWA_NOP },
	{ 0xa008, 0xa008, interrupt_enable_w },
	{ 0xa010, 0xa010, amidar_flipx_w },
	{ 0xa018, 0xa018, amidar_flipy_w },
	{ 0xa030, 0xa030, MWA_NOP },
	{ 0xa038, 0xa038, MWA_NOP },
	{ 0xb800, 0xb800, sound_command_w },
	{ 0xb810, 0xb810, MWA_NOP },
	{ -1 }	/* end of table */
};



static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x8000, 0x83ff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x8000, 0x83ff, MWA_RAM },
{ 0x9000, 0x9000, MWA_NOP },
{ 0x9080, 0x9080, MWA_NOP },
	{ -1 }	/* end of table */
};



static struct IOReadPort sound_readport[] =
{
	{ 0x80, 0x80, AY8910_read_port_0_r },
	{ 0x20, 0x20, AY8910_read_port_1_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x40, 0x40, AY8910_control_port_0_w },
	{ 0x80, 0x80, AY8910_write_port_0_w },
	{ 0x10, 0x10, AY8910_control_port_1_w },
	{ 0x20, 0x20, AY8910_write_port_1_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( amidar_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably space for button 2 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "255" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably space for player 2 button 2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x02, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x04, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "30000 70000" )
	PORT_DIPSETTING(    0x04, "50000 80000" )
	PORT_DIPNAME( 0x08, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x08, "Cocktail" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_DIPNAME( 0x20, 0x00, "unknown1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_DIPNAME( 0x80, 0x00, "unknown2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x0f, 0x0f, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0a, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x08, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x0c, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0x0e, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x07, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x06, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0x0b, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0d, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x09, "1 Coin/7 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0xf0, 0xf0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0xa0, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x20, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x80, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0xc0, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0xe0, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x70, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x60, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0xb0, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x30, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xd0, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x50, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x90, "1 Coin/7 Credits" )
	PORT_DIPSETTING(    0x00, "Disable All Coins" )
INPUT_PORTS_END

/* similar to Amidar, dip swtiches are different and port 3, which in Amidar */
/* selects coins per credit, is not used. */
INPUT_PORTS_START( turtles_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably space for button 2 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "126" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably space for player 2 button 2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x06, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "A 1/1 B 2/1 C 1/1" )
	PORT_DIPSETTING(    0x02, "A 1/2 B 1/1 C 1/2" )
	PORT_DIPSETTING(    0x04, "A 1/3 B 3/1 C 1/3" )
	PORT_DIPSETTING(    0x06, "A 1/4 B 4/1 C 1/4" )
	PORT_DIPNAME( 0x08, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x08, "Cocktail" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_DIPNAME( 0x20, 0x00, "unknown1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_DIPNAME( 0x80, 0x00, "unknown2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

/* same as Turtles, but dip switches are different. */
INPUT_PORTS_START( turpin_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably space for button 2 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "6" )
	PORT_DIPSETTING(    0x03, "126" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably space for player 2 button 2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x06, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x04, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x08, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x08, "Cocktail" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_DIPNAME( 0x20, 0x00, "unknown1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_DIPNAME( 0x80, 0x00, "unknown2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 64*16*16 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,     0, 8 },
	{ 1, 0x0000, &spritelayout,   0, 8 },
	{ -1 } /* end of array */
};



static unsigned char amidar_color_prom[] =
{
	/* palette */
	0x00,0x07,0xC0,0xB6,0x00,0x38,0xC5,0x67,0x00,0x30,0x07,0x3F,0x00,0x07,0x30,0x3F,
	0x00,0x3F,0x30,0x07,0x00,0x38,0x67,0x3F,0x00,0xFF,0x07,0xDF,0x00,0xF8,0x07,0xFF
};



static unsigned char turtles_color_prom[] =
{
	/* palette */
	0x00,0xC0,0x57,0xFF,0x00,0x66,0xF2,0xFE,0x00,0x2D,0x12,0xBF,0x00,0x2F,0x7D,0xB8,
	0x00,0x72,0xD2,0x06,0x00,0x94,0xFF,0xE8,0x00,0x54,0x2F,0xF6,0x00,0x24,0xBF,0xC6
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz */
			0,
			amidar_readmem,writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			2100000,	/* 2 Mhz?????? */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			amidar_sh_interrupt,10
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	32,32,
	amidar_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	amidar_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	amidar_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( amidar_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "amidarus.2c", 0x0000, 0x1000, 0xd04355e9 )
	ROM_LOAD( "amidarus.2e", 0x1000, 0x1000, 0xaaf3b9f9 )
	ROM_LOAD( "amidarus.2f", 0x2000, 0x1000, 0x65dbb08f )
	ROM_LOAD( "amidarus.2h", 0x3000, 0x1000, 0x42647e74 )
	ROM_LOAD( "amidarus.2j", 0x4000, 0x1000, 0x80994cd9 )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "amidarus.5f", 0x0000, 0x0800, 0xa114cb34 )
	ROM_LOAD( "amidarus.5h", 0x0800, 0x0800, 0x0d1cafd2 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "amidarus.5c", 0x0000, 0x1000, 0x2fd961f7 )
	ROM_LOAD( "amidarus.5d", 0x1000, 0x1000, 0xfa4bd265 )
ROM_END

ROM_START( amidarjp_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "amidar.2c", 0x0000, 0x1000, 0x061cf902 )
	ROM_LOAD( "amidar.2e", 0x1000, 0x1000, 0x68948086 )
	ROM_LOAD( "amidar.2f", 0x2000, 0x1000, 0x89747538 )
	ROM_LOAD( "amidar.2h", 0x3000, 0x1000, 0xa1bc1266 )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "amidar.5f", 0x0000, 0x0800, 0x1e31bc69 )
	ROM_LOAD( "amidar.5h", 0x0800, 0x0800, 0x8b6560db )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "amidar.5c", 0x0000, 0x1000, 0xcdfaa8be )
	ROM_LOAD( "amidar.5d", 0x1000, 0x1000, 0xc84cdcfc )
ROM_END

ROM_START( amigo_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "2732.a1", 0x0000, 0x1000, 0x43d647a4 )
	ROM_LOAD( "2732.a2", 0x1000, 0x1000, 0x3f1c500a )
	ROM_LOAD( "2732.a3", 0x2000, 0x1000, 0xa698c12c )
	ROM_LOAD( "2732.a4", 0x3000, 0x1000, 0x3c6d621d )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "2716.a6", 0x0000, 0x0800, 0xe09ed6c8 )
	ROM_LOAD( "2716.a5", 0x0800, 0x0800, 0x3355a22f )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "2732.a7", 0x0000, 0x1000, 0x2fd961f7 )
	ROM_LOAD( "2732.a8", 0x1000, 0x1000, 0xfa4bd265 )
ROM_END

ROM_START( turtles_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "turt_vid.2c", 0x0000, 0x1000, 0xd64b683b )
	ROM_LOAD( "turt_vid.2e", 0x1000, 0x1000, 0x694c07b6 )
	ROM_LOAD( "turt_vid.2f", 0x2000, 0x1000, 0x07385210 )
	ROM_LOAD( "turt_vid.2h", 0x3000, 0x1000, 0xb71c1118 )
	ROM_LOAD( "turt_vid.2j", 0x4000, 0x1000, 0x2f33ea7b )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "turt_vid.5h", 0x0000, 0x0800, 0x077a3a46 )
	ROM_LOAD( "turt_vid.5f", 0x0800, 0x0800, 0xd1107d84 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "turt_snd.5c", 0x0000, 0x1000, 0x90ecc748 )
	ROM_LOAD( "turt_snd.5d", 0x1000, 0x1000, 0xad8bffad )
ROM_END

ROM_START( turpin_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "M1", 0x0000, 0x1000, 0x805f1577 )
	ROM_LOAD( "M2", 0x1000, 0x1000, 0x694f9789 )
	ROM_LOAD( "M3", 0x2000, 0x1000, 0x1b08134e )
	ROM_LOAD( "M4", 0x3000, 0x1000, 0xb71c1118 )
	ROM_LOAD( "M5", 0x4000, 0x1000, 0x112be471 )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "C1", 0x0000, 0x0800, 0x077a3a46 )
	ROM_LOAD( "C2", 0x0800, 0x0800, 0xd1107d84 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "D1", 0x0000, 0x1000, 0x90ecc748 )
	ROM_LOAD( "D2", 0x1000, 0x1000, 0xad8bffad )
ROM_END



static int amidar_hiload(const char *name)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x8200],"\x00\x00\x01",3) == 0 &&
			memcmp(&RAM[0x821b],"\x00\x00\x01",3) == 0)
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			fread(&RAM[0x8200],1,3*10,f);
			RAM[0x80a8] = RAM[0x8200];
			RAM[0x80a9] = RAM[0x8201];
			RAM[0x80aa] = RAM[0x8202];
			fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void amidar_hisave(const char *name)
{
	FILE *f;
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	if ((f = fopen(name,"wb")) != 0)
	{
		fwrite(&RAM[0x8200],1,3*10,f);
		fclose(f);
	}
}



struct GameDriver amidar_driver =
{
	"Amidar (US version)",
	"amidar",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (MAME driver)\nAlan J. McCormick (color info)",
	&machine_driver,

	amidar_rom,
	0, 0,
	0,

	0/*TBR*/,amidar_input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	amidar_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	amidar_hiload, amidar_hisave
};

struct GameDriver amidarjp_driver =
{
	"Amidar (Japanese version)",
	"amidarjp",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (MAME driver)\nAlan J. McCormick (color info)",
	&machine_driver,

	amidarjp_rom,
	0, 0,
	0,

	0/*TBR*/,amidar_input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	amidar_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	amidar_hiload, amidar_hisave
};

struct GameDriver amigo_driver =
{
	"Amigo (Amidar US bootleg)",
	"amigo",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (MAME driver)\nAlan J. McCormick (color info)\nDavid Winter (game driver)",
	&machine_driver,

	amigo_rom,
	0, 0,
	0,

	0/*TBR*/,amidar_input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	amidar_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	amidar_hiload, amidar_hisave
};

struct GameDriver turtles_driver =
{
	"Turtles",
	"turtles",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (MAME driver)\nAlan J. McCormick (color info)",
	&machine_driver,

	turtles_rom,
	0, 0,
	0,

	0/*TBR*/,turtles_input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	turtles_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	0, 0
};

struct GameDriver turpin_driver =
{
	"Turpin",
	"turpin",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (MAME driver)\nAlan J. McCormick (color info)",
	&machine_driver,

	turpin_rom,
	0, 0,
	0,

	0/*TBR*/,turpin_input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	turtles_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	0, 0
};
