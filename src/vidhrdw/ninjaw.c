#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"

#define TC0100SCN_GFX_NUM 1

struct tempsprite
{
	int gfx;
	int code,color;
	int flipx,flipy;
	int x,y;
	int zoomx,zoomy;
	int primask;
};
static struct tempsprite *spritelist;

static int taito_hide_pixels;



/**********************************************************/

static VIDEO_START( ninjaw_core )
{
	int chips;

	spritelist = auto_malloc(0x1000 * sizeof(*spritelist));
	if (!spritelist)
		return 1;

	chips = number_of_TC0100SCN();

	if (chips <= 0)	/* we have an erroneous TC0100SCN configuration */
		return 1;

	if (TC0100SCN_vh_start(chips,TC0100SCN_GFX_NUM,taito_hide_pixels,0,0,0,0,0,0))
		return 1;

	if (has_TC0110PCR())
		if (TC0110PCR_vh_start())
			return 1;

	if (has_second_TC0110PCR())
		if (TC0110PCR_1_vh_start())
			return 1;

	if (has_third_TC0110PCR())
		if (TC0110PCR_2_vh_start())
			return 1;

	/* Ensure palette from correct TC0110PCR used for each screen */
	TC0100SCN_set_chip_colbanks(0x0,0x100,0x200);

	return 0;
}

VIDEO_START( ninjaw )
{
	taito_hide_pixels = 22;
	return video_start_ninjaw_core();
}

/************************************************************
			SPRITE DRAW ROUTINE
************************************************************/

static void ninjaw_draw_sprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int *primasks,int y_offs)
{
	int offs, data, tilenum, color, flipx, flipy;
	int x, y, priority, invis, curx, cury;
	int code;

#ifdef MAME_DEBUG
	int unknown=0;
#endif

	/* pdrawgfx() needs us to draw sprites front to back, so we have to build a list
	   while processing sprite ram and then draw them all at the end */
	struct tempsprite *sprite_ptr = spritelist;

	for (offs = (spriteram_size/2)-4;offs >=0;offs -= 4)
	{
		data = spriteram16[offs+2];
		tilenum = data & 0x7fff;

		if (!tilenum) continue;

		data = spriteram16[offs+0];
//		x = (data - 8) & 0x3ff;
		x = (data - 32) & 0x3ff;	/* aligns sprites on rock outcrops and sewer hole */

		data = spriteram16[offs+1];
		y = (data - 0) & 0x1ff;

		/* don't know meaning of "invis" bit (Darius: explosions of your bomb shots) */
		data = spriteram16[offs+3];
		flipx    = (data & 0x1);
		flipy    = (data & 0x2) >> 1;
		priority = (data & 0x4) >> 2;	/* 1=low */
		invis    = (data & 0x8) >> 3;
		color    = (data & 0x7f00) >> 8;

//	Ninjaw: this stops your player flickering black and cutting into tank sprites
//		if (invis && (priority==1)) continue;

#ifdef MAME_DEBUG
		if (data & 0x80f0)   unknown |= (data &0x80f0);
#endif

		y += y_offs;

		/* sprite wrap: coords become negative at high values */
		if (x>0x3c0) x -= 0x400;
		if (y>0x180) y -= 0x200;

		curx = x;
		cury = y;
		code = tilenum;

		sprite_ptr->code = code;
		sprite_ptr->color = color;
		sprite_ptr->flipx = flipx;
		sprite_ptr->flipy = flipy;
		sprite_ptr->x = curx;
		sprite_ptr->y = cury;

		if (primasks)
		{
			sprite_ptr->primask = primasks[priority];
			sprite_ptr++;
		}
		else
		{
			drawgfx(bitmap,Machine->gfx[0],
					sprite_ptr->code,
					sprite_ptr->color,
					sprite_ptr->flipx,sprite_ptr->flipy,
					sprite_ptr->x,sprite_ptr->y,
					cliprect,TRANSPARENCY_PEN,0);
		}
	}

	/* this happens only if primsks != NULL */
	while (sprite_ptr != spritelist)
	{
		sprite_ptr--;

		pdrawgfx(bitmap,Machine->gfx[0],
				sprite_ptr->code,
				sprite_ptr->color,
				sprite_ptr->flipx,sprite_ptr->flipy,
				sprite_ptr->x,sprite_ptr->y,
				cliprect,TRANSPARENCY_PEN,0,
				sprite_ptr->primask);
	}

#ifdef MAME_DEBUG
	if (unknown)
	{
		char buf[80];
		sprintf(buf,"unknown sprite bits: %04x",unknown);
		usrintf_showmessage(buf);
	}
#endif
}


/**************************************************************
				SCREEN REFRESH
**************************************************************/

VIDEO_UPDATE( ninjaw )
{
	UINT8 layer[3];

	TC0100SCN_tilemap_update();

	layer[0] = TC0100SCN_bottomlayer(0);
	layer[1] = layer[0]^1;
	layer[2] = 2;

	fillbitmap(priority_bitmap,0,cliprect);

	/* Ensure screen blanked even when bottom layers not drawn due to disable bit */
	fillbitmap(bitmap, Machine->pens[0], cliprect);

	/* chip 0 does tilemaps on the left, chip 1 center, chip 2 the right */
	TC0100SCN_tilemap_draw(bitmap,cliprect,0,layer[0],TILEMAP_IGNORE_TRANSPARENCY,1);	/* left */
	TC0100SCN_tilemap_draw(bitmap,cliprect,1,layer[0],TILEMAP_IGNORE_TRANSPARENCY,1);	/* center */
	TC0100SCN_tilemap_draw(bitmap,cliprect,2,layer[0],TILEMAP_IGNORE_TRANSPARENCY,1);	/* right */
	TC0100SCN_tilemap_draw(bitmap,cliprect,0,layer[1],0,2);
	TC0100SCN_tilemap_draw(bitmap,cliprect,1,layer[1],0,2);
	TC0100SCN_tilemap_draw(bitmap,cliprect,2,layer[1],0,2);
	TC0100SCN_tilemap_draw(bitmap,cliprect,0,layer[2],0,4);
	TC0100SCN_tilemap_draw(bitmap,cliprect,1,layer[2],0,4);
	TC0100SCN_tilemap_draw(bitmap,cliprect,2,layer[2],0,4);

	/* Sprites can be under/over the layer below text layer */
	{
		int primasks[2] = {0xf0,0xfc};
		ninjaw_draw_sprites(bitmap,cliprect,primasks,8);
	}
}

