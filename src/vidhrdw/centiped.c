/*************************************************************************

	Atari Centipede hardware

*************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "centiped.h"



static struct rectangle spritevisiblearea =
{
	1*8, 31*8-1,
	0*8, 30*8-1
};

static struct rectangle spritevisiblearea_flip =
{
	1*8, 31*8-1,
	2*8, 32*8-1
};


/***************************************************************************

  Centipede doesn't have a color PROM. Eight RAM locations control
  the color of characters and sprites. The meanings of the four bits are
  (all bits are inverted):

  bit 3 alternate
        blue
        green
  bit 0 red

  The alternate bit affects blue and green, not red. The way I weighted its
  effect might not be perfectly accurate, but is reasonably close.

***************************************************************************/

PALETTE_INIT( centiped )
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	/* characters use colors 0-3, that become 0-15 due to raster effects */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = i;

	/* Centipede is unusual because the sprite color code specifies the */
	/* colors to use one by one, instead of a combination code. */
	/* bit 5-4 = color to use for pen 11 */
	/* bit 3-2 = color to use for pen 10 */
	/* bit 1-0 = color to use for pen 01 */
	/* pen 00 is transparent */
	for (i = 0;i < TOTAL_COLORS(1);i+=4)
	{
		COLOR(1,i+0) = 16;
		COLOR(1,i+1) = 16 + ((i >> 2) & 3);
		COLOR(1,i+2) = 16 + ((i >> 4) & 3);
		COLOR(1,i+3) = 16 + ((i >> 6) & 3);
	}
}


static void setcolor(int pen,int data)
{
	int r,g,b;


	r = 0xff * ((~data >> 0) & 1);
	g = 0xff * ((~data >> 1) & 1);
	b = 0xff * ((~data >> 2) & 1);

	if (~data & 0x08) /* alternate = 1 */
	{
		/* when blue component is not 0, decrease it. When blue component is 0, */
		/* decrease green component. */
		if (b) b = 0xc0;
		else if (g) g = 0xc0;
	}

	palette_set_color(pen,r,g,b);
}

WRITE_HANDLER( centiped_paletteram_w )
{
	paletteram[offset] = data;

	/* the char palette will be effectively updated by the next interrupt handler */

	if (offset >= 12 && offset < 16)	/* sprites palette */
	{
		setcolor(16 + (offset - 12),data);
	}
}

static int powerup_counter;

MACHINE_INIT( centiped )
{
	powerup_counter = 10;
}


INTERRUPT_GEN( centiped_interrupt )
{
	int offset;
	int slice = 3 - cpu_getiloops();

	/* set the palette for the previous screen slice to properly support */
	/* midframe palette changes in test mode */
	for (offset = 4;offset < 8;offset++)
		setcolor(4 * slice + (offset - 4),paletteram[offset]);

	/* Centipede doesn't like to receive interrupts just after a reset. */
	/* The only workaround I've found is to wait a little before starting */
	/* to generate them. */
	if (powerup_counter == 0)
		cpu_set_irq_line(0, 0, HOLD_LINE);
	else
		powerup_counter--;
}



/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

VIDEO_UPDATE( centiped )
{
	int offs;

	if (get_vh_global_attribute_changed())
		memset (dirtybuffer, 1, videoram_size);

	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy,flipx,flipy;


			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;
			flipx = videoram[offs] & 0x40;
			flipy = videoram[offs] & 0x80;

			if (flip_screen)
			{
				sy += 2;
				flipx = !flipx;
				flipy = !flipy;
			}

			drawgfx(tmpbitmap,Machine->gfx[0],
					(videoram[offs] & 0x3f) + 0x40,
					(sy + 1) / 8,	/* support midframe palette changes in test mode */
					flipx,flipy,
					8*sx,8*sy,
					&Machine->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites */
	for (offs = 0;offs < 0x10;offs++)
	{
		int code,color;
		int flipx;
		int x, y;


		code = ((spriteram[offs] & 0x3e) >> 1) | ((spriteram[offs] & 0x01) << 6);
		color = spriteram[offs+0x30];
		flipx = (spriteram[offs] & 0x80);
		x = spriteram[offs + 0x20];
		y = 240 - spriteram[offs + 0x10];

		if (flip_screen)
		{
			y += 16;
		}

		drawgfx(bitmap,Machine->gfx[1],
				code,
				color & 0x3f,
				flip_screen,flipx,
				x,y,
				flip_screen ? &spritevisiblearea_flip : &spritevisiblearea,
				TRANSPARENCY_PEN,0);
	}
}
