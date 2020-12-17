/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

  This is the driver for the "Galaxian" style board, used, with small
  variations, by an incredible amount of games in the early 80s.

  This video driver is used by the following drivers:
  - galaxian.c
  - mooncrst.c
  - moonqsr.c
  - scramble.c
  - scobra.c
  - ckongs.c

  TODO: cocktail support hasn't been implemented properly yet

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



#define MAX_STARS 250
#define STARS_COLOR_BASE 32

unsigned char *galaxian_attributesram;
unsigned char *galaxian_bulletsram;
int galaxian_bulletsram_size;
static int stars_on,stars_blink;
static int stars_type;	/* 0 = Galaxian stars  1 = Scramble stars */
static unsigned int stars_scroll;

struct star
{
	int x,y,code,col;
};
static struct star stars[MAX_STARS];
static int total_stars;
static int gfx_bank;	/* used by Pisces and "japirem" only */
static int gfx_extend;	/* used by Moon Cresta only */
static int flipscreen[2];



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Moon Cresta has one 32 bytes palette PROM, connected to the RGB output
  this way:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

  The output of the background star generator is connected this way:

  bit 5 -- 100 ohm resistor  -- BLUE
        -- 150 ohm resistor  -- BLUE
        -- 100 ohm resistor  -- GREEN
        -- 150 ohm resistor  -- GREEN
        -- 100 ohm resistor  -- RED
  bit 0 -- 150 ohm resistor  -- RED

