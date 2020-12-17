/***************************************************************************

Naughty Boy driver by Sal and John Bugliarisi.
This driver is based largely on MAME's Phoenix driver, since Naughty Boy runs
on similar hardware as Phoenix. Phoenix driver provided by Brad Oliver.
Thanks to Richard Davies for his Phoenix emulator source.


Naughty Boy memory map

0000-3fff 16Kb Program ROM
4000-7fff 1Kb Work RAM (mirrored)
8000-87ff 2Kb Video RAM Charset A (lower priority, mirrored)
8800-8fff 2Kb Video RAM Charset b (higher priority, mirrored)
9000-97ff 2Kb Video Control write-only (mirrored)
9800-9fff 2Kb Video Scroll Register (mirrored)
a000-a7ff 2Kb Sound Control A (mirrored)
a800-afff 2Kb Sound Control B (mirrored)
b000-b7ff 2Kb 8bit Game Control read-only (mirrored)
b800-bfff 1Kb 8bit Dip Switch read-only (mirrored)
c000-0000 16Kb Unused

memory mapped ports:

read-only:
b000-b7ff IN
b800-bfff DSW


Naughty Boy Switch Settings
(C)1982 Cinematronics

 --------------------------------------------------------
|Option |Factory|Descrpt| 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
 ------------------------|-------------------------------
|Lives  |       |2      |on |on |   |   |   |   |   |   |
 ------------------------ -------------------------------
|       |   X   |3      |off|on |   |   |   |   |   |   |
 ------------------------ -------------------------------
|       |       |4      |on |off|   |   |   |   |   |   |
 ------------------------ -------------------------------
|       |       |5      |off|off|   |   |   |   |   |   |
 ------------------------ -------------------------------
|Extra  |       |10000  |   |   |on |on |   |   |   |   |
 ------------------------ -------------------------------
|       |   X   |30000  |   |   |off|on |   |   |   |   |
 ------------------------ -------------------------------
|       |       |50000  |   |   |on |off|   |   |   |   |
 ------------------------ -------------------------------
|       |       |70000  |   |   |off|off|   |   |   |   |
 ------------------------ -------------------------------
|Credits|       |2c, 1p |   |   |   |   |on |on |   |   |
 ------------------------ -------------------------------
|       |   X   |1c, 1p |   |   |   |   |off|on |   |   |
 ------------------------ -------------------------------
|       |       |1c, 2p |   |   |   |   |on |off|   |   |
 ------------------------ -------------------------------
|       |       |4c, 3p |   |   |   |   |off|off|   |   |
 ------------------------ -------------------------------
|Dffclty|   X   |Easier |   |   |   |   |   |   |on |   |
 ------------------------ -------------------------------
|       |       |Harder |   |   |   |   |   |   |off|   |
 ------------------------ -------------------------------
| Type  |       |Upright|   |   |   |   |   |   |   |on |
 ------------------------ -------------------------------
|       |       |Cktail |   |   |   |   |   |   |   |off|
 ------------------------ -------------------------------

*
* Pop Flamer
*

Pop Flamer appears to run on identical hardware as Naughty Boy.
The dipswitches are even identical. Spooky.

                        1   2   3   4   5   6   7   8
-------------------------------------------------------
Number of Mr. Mouse 2 |ON |ON |   |   |   |   |   |   |
                    3 |OFF|ON |   |   |   |   |   |   |
                    4 |ON |OFF|   |   |   |   |   |   |
                    5 |OFF|OFF|   |   |   |   |   |   |
-------------------------------------------------------
Extra Mouse    10,000 |   |   |ON |ON |   |   |   |   |
               30,000 |   |   |OFF|ON |   |   |   |   |
               50,000 |   |   |ON |OFF|   |   |   |   |
               70,000 |   |   |OFF|OFF|   |   |   |   |
-------------------------------------------------------
Credit  2 coin 1 play |   |   |   |   |ON |ON |   |   |
        1 coin 1 play |   |   |   |   |OFF|ON |   |   |
        1 coin 2 play |   |   |   |   |ON |OFF|   |   |
        1 coin 3 play |   |   |   |   |OFF|OFF|   |   |
-------------------------------------------------------
Skill          Easier |   |   |   |   |   |   |ON |   |
               Harder |   |   |   |   |   |   |OFF|   |
-------------------------------------------------------
Game style      Table |   |   |   |   |   |   |   |OFF|
              Upright |   |   |   |   |   |   |   |ON |


 ***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern unsigned char *naughtyb_videoram2;
extern unsigned char *naughtyb_scrollreg;

int naughtyb_DSW_r (int offset);
int naughtyb_interrupt (void);

void naughtyb_videoram2_w(int offset,int data);
void naughtyb_scrollreg_w (int offset,int data);
void naughtyb_videoreg_w (int offset,int data);
int naughtyb_vh_start(void);
void naughtyb_vh_stop(void);
void naughtyb_vh_screenrefresh(struct osd_bitmap *bitmap);

// Let's skip the sound for now.. ;)
// void naughtyb_sound_control_a_w(int offset, int data);
// void naughtyb_sound_control_b_w(int offset, int data);
// int naughtyb_sh_init(const char *gamename);
// int naughtyb_sh_start(void);
// void naughtyb_sh_update(void);


static struct MemoryReadAddress readmem[] =
{
	{ 0xb800, 0xbfff, input_port_1_r },     /* DSW */
	{ 0x4000, 0x7fff, MRA_RAM },    /* work RAM */
	{ 0x8000, 0x87ff, MRA_RAM },    /* Scrollable video RAM */
	{ 0x8800, 0x8fff, MRA_RAM },    /* Scores RAM and so on */
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0xb000, 0xb7ff, input_port_0_r },     /* IN0 */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x4000, 0x7fff, MWA_RAM },
	{ 0x8800, 0x8fff, naughtyb_videoram2_w, &naughtyb_videoram2 },
	{ 0x8000, 0x87ff, videoram_w, &videoram, &videoram_size },
	{ 0x9000, 0x97ff, naughtyb_videoreg_w },
	{ 0x9800, 0x9fff, MWA_RAM, &naughtyb_scrollreg },
//        { 0xa000, 0xa7ff, naughtyb_sound_control_a_w },
//        { 0xa800, 0xafff, naughtyb_sound_control_b_w },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ -1 }  /* end of table */
};


INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )

	PORT_START	/* DSW0 & VBLANK */
	PORT_DIPNAME( 0x03, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x0c, 0x04, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x04, "30000" )
	PORT_DIPSETTING(    0x08, "50000" )
	PORT_DIPSETTING(    0x0c, "70000" )
	PORT_DIPNAME( 0x30, 0x10, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x20, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x30, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x40, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x40, "Hard" )
	/* This is a bit of a mystery. Bit 0x80 is read as the vblank, but
	   it apparently also controls cocktail/table mode. */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	512,    /* 512 characters */
	2,      /* 2 bits per pixel */
	{ 0, 512*8*8 }, /* the two bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },     /* pretty straightforward layout */
	8*8     /* every char takes 8 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,  0, 16 },
	{ 1, 0x2000, &charlayout,  0, 16 },
	{ -1 } /* end of array */
};



const unsigned char palette[] =
{
	0x00,0x00,0x00,	/* BLACK */
	0xdb,0xdb,0xdb,	/* WHITE */
	0xff,0x00,0x00,	/* RED */
	0x00,0xff,0x00, /* GREEN */
	0x24,0x24,0xdb,	/* BLUE */
	0x00,0xff,0xdb,	/* CYAN, */
	0xff,0xff,0x00,	/* YELLOW, */
	239,3,239,      /* PINK */
	0xff,0xb6,0x49,	/* ORANGE */
	0xff,0x24,0xb6, /* LTPURPLE */
	0xb6,0x24,0xff, /* DKPURPLE */
	0x00,0xdb,0xdb,	/* DKCYAN */
	0xdb,0xdb,0x00,	/* DKYELLOW */
	0x95,0x95,0xff, /* BLUISH */
	0xff,0x00,0xff, /* PURPLE */
	0xdb,0x49,0x00, /* BROWN */
};

enum {BLACK,WHITE,RED,GREEN,BLUE,CYAN,YELLOW,PINK,ORANGE,LTPURPLE,
                DKPURPLE,DKCYAN,DKYELLOW,BLUISH,PURPLE,BROWN};
#define UNUSED BLACK

/* COLORS ARE **COMPLETELY** GUESSED. */
/* 4 colors per pixel * 8 groups of characters * 2 charsets * 2 pallettes */
const unsigned char colortable[] =
{
	/* charset A */
	BLACK,WHITE,BLUE,RED,          // bg, title outline, title main, hiscore
	BLACK,WHITE,BROWN,GREEN,        // bg,cinemat,bridge edge,trees
	BLACK,YELLOW,RED,PURPLE,      //   /* bg, rock explosion */
	BLACK,YELLOW,RED,PURPLE,           /* bg, monster,mouth,flash */
	BLACK,YELLOW,RED,PURPLE,           /* bg, monster,mouth,flash */
	BLACK,BLUISH,YELLOW,WHITE,         /* bg, arms/legs,rock/face,body/feet */
	BLACK,BLUISH,YELLOW,WHITE,         /* bg, arms/legs,rock/face,body/feet */
	BLACK,BLUISH,YELLOW,WHITE,         /* bg, during throw */
	/* charset B */
	BLACK,WHITE,RED,RED,          // bg, title outline, title main, hiscore
	BLACK,WHITE,BROWN,GREEN,        // bg,cinemat,bridge edge,trees
	BLACK,YELLOW,RED,PURPLE,      //   /* bg, rock explosion */
	BLACK,YELLOW,RED,PURPLE,           /* bg, monster,mouth,flash */
	BLACK,YELLOW,RED,PURPLE,           /* bg, monster,mouth,flash */
	BLACK,BLUISH,YELLOW,WHITE,         /* bg, arms/legs,rock/face,body/feet */
	BLACK,BLUISH,YELLOW,WHITE,         /* bg, arms/legs,rock/face,body/feet */
	BLACK,BLUISH,YELLOW,WHITE,         /* bg, during throw */
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			1500000,	/* 3 Mhz ? */
			0,
			readmem,writemem,0,0,
			naughtyb_interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
	28*8, 36*8, { 0*8, 28*8-1, 0*8, 36*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,

	VIDEO_TYPE_RASTER,
	0,
	naughtyb_vh_start,
	naughtyb_vh_stop,
	naughtyb_vh_screenrefresh,

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

ROM_START( naughtyb_rom )
        ROM_REGION(0x10000)      /* 64k for code */
        ROM_LOAD( "nb1ic30", 0x0000, 0x0800, 0xe786dfc2 )
        ROM_LOAD( "nb2ic29", 0x0800, 0x0800, 0x3529d2bd )
        ROM_LOAD( "nb3ic28", 0x1000, 0x0800, 0xc2f542bd )
        ROM_LOAD( "nb4ic27", 0x1800, 0x0800, 0xcfbf8b5b )
        ROM_LOAD( "nb5ic26", 0x2000, 0x0800, 0x9974a746 )
        ROM_LOAD( "nb6ic25", 0x2800, 0x0800, 0x3f95af23 )
        ROM_LOAD( "nb7ic24", 0x3000, 0x0800, 0xc0994fa1 )
        ROM_LOAD( "nb8ic23", 0x3800, 0x0800, 0x22d97209 )

        ROM_REGION(0x4000)      /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "nb9ic50",  0x0000, 0x0800, 0x56e6b6e8 )
        ROM_LOAD( "nb10ic49", 0x0800, 0x0800, 0x874eec76 )
        ROM_LOAD( "nb11ic48", 0x1000, 0x0800, 0xc7130b4d )
        ROM_LOAD( "nb12ic47", 0x1800, 0x0800, 0x5e0eaa60 )
        ROM_LOAD( "nb13ic46", 0x2000, 0x0800, 0xa7e78c51 )
        ROM_LOAD( "nb14ic45", 0x2800, 0x0800, 0xc07b7ad3 )
        ROM_LOAD( "nb15ic44", 0x3000, 0x0800, 0x57092643 )
        ROM_LOAD( "nb16ic43", 0x3800, 0x0800, 0x88e94dcf )
ROM_END



static int hiload(const char *name)
{
   FILE *f;

   /* get RAM pointer (this game is multiCPU, we can't assume the global */
   /* RAM pointer is pointing to the right place) */
   unsigned char *RAM = Machine->memory_region[0];


   /* check if the hi score has already been written to screen */
   if((RAM[0x874a] == 8) && (RAM[0x8746] == 9) && (RAM[0x8742] == 7) && (RAM[0x873e] == 8) &&  /* HIGH */
      (RAM[0x8743] == 0x20) && (RAM[0x873f] == 0x20) && (RAM[0x873b] == 0x20) && (RAM[0x8737] == 0x20))
   {
      if ((f = fopen(name,"rb")) != 0)
      {
	 char buf[10];
	 int hi;

         fread(&RAM[0x4003],1,4,f);

	 /* also copy the high score to the screen, otherwise it won't be */
	 /* updated */
	 hi = (RAM[0x4006] & 0x0f) +
	      (RAM[0x4006] >> 4) * 10 +
	      (RAM[0x4005] & 0x0f) * 100 +
	      (RAM[0x4005] >> 4) * 1000 +
	      (RAM[0x4004] & 0x0f) * 10000 +
	      (RAM[0x4004] >> 4) * 100000 +
	      (RAM[0x4003] & 0x0f) * 1000000 +
	      (RAM[0x4003] >> 4) * 10000000;
	 if (hi)
	 {
		 sprintf(buf,"%8d",hi);
		 if (buf[2] != ' ') videoram_w(0x0743,buf[2]-'0'+0x20);
		 if (buf[3] != ' ') videoram_w(0x073f,buf[3]-'0'+0x20);
		 if (buf[4] != ' ') videoram_w(0x073b,buf[4]-'0'+0x20);
		 if (buf[5] != ' ') videoram_w(0x0737,buf[5]-'0'+0x20);
		 if (buf[6] != ' ') videoram_w(0x0733,buf[6]-'0'+0x20);
		 if (buf[7] != ' ') videoram_w(0x072f,buf[7]-'0'+0x20);
	 }
	 fclose(f);
      }

      return 1;
   }
   else return 0; /* we can't load the hi scores yet */
}



static unsigned long get_score(char *score)
{
   return (score[3])+(154*score[2])+((unsigned long)(39322)*score[1])+((unsigned long)(39322)*154*score[0]);
}

static void hisave(const char *name)
{
   unsigned long score1,score2,hiscore;
   FILE *f;

   unsigned char *RAM = Machine->memory_region[0];

   score1 = get_score(&RAM[0x4020]);
   score2 = get_score(&RAM[0x4030]);
   hiscore = get_score(&RAM[0x4003]);

   if (score1 > hiscore) RAM += 0x4020;
   else if (score2 > hiscore) RAM += 0x4030;
   else RAM += 0x4003;

   if ((f = fopen(name,"wb")) != 0)
   {
      fwrite(&RAM[0],1,4,f);
      fclose(f);
   }
}




struct GameDriver naughtyb_driver =
{
	"Naughty Boy",
	"naughtyb",
	"Sal and John Bugliarisi (MAME driver)\nMirko Buffoni (additional code)\nNicola Salmoria (additional code)",
	&machine_driver,

	naughtyb_rom,
	0, 0,
	0,

	0/*TBR*/,input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	hiload, hisave
};


ROM_START( popflame_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ic86.pop", 0x0000, 0x1000, 0x3874201e )
	ROM_LOAD( "ic80.pop", 0x1000, 0x1000, 0x62c9e875 )
	ROM_LOAD( "ic94.pop", 0x2000, 0x1000, 0xef0139a5 )
	ROM_LOAD( "ic100.pop", 0x3000, 0x1000, 0x782f7f4d )

	ROM_REGION(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ic29.pop", 0x0000, 0x1000, 0x8244c820 )
	ROM_LOAD( "ic38.pop", 0x1000, 0x1000, 0x19ed84eb )
	ROM_LOAD( "ic3.pop", 0x2000, 0x1000, 0xde7c7614 )
	ROM_LOAD( "ic13.pop", 0x3000, 0x1000, 0xb40c561a )
ROM_END



struct GameDriver popflame_driver =
{
	"Pop Flamer",
	"popflame",
	"Brad Oliver (MAME driver)\nSal and John Bugliarisi (Naughty Boy driver)\nMirko Buffoni (additional code)\nNicola Salmoria (additional code)",
	&machine_driver,

	popflame_rom,
	0, 0,
	0,

	0/*TBR*/,input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	0, 0
};
