/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *commando_bgvideoram,*commando_bgcolorram;
int commando_bgvideoram_size;
unsigned char *commando_scrollx,*commando_scrolly;
static unsigned char *dirtybuffer2;
static unsigned char *spritebuffer1,*spritebuffer2;
static struct osd_bitmap *tmpbitmap2;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Commando has three 256x4 palette PROMs (one per gun), connected to the
  RGB output this way:

  bit 3 -- 220 ohm resistor  -- RED/GREEN/BLUE
        -- 470 ohm resistor  -- RED/GREEN/BLUE
        -- 1  kohm resistor  -- RED/GREEN/BLUE
  bit 0 -- 2.2kohm resistor  -- RED/GREEN/BLUE

***************************************************************************/
void commando_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < 256;i++)
	{
		int bit0,bit1,bit2,bit3;


		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		bit3 = (color_prom[i] >> 3) & 0x01;
		palette[3*i] = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[i+256] >> 0) & 0x01;
		bit1 = (color_prom[i+256] >> 1) & 0x01;
		bit2 = (color_prom[i+256] >> 2) & 0x01;
		bit3 = (color_prom[i+256] >> 3) & 0x01;
		palette[3*i + 1] = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[i+256*2] >> 0) & 0x01;
		bit1 = (color_prom[i+256*2] >> 1) & 0x01;
		bit2 = (color_prom[i+256*2] >> 2) & 0x01;
		bit3 = (color_prom[i+256*2] >> 3) & 0x01;
		palette[3*i + 2] = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
	}

	/* characters use colors 192-255 */
	for (i = 0;i < 16*4;i++)
		colortable[i] = i + 192;

	/* sprites use colors 128-191 */
	for (i = 16*4;i < 16*4+4*16;i++)
		colortable[i] = i - 16*4 + 128;

	/* background tiles use colors 0-127 */
	for (i = 16*4+4*16;i < 16*4+4*16+16*8;i++)
		colortable[i] = i - (16*4+4*16);
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int commando_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

	if ((dirtybuffer2 = malloc(commando_bgvideoram_size)) == 0)
	{
		generic_vh_stop();
		return 1;
	}
	memset(dirtybuffer2,1,commando_bgvideoram_size);

	if ((spritebuffer1 = malloc(spriteram_size)) == 0)
	{
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}

	if ((spritebuffer2 = malloc(spriteram_size)) == 0)
	{
		free(spritebuffer1);
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}

	if (generic_vh_start() != 0)
		return 1;

	/* the background area is twice as tall and twice as large as the screen */
	if ((tmpbitmap2 = osd_create_bitmap(2*Machine->drv->screen_width,2*Machine->drv->screen_height)) == 0)
	{
		free(spritebuffer2);
		free(spritebuffer1);
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void commando_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap2);
	free(spritebuffer2);
	free(spritebuffer1);
	free(dirtybuffer2);
	generic_vh_stop();
}



void commando_bgvideoram_w(int offset,int data)
{
	if (commando_bgvideoram[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		commando_bgvideoram[offset] = data;
	}
}



void commando_bgcolorram_w(int offset,int data)
{
	if (commando_bgcolorram[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		commando_bgcolorram[offset] = data;
	}
}



int commando_interrupt(void)
{
	if (cpu_getiloops() == 1) return 0x00cf;	/* RST 08h */
	else
	{
		/* we must store previous sprite data in a buffer and draw that instead of */
		/* the latest one, otherwise sprites will not be synchronized with */
		/* background scrolling */
		memcpy(spritebuffer2,spritebuffer1,spriteram_size);
		memcpy(spritebuffer1,spriteram,spriteram_size);

		return 0x00d7;	/* RST 10h - VBLANK */
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void commando_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	for (offs = commando_bgvideoram_size - 1;offs >= 0;offs--)
	{
		int sx,sy;


		if (dirtybuffer2[offs])
		{
			dirtybuffer2[offs] = 0;

			sx = offs % 32;
			sy = 31 - offs / 32;

			drawgfx(tmpbitmap2,Machine->gfx[1],
					commando_bgvideoram[offs] + 4*(commando_bgcolorram[offs] & 0xc0),
					commando_bgcolorram[offs] & 0x0f,
					commando_bgcolorram[offs] & 0x20,commando_bgcolorram[offs] & 0x10,
					16 * sx,16 * sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the background graphics */
	{
		int scrollx,scrolly;


		scrollx = -(commando_scrollx[0] + 256 * commando_scrollx[1]);
		scrolly = commando_scrolly[0] + 256 * commando_scrolly[1] - 256;

		copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int bank;


		/* the meaning of bit 1 of [offs+1] is unknown */

		bank = ((spritebuffer2[offs + 1] >> 6) & 3);

		if (bank < 3)
			drawgfx(bitmap,Machine->gfx[2],
					spritebuffer2[offs] + 256* bank,
					(spritebuffer2[offs + 1] >> 4) & 3,
					spritebuffer2[offs + 1] & 0x08,spritebuffer2[offs + 1] & 0x04,
					spritebuffer2[offs + 2],240 - spritebuffer2[offs + 3] + 0x100 * (spritebuffer2[offs + 1] & 0x01),
					&Machine->drv->visible_area,TRANSPARENCY_PEN,15);
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int charcode;


		charcode = videoram[offs] + 4 * (colorram[offs] & 0xc0);

		if (charcode != 0x20)	/* don't draw spaces */
		{
			int sx,sy;


			sx = 8 * (offs / 32);
			sy = 8 * (31 - offs % 32);

			drawgfx(bitmap,Machine->gfx[0],
					charcode,
					colorram[offs] & 0x0f,
					colorram[offs] & 0x20,0,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,3);
		}
	}
}