***************************************************************************/
void galaxian_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;


	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	/* first, the character/sprite palette */
	for (i = 0;i < 32;i++)
	{
		int bit0,bit1,bit2;


		/* red component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		color_prom++;
	}
	/* now the stars */
	for (i = 0;i < 64;i++)
	{
		int bits;
		int map[4] = { 0x00, 0x88, 0xcc, 0xff };


		bits = (i >> 0) & 0x03;
		*(palette++) = map[bits];
		bits = (i >> 2) & 0x03;
		*(palette++) = map[bits];
		bits = (i >> 4) & 0x03;
		*(palette++) = map[bits];
	}


	/* characters and sprites use the same palette */
	for (i = 0;i < TOTAL_COLORS(0);i++)
	{
		if (i & 3) COLOR(0,i) = i;
		else COLOR(0,i) = 0;	/* 00 is always black, regardless of the contents of the PROM */
	}

	/* bullets can be either white or yellow */
	COLOR(2,0) = 0;
	COLOR(2,1) = 0x0f + STARS_COLOR_BASE;	/* yellow */
	COLOR(2,2) = 0;
	COLOR(2,3) = 0x3f + STARS_COLOR_BASE;	/* white */
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
static int common_vh_start(void)
{
	int generator;
	int x,y;


	gfx_bank = 0;
	gfx_extend = 0;
	stars_on = 0;

	if (generic_vh_start() != 0)
		return 1;


	/* precalculate the star background */
	total_stars = 0;
	generator = 0;

	for (y = 255;y >= 0;y--)
	{
		for (x = 511;x >= 0;x--)
		{
			int bit1,bit2;


			generator <<= 1;
			bit1 = (~generator >> 17) & 1;
			bit2 = (generator >> 5) & 1;

			if (bit1 ^ bit2) generator |= 1;

			if (y >= Machine->drv->visible_area.min_y &&
					y <= Machine->drv->visible_area.max_y &&
					((~generator >> 16) & 1) &&
					(generator & 0xff) == 0xff)
			{
				int color;

				color = (~(generator >> 8)) & 0x3f;
				if (color && total_stars < MAX_STARS)
				{
					stars[total_stars].x = x;
					stars[total_stars].y = y;
					stars[total_stars].code = color;
					stars[total_stars].col = Machine->pens[color + STARS_COLOR_BASE];

					total_stars++;
				}
			}
		}
	}

	return 0;
}

int galaxian_vh_start(void)
{
	stars_type = 0;
	return common_vh_start();
}

int scramble_vh_start(void)
{
	stars_type = 1;
	return common_vh_start();
}



void galaxian_flipx_w(int offset,int data)
{
	if (flipscreen[0] != (data & 1))
	{
		flipscreen[0] = data & 1;
		memset(dirtybuffer,1,videoram_size);
	}
}

void galaxian_flipy_w(int offset,int data)
{
	if (flipscreen[1] != (data & 1))
	{
		flipscreen[1] = data & 1;
		memset(dirtybuffer,1,videoram_size);
	}
}



void galaxian_attributes_w(int offset,int data)
{
	if ((offset & 1) && galaxian_attributesram[offset] != data)
	{
		int i;


		for (i = offset / 2;i < videoram_size;i += 32)
			dirtybuffer[i] = 1;
	}

	galaxian_attributesram[offset] = data;
}



void galaxian_stars_w(int offset,int data)
{
	stars_on = (data & 1);
	stars_scroll = 0;
}



void pisces_gfxbank_w(int offset,int data)
{
	gfx_bank = data & 1;
}



void mooncrst_gfxextend_w(int offset,int data)
{
	if (data) gfx_extend |= (1 << offset);
	else gfx_extend &= ~(1 << offset);
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void galaxian_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int i,offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy,charcode;


			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

			if (flipscreen[0]) sx = 31 - sx;
			if (flipscreen[1]) sy = 31 - sy;


			charcode = videoram[offs];

			/* bit 5 of [2*(offs%32)+1] is used only by Moon Quasar */
			if (galaxian_attributesram[2 * (offs % 32) + 1] & 0x20)
				charcode += 256;

			/* gfx_bank is used by Pisces and japirem/Uniwars only */
			if (gfx_bank)
				charcode += 256;

			/* gfx_extend is used by Moon Cresta only */
			if ((gfx_extend & 4) && (charcode & 0xc0) == 0x80)
				charcode = (charcode & 0x3f) | (gfx_extend << 6);

 			drawgfx(tmpbitmap,Machine->gfx[0],
					charcode,
					galaxian_attributesram[2 * (offs % 32) + 1] & 0x07,
					flipscreen[0],flipscreen[1],
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		int scroll[32];


		for (i = 0;i < 32;i++)
			scroll[i] = -galaxian_attributesram[2 * i];

		copyscrollbitmap(bitmap,tmpbitmap,0,0,32,scroll,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* Draw the bullets */
	for (offs = 0;offs < galaxian_bulletsram_size;offs += 4)
	{
		int x,y;
		int color;


		if (offs == 7*4) color = 0;	/* yellow */
		else color = 1;	/* white */

		x = 255 - galaxian_bulletsram[offs + 3] - Machine->drv->gfxdecodeinfo[2].gfxlayout->width;
		y = 256 - galaxian_bulletsram[offs + 1] - Machine->drv->gfxdecodeinfo[2].gfxlayout->height;

		drawgfx(bitmap,Machine->gfx[2],
				0,	/* this is just a line, generated by the hardware */
				color,
				0,0,
				x,y,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}


	/* Draw the sprites */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int flipx,flipy,sx,sy,spritecode;


		sx = spriteram[offs + 3];
		if (sx > 8)	/* ??? */
		{
			sy = 240 - spriteram[offs];
			flipx = spriteram[offs + 1] & 0x40;
			flipy = spriteram[offs + 1] & 0x80;

			if (flipscreen[0])
			{
				flipx = !flipx;
				sx = 240 - sx;
			}
			if (flipscreen[1])
			{
				flipy = !flipy;
				sy = 240 - sy;
			}

			spritecode = spriteram[offs + 1] & 0x3f;

			/* bit 4 of [offs+2] is used only by Crazy Kong */
			if (spriteram[offs + 2] & 0x10)
				spritecode += 64;

			/* bit 5 of [offs+2] is used only by Moon Quasar */
			if (spriteram[offs + 2] & 0x20)
				spritecode += 64;

			/* gfx_bank is used by Pisces and japirem/Uniwars only */
			if (gfx_bank)
				spritecode += 64;

			/* gfx_extend is used by Moon Cresta only */
			if ((gfx_extend & 4) && (spritecode & 0x30) == 0x20)
				spritecode = (spritecode & 0x0f) | (gfx_extend << 4);

			drawgfx(bitmap,Machine->gfx[1],
					spritecode,
					spriteram[offs + 2] & 0x07,
					flipx,flipy,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}


	/* draw the stars */
	if (stars_on)
	{
		int bpen;


		bpen = Machine->pens[0];

		switch (stars_type)
		{
			case 0:	/* Galaxian stars */
				for (offs = 0;offs < total_stars;offs++)
				{
					int x,y;


					x = (stars[offs].x + stars_scroll/2) % 256;
					y = stars[offs].y;

					if ((y & 1) ^ ((x >> 4) & 1))
					{
						if (Machine->orientation & ORIENTATION_SWAP_XY)
						{
							int temp;


							temp = x;
							x = y;
							y = temp;
						}
						if (Machine->orientation & ORIENTATION_FLIP_X)
							x = 255 - x;
						if (Machine->orientation & ORIENTATION_FLIP_Y)
							y = 255 - y;

						if (bitmap->line[y][x] == bpen)
							bitmap->line[y][x] = stars[offs].col;
					}
				}
				break;

			case 1:	/* Scramble stars */
				for (offs = 0;offs < total_stars;offs++)
				{
					int x,y;


					x = stars[offs].x / 2;
					y = stars[offs].y;

					if ((y & 1) ^ ((x >> 4) & 1))
					{
						if (Machine->orientation & ORIENTATION_SWAP_XY)
						{
							int temp;


							temp = x;
							x = y;
							y = temp;
						}
						if (Machine->orientation & ORIENTATION_FLIP_X)
							x = 255 - x;
						if (Machine->orientation & ORIENTATION_FLIP_Y)
							y = 255 - y;

						if (bitmap->line[y][x] == bpen)
						{
							switch (stars_blink)
							{
								case 0:
									if (stars[offs].code & 1) bitmap->line[y][x] = stars[offs].col;
									break;
								case 1:
									if (stars[offs].code & 4) bitmap->line[y][x] = stars[offs].col;
									break;
								case 2:
									if (stars[offs].x & 4) bitmap->line[y][x] = stars[offs].col;
									break;
								case 3:
									bitmap->line[y][x] = stars[offs].col;
									break;
							}
						}
					}
				}
				break;
		}
	}
}



int galaxian_vh_interrupt(void)
{
	stars_scroll++;

	return nmi_interrupt();
}



int scramble_vh_interrupt()
{
	static int blink_count;


	blink_count++;
	if (blink_count >= 43)
	{
		blink_count = 0;
		stars_blink = (stars_blink + 1) % 4;
	}

	return nmi_interrupt();
}
