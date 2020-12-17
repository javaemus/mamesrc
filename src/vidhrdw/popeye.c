/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *popeye_videoram;
int popeye_videoram_size;
unsigned char *popeye_background_pos,*popeye_palette_bank;
static unsigned char *dirtybuffer2;
static struct osd_bitmap *tmpbitmap2;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Popeye has four color PROMS:
  - 32x8 char palette
  - 32x8 background palette
  - two 256x4 sprite palette
  I don't know for sure how the PROMs are connected to the RGB output, but
  it's probably the usual:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

***************************************************************************/
void popeye_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < 256;i++)
	{
		int bits;


		bits = (i >> 0) & 0x07;
		palette[3*i] = (bits >> 1) | (bits << 2) | (bits << 5);
		bits = (i >> 3) & 0x07;
		palette[3*i + 1] = (bits >> 1) | (bits << 2) | (bits << 5);
		bits = (i >> 6) & 0x03;
		palette[3*i + 2] = bits | (bits >> 2) | (bits << 4) | (bits << 6);
	}

	for (i = 0;i < 32;i++)	/* characters */
	{
		colortable[2*i] = 0;
		colortable[2*i + 1] = color_prom[i];
	}
	for (i = 0;i < 32;i++)	/* background */
	{
		colortable[i+32*2] = color_prom[i+32];
	}
	for (i = 0;i < 64*4;i++)	/* sprites */
	{
		colortable[i+32*2+32] = (color_prom[i+64] & 0x0f) | ((color_prom[i+64+64*4] << 4) & 0xf0);
	}
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int popeye_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

	if ((dirtybuffer2 = malloc(popeye_videoram_size)) == 0)
	{
		generic_vh_stop();
		return 1;
	}
	memset(dirtybuffer2,1,popeye_videoram_size);

	if ((tmpbitmap2 = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}

	return 0;
}


/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void popeye_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap2);
	free(dirtybuffer2);
	generic_vh_stop();
}



void popeye_videoram_w(int offset,int data)
{
	if (data & 0x80)	/* write to the upper nibble */
	{
		if ((popeye_videoram[offset] & 0xf0) != ((data << 4) & 0xf0))
		{
			dirtybuffer2[offset] = 1;

			popeye_videoram[offset] = (popeye_videoram[offset] & 0x0f) | ((data << 4) & 0xf0);
		}
	}
	else	/* write to the lower nibble */
	{
		if ((popeye_videoram[offset] & 0x0f) != (data & 0x0f))
		{
			dirtybuffer2[offset] = 1;

			popeye_videoram[offset] = (popeye_videoram[offset] & 0xf0) | (data & 0x0f);
		}
	}
}



void popeye_palettebank_w(int offset,int data)
{
	if ((data & 0x08) != (*popeye_palette_bank & 0x08))
	{
		memset(dirtybuffer,1,videoram_size);
		memset(dirtybuffer2,1,popeye_videoram_size);
	}

	*popeye_palette_bank = data;
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void popeye_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = popeye_videoram_size - 1 - 128;offs >= 128;offs--)
	{
		if (dirtybuffer2[offs])
		{
			int sx,sy,y,colour;


			dirtybuffer2[offs] = 0;

			sx = 8 * (offs % 64);
			sy = 8 * (offs / 64) - 16;

			if (sx >= Machine->drv->visible_area.min_x &&
					sx+7 <= Machine->drv->visible_area.max_x &&
					sy >= Machine->drv->visible_area.min_y &&
					sy+7 <= Machine->drv->visible_area.max_y)
			{
				colour = Machine->gfx[3]->colortable[(popeye_videoram[offs] & 0x0f) + 2*(*popeye_palette_bank & 0x08)];
				for (y = 0;y < 4;y++)
					memset(&tmpbitmap2->line[sy+y][sx],colour,8);

				colour = Machine->gfx[3]->colortable[(popeye_videoram[offs] >> 4) + 2*(*popeye_palette_bank & 0x08)];
				for (y = 4;y < 8;y++)
					memset(&tmpbitmap2->line[sy+y][sx],colour,8);
			}
		}
	}


	if (popeye_background_pos[0] == 0)	/* no background */
	{
		for (offs = videoram_size - 1;offs >= 0;offs--)
		{
			if (dirtybuffer[offs])
			{
				int sx,sy;


				dirtybuffer[offs] = 0;

				sx = 16 * (offs % 32);
				sy = 16 * (offs / 32) - 16;

				drawgfx(tmpbitmap,Machine->gfx[0],
						videoram[offs],colorram[offs],
						0,0,sx,sy,
						&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
			}
		}


		/* copy the frontmost playfield (should be in front of sprites, but never mind) */
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}
	else
	{
		/* copy the background graphics */
		copybitmap(bitmap,tmpbitmap2,0,0,
			400 - 2 * popeye_background_pos[0],
			2 * (256 - popeye_background_pos[1]),
			&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		/*
		 * offs+3:
		 * bit 7 ?
		 * bit 6 ?
		 * bit 5 ?
		 * bit 4 MSB of sprite code
		 * bit 3 vertical flip
		 * bit 2 sprite bank
		 * bit 1 \ color (with bit 2 as well)
		 * bit 0 /
		 */
		if (spriteram[offs] != 0)
			drawgfx(bitmap,Machine->gfx[(spriteram[offs + 3] & 0x04) ? 1 : 2],
					0xff - (spriteram[offs + 2] & 0x7f) - 8*(spriteram[offs + 3] & 0x10),
					(spriteram[offs + 3] & 0x07) + 8*(*popeye_palette_bank & 0x07),
					spriteram[offs + 2] & 0x80,spriteram[offs + 3] & 0x08,
					2*(spriteram[offs])-7,2*(256-spriteram[offs + 1]) - 16,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}


	if (popeye_background_pos[0] != 0)	/* background is present */
	{
		/* draw the frontmost playfield. They are characters, but draw them as sprites */
		for (offs = videoram_size - 1;offs >= 0;offs--)
		{
			int sx,sy;


			if (videoram[offs] != 0xff)	/* don't draw spaces */
			{
				sx = 16 * (offs % 32);
				sy = 16 * (offs / 32) - 16;

				drawgfx(bitmap,Machine->gfx[0],
						videoram[offs],colorram[offs],
						0,0,sx,sy,
						&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
			}
		}
	}
}