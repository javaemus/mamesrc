/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



static int gfx_bank,palette_bank;
static const unsigned char *color_codes;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Donkey Kong has two 256x4 palette PROMs and one 256x4 PROM which contains
  the color codes to use for characters on a per column (actually per row,
  since the display is rotated) basis. The codes might also have a per row
  (i.e. per column) component, but the Donkey Kong PROM has the same pattern
  repeated, so we can't know for sure.
  The palette PROMs are connected to the RGB output this way:

  bit 3 -- 220 ohm resistor -- inverter  -- RED \
        -- 470 ohm resistor -- inverter  -- RED | + 470 ohm pullup
        -- 1  kohm resistor -- inverter  -- RED /
  bit 0 -- 220 ohm resistor -- inverter  -- GREEN \
  bit 3 -- 470 ohm resistor -- inverter  -- GREEN | + 470 ohm pullup
        -- 1  kohm resistor -- inverter  -- GREEN /
        -- 220 ohm resistor -- inverter  -- BLUE \ + 680 ohm pullup
  bit 0 -- 470 ohm resistor -- inverter  -- BLUE /

***************************************************************************/
void dkong_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])
	float r0,r1,r2,p1,p2,max,min;
	r0 = 1000;
	r1 = 470;
	r2 = 220;
	p1 = 470;
	p2 = 680;


	max = 1/r0 + 1/r1 + 1/r2 + 1/p2;
	min = 1/p2;

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,val;


		/* red component */
		bit0 = (color_prom[0] >> 1) & 1;
		bit1 = (color_prom[0] >> 2) & 1;
		bit2 = (color_prom[0] >> 3) & 1;
		val = 255 - (bit0/r0 + bit1/r1 + bit2/r2 + 1/p1 - min) * 255 / (max-min);
		if (val < 0) val = 0;
		*(palette++) = val;
		/* green component */
		bit0 = (color_prom[Machine->drv->total_colors] >> 2) & 1;
		bit1 = (color_prom[Machine->drv->total_colors] >> 3) & 1;
		bit2 = (color_prom[0] >> 0) & 1;
		val = 255 - (bit0/r0 + bit1/r1 + bit2/r2 + 1/p1 - min) * 255 / (max-min);
		if (val < 0) val = 0;
		*(palette++) = val;
		/* blue component */
		bit0 = 0;
		bit1 = (color_prom[Machine->drv->total_colors] >> 0) & 1;
		bit2 = (color_prom[Machine->drv->total_colors] >> 1) & 1;
		val = 255 - (bit0/r0 + bit1/r1 + bit2/r2 + 1/p2 - min) * 255 / (max-min);
		if (val < 0) val = 0;
		*(palette++) = val;

		color_prom++;
	}

	color_prom += Machine->drv->total_colors;
	/* color_prom now points to the beginning of the character color codes */
	color_codes = color_prom;	/* we'll need it later */

	/* sprites use the same palette as characters */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = i;
}



int dkong_vh_start(void)
{
	gfx_bank = 0;
	palette_bank = 0;

	return generic_vh_start();
}



void dkongjr_gfxbank_w(int offset,int data)
{
	if (gfx_bank != (data & 1))
	{
		gfx_bank = data & 1;
		memset(dirtybuffer,1,videoram_size);
	}
}

void dkong3_gfxbank_w(int offset,int data)
{
	if (gfx_bank != (~data & 1))
	{
		gfx_bank = ~data & 1;
		memset(dirtybuffer,1,videoram_size);
	}
}



void dkong_palettebank_w(int offset,int data)
{
	int newbank;


	newbank = palette_bank;
	if (data & 1)
		newbank |= 1 << offset;
	else
		newbank &= ~(1 << offset);

	if (palette_bank != newbank)
	{
		palette_bank = newbank;
		memset(dirtybuffer,1,videoram_size);
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void dkong_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;
			int charcode,color;


			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

			charcode = videoram[offs] + 256 * gfx_bank;
			/* retrieve the character color from the PROM */
			color = (color_codes[offs % 32] & 0x0f) + 0x10 * palette_bank;

			drawgfx(tmpbitmap,Machine->gfx[0],
					charcode,color,
					0,0,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites. */
	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		if (spriteram[offs])
		{
			/* spriteram[offs + 2] & 0x40 is used by Donkey Kong 3 only */
			/* spriteram[offs + 2] & 0x30 don't seem to be used (they are */
			/* probably not part of the color code, since Mario Bros, which */
			/* has similar hardware, uses a memory mapped port to change */
			/* palette bank, so it's limited to 16 color codes) */
			drawgfx(bitmap,Machine->gfx[1],
					(spriteram[offs + 1] & 0x7f) + 2 * (spriteram[offs + 2] & 0x40),
					(spriteram[offs + 2] & 0x0f) + 16 * palette_bank,
					spriteram[offs + 2] & 0x80,spriteram[offs + 1] & 0x80,
					spriteram[offs + 3] - 8,240 - spriteram[offs] + 7,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}



/* this would be the same as dkong, but since we don't have the color PROMs for */
/* dkongjr and dkong3, we have to pick the character colors in a different way */
void dkongjr_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;
			int charcode,color;


			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

			charcode = videoram[offs] + 256 * gfx_bank;
			color = charcode >> 2;	/* this is wrong, but gives a reasonable approximation */

			drawgfx(tmpbitmap,Machine->gfx[0],
					charcode,color,
					0,0,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites. */
	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		if (spriteram[offs])
		{
			/* spriteram[offs + 2] & 0x40 is used by Donkey Kong 3 only */
			/* spriteram[offs + 2] & 0x30 don't seem to be used (they are */
			/* probably not part of the color code, since Mario Bros, which */
			/* has similar hardware, uses a memory mapped port to change */
			/* palette bank, so it's limited to 16 color codes) */
			drawgfx(bitmap,Machine->gfx[1],
					(spriteram[offs + 1] & 0x7f) + 2 * (spriteram[offs + 2] & 0x40),
					(spriteram[offs + 2] & 0x0f) + 16 * palette_bank,
					spriteram[offs + 2] & 0x80,spriteram[offs + 1] & 0x80,
					spriteram[offs + 3] - 8,240 - spriteram[offs] + 7,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}