/***************************************************************************

	Midway MCR-68k system

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "mcr.h"


#define LOW_BYTE(x) ((x) & 0xff)


UINT8 mcr68_sprite_clip;
INT8 mcr68_sprite_xoffset;



/*************************************
 *
 *	Palette RAM writes
 *
 *************************************/

WRITE16_HANDLER( mcr68_paletteram_w )
{
	int newword, r, g, b;

	COMBINE_DATA(&paletteram16[offset]);
	newword = paletteram16[offset];

	r = (newword >> 6) & 7;
	b = (newword >> 3) & 7;
	g = (newword >> 0) & 7;

	/* up to 8 bits */
	r = (r << 5) | (r << 2) | (r >> 1);
	g = (g << 5) | (g << 2) | (g >> 1);
	b = (b << 5) | (b << 2) | (b >> 1);

	palette_set_color(offset, r, g, b);
}



/*************************************
 *
 *	Video RAM writes
 *
 *************************************/

WRITE16_HANDLER( mcr68_videoram_w )
{
	int oldword = videoram16[offset];
	int newword = oldword;
	COMBINE_DATA(&newword);

	if (oldword != newword)
	{
		dirtybuffer[offset & ~1] = 1;
		videoram16[offset] = newword;
	}
}



/*************************************
 *
 *	Background update
 *
 *************************************/

static void mcr68_update_background(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int overrender)
{
	int offs;

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = (videoram_size / 2) - 2; offs >= 0; offs -= 2)
	{
		/* this works for overrendering as well, since the sprite code will mark */
		/* intersecting tiles for us */
		if (dirtybuffer[offs])
		{
			int mx = (offs / 2) % 32;
			int my = (offs / 2) / 32;
			int attr = LOW_BYTE(videoram16[offs + 1]);
			int color = (attr & 0x30) >> 4;
			int code = LOW_BYTE(videoram16[offs]) + 256 * (attr & 0x03) + 1024 * ((attr >> 6) & 0x03);

			if (!overrender)
				drawgfx(bitmap, Machine->gfx[0], code, color ^ 3, attr & 0x04, attr & 0x08,
						16 * mx, 16 * my, cliprect, TRANSPARENCY_NONE, 0);
			else if (Machine->gfx[0]->total_elements < 0x1000 && (attr & 0x80))
				drawgfx(bitmap, Machine->gfx[0], code, color ^ 3, attr & 0x04, attr & 0x08,
						16 * mx, 16 * my, cliprect, TRANSPARENCY_PEN, 0);
			else
				continue;

			/* only clear the dirty flag if we're not overrendering */
			dirtybuffer[offs] = 0;
		}
	}
}



/*************************************
 *
 *	Sprite update
 *
 *************************************/

static void mcr68_update_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int priority)
{
	struct rectangle sprite_clip = Machine->visible_area;
	int offs;

	/* adjust for clipping */
	sprite_clip.min_x += mcr68_sprite_clip;
	sprite_clip.max_x -= mcr68_sprite_clip;
	sect_rect(&sprite_clip, cliprect);

	fillbitmap(priority_bitmap,1,&sprite_clip);

	/* loop over sprite RAM */
	for (offs = spriteram_size / 2 - 4;offs >= 0;offs -= 4)
	{
		int code, color, flipx, flipy, x, y, sx, sy, xcount, ycount, flags;

		flags = LOW_BYTE(spriteram16[offs + 1]);
		code = LOW_BYTE(spriteram16[offs + 2]) + 256 * ((flags >> 3) & 0x01) + 512 * ((flags >> 6) & 0x03);

		/* skip if zero */
		if (code == 0)
			continue;

		/* also skip if this isn't the priority we're drawing right now */
		if (((flags >> 2) & 1) != priority)
			continue;

		/* extract the bits of information */
		color = ~flags & 0x03;
		flipx = flags & 0x10;
		flipy = flags & 0x20;
		x = LOW_BYTE(spriteram16[offs + 3]) * 2 + mcr68_sprite_xoffset;
		y = (241 - LOW_BYTE(spriteram16[offs])) * 2;

		/* allow sprites to clip off the left side */
		if (x > 0x1f0) x -= 0x200;

		/* sprites use color 0 for background pen and 8 for the 'under tile' pen.
			The color 8 is used to cover over other sprites. */

		/* first draw the sprite, visible */
		pdrawgfx(bitmap, Machine->gfx[1], code, color, flipx, flipy, x, y,
				&sprite_clip, TRANSPARENCY_PENS, 0x0101, 0x00);

		/* then draw the mask, behind the background but obscuring following sprites */
		pdrawgfx(bitmap, Machine->gfx[1], code, color, flipx, flipy, x, y,
				&sprite_clip, TRANSPARENCY_PENS, 0xfeff, 0x02);


		/* mark tiles underneath as dirty for overrendering */
		if (priority == 0)
		{
			sx = x / 16;
			sy = y / 16;
			xcount = (x & 15) ? 3 : 2;
			ycount = (y & 15) ? 3 : 2;

			for (y = sy; y < sy + ycount; y++)
				for (x = sx; x < sx + xcount; x++)
					if (x >= 0 && x < 32 && y >= 0 && y < 30)
						dirtybuffer[(32 * y + x) * 2] = 1;
		}
	}
}



/*************************************
 *
 *	General MCR/68k update
 *
 *************************************/

VIDEO_UPDATE( mcr68 )
{
	/* draw the background */
	mcr68_update_background(tmpbitmap, &Machine->visible_area, 0);

	/* copy it to the destination */
	copybitmap(bitmap, tmpbitmap, 0, 0, 0, 0, cliprect, TRANSPARENCY_NONE, 0);

	/* draw the low-priority sprites */
	mcr68_update_sprites(bitmap, cliprect, 0);

    /* redraw tiles with priority over sprites */
	mcr68_update_background(bitmap, cliprect, 1);

	/* draw the high-priority sprites */
	mcr68_update_sprites(bitmap, cliprect, 1);
}



/*************************************
 *
 *	Zwackery palette RAM writes
 *
 *************************************/

WRITE16_HANDLER( zwackery_paletteram_w )
{
	int newword, r, g, b;

	COMBINE_DATA(&paletteram16[offset]);
	newword = paletteram16[offset];

	r = (~newword >> 10) & 31;
	b = (~newword >> 5) & 31;
	g = (~newword >> 0) & 31;

	/* up to 8 bits */
	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	palette_set_color(offset, r, g, b);
}



/*************************************
 *
 *	Zwackery video RAM writes
 *
 *************************************/

WRITE16_HANDLER( zwackery_videoram_w )
{
	int oldword = videoram16[offset];
	int newword = oldword;
	COMBINE_DATA(&newword);

	if (oldword != newword)
	{
		dirtybuffer[offset] = 1;
		videoram16[offset] = newword;
	}
}



/*************************************
 *
 *	Zwackery video RAM writes
 *
 *************************************/

WRITE16_HANDLER( zwackery_spriteram_w )
{
	/* yech -- Zwackery relies on the upper 8 bits of a spriteram read being $ff! */
	/* to make this happen we always write $ff in the upper 8 bits */
	COMBINE_DATA(&spriteram16[offset]);
	spriteram16[offset] |= 0xff00;
}



/*************************************
 *
 *	Zwackery color data conversion
 *
 *************************************/

PALETTE_INIT( zwackery )
{
	const UINT8 *colordatabase = (const UINT8 *)memory_region(REGION_GFX3);
	struct GfxElement *gfx0 = Machine->gfx[0];
	struct GfxElement *gfx2 = Machine->gfx[2];
	int code, y, x, ix;

	/* "colorize" each code */
	for (code = 0; code < gfx0->total_elements; code++)
	{
		const UINT8 *coldata = colordatabase + code * 32;
		UINT8 *gfxdata0 = gfx0->gfxdata + code * gfx0->char_modulo;
		UINT8 *gfxdata2 = gfx2->gfxdata + code * gfx2->char_modulo;

		/* assume 16 rows */
		for (y = 0; y < 16; y++)
		{
			const UINT8 *cd = coldata;
			UINT8 *gd0 = gfxdata0;
			UINT8 *gd2 = gfxdata2;

			/* 16 colums, in batches of 4 pixels */
			for (x = 0; x < 16; x += 4)
			{
				int pen0 = *cd++;
				int pen1 = *cd++;
				int tp0, tp1;

				/* every 4 pixels gets its own foreground/background colors */
				for (ix = 0; ix < 4; ix++, gd0++)
					*gd0 = *gd0 ? pen1 : pen0;

				/* for gfx 2, we convert all low-priority pens to 0 */
				tp0 = (pen0 & 0x80) ? pen0 : 0;
				tp1 = (pen1 & 0x80) ? pen1 : 0;
				for (ix = 0; ix < 4; ix++, gd2++)
					*gd2 = *gd2 ? tp1 : tp0;
			}

			/* advance */
			if (y % 4 == 3) coldata = cd;
			gfxdata0 += gfx0->line_modulo;
			gfxdata2 += gfx2->line_modulo;
		}
	}
}



/*************************************
 *
 *	Zwackery background update
 *
 *************************************/

static void zwackery_update_background(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int overrender)
{
	int offs;

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = (videoram_size / 2) - 1; offs >= 0; offs--)
	{
		/* this works for overrendering as well, since the sprite code will mark */
		/* intersecting tiles for us */
		if (dirtybuffer[offs])
		{
			int data = videoram16[offs];
			int mx = offs % 32;
			int my = offs / 32;
			int color = (data >> 13) & 7;
			int code = data & 0x3ff;

			/* standard case: draw with no transparency */
			if (!overrender)
				drawgfx(bitmap, Machine->gfx[0], code, color, data & 0x0800, data & 0x1000,
						16 * mx, 16 * my, &Machine->visible_area, TRANSPARENCY_NONE, 0);

			/* overrender case: for non-zero colors, draw with transparency pen 0 */
			/* we use gfx[2] here, which was generated above to have all low-priority */
			/* colors set to pen 0 */
			else if (color != 0)
				drawgfx(bitmap, Machine->gfx[2], code, color, data & 0x0800, data & 0x1000,
						16 * mx, 16 * my, &Machine->visible_area, TRANSPARENCY_PEN, 0);

			/* only clear the dirty flag if we're not overrendering */
			dirtybuffer[offs] = 0;
		}
	}
}



/*************************************
 *
 *	Sprite update
 *
 *************************************/

static void zwackery_update_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int priority)
{
	int offs;

	fillbitmap(priority_bitmap,1,cliprect);

	/* loop over sprite RAM */
	for (offs = spriteram_size / 2 - 4;offs >= 0;offs -= 4)
	{
		int code, color, flipx, flipy, x, y, sx, sy, xcount, ycount, flags;

		/* get the code and skip if zero */
		code = LOW_BYTE(spriteram16[offs + 2]);
		if (code == 0)
			continue;

		/* extract the flag bits and determine the color */
		flags = LOW_BYTE(spriteram16[offs + 1]);
		color = ((~flags >> 2) & 0x0f) | ((flags & 0x02) << 3);

		/* for low priority, draw everything but color 7 */
		if (!priority)
		{
			if (color == 7)
				continue;
		}

		/* for high priority, only draw color 7 */
		else
		{
			if (color != 7)
				continue;
		}

		/* determine flipping and coordinates */
		flipx = ~flags & 0x40;
		flipy = flags & 0x80;
		x = (231 - LOW_BYTE(spriteram16[offs + 3])) * 2;
		y = (241 - LOW_BYTE(spriteram16[offs])) * 2;

		if (x <= -32) x += 512;

		/* sprites use color 0 for background pen and 8 for the 'under tile' pen.
			The color 8 is used to cover over other sprites. */

		/* first draw the sprite, visible */
		pdrawgfx(bitmap, Machine->gfx[1], code, color, flipx, flipy, x, y,
				cliprect, TRANSPARENCY_PENS, 0x0101, 0x00);

		/* then draw the mask, behind the background but obscuring following sprites */
		pdrawgfx(bitmap, Machine->gfx[1], code, color, flipx, flipy, x, y,
				cliprect, TRANSPARENCY_PENS, 0xfeff, 0x02);

		/* mark tiles underneath as dirty for overrendering */
		if (priority == 0)
		{
			sx = x / 16;
			sy = y / 16;
			xcount = (x & 15) ? 3 : 2;
			ycount = (y & 15) ? 3 : 2;

			for (y = sy; y < sy + ycount; y++)
				for (x = sx; x < sx + xcount; x++)
					dirtybuffer[32 * (y & 31) + (x & 31)] = 1;
		}
	}
}



/*************************************
 *
 *	Zwackery MCR/68k update
 *
 *************************************/

VIDEO_UPDATE( zwackery )
{
	/* draw the background */
	zwackery_update_background(tmpbitmap, &Machine->visible_area, 0);

	/* copy it to the destination */
	copybitmap(bitmap, tmpbitmap, 0, 0, 0, 0, cliprect, TRANSPARENCY_NONE, 0);

	/* draw the low-priority sprites */
	zwackery_update_sprites(bitmap, cliprect, 0);

	/* draw the background */
	zwackery_update_background(bitmap, cliprect, 1);

	/* draw the high-priority sprites */
	zwackery_update_sprites(bitmap, cliprect, 1);
}
