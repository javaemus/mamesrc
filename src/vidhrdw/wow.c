/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "Z80.h"

void Gorf_CopyLine(int Line);

unsigned char *wow_videoram;

int ColourSplit=0;
int magic_expand_color, magic_control, collision;

/* Gorf Star Screen & other arrays */

#define MAX_STARS 750

int StarColour[8] =
{
	0,1,2,3,3,2,1,0
};

static int total_stars;

struct star
{
	int x,y,colour;
};

static struct star stars[MAX_STARS];

/*
 * Default colours for WOW
 *
 * It doesn't set them up for initial screen
 */

int Colour[8] =
{
	0,0,0,0,0xC7,0xF3,0x7C,0x51
};

/* Latch Registers */

int Latch[16] =
{
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};


int wow_intercept_r(int offset)
{
	int res;

	res = collision;
	collision = 0;

	return res;
}

int wow_video_retrace_r(int offset)
{
	extern int CurrentScan;

    return CurrentScan;
}

/* Doesn't seem to work (i.e. I don't fully understand it!) */

void colour_split_w(int offset, int data)
{
/*
	if(ColourSplit != data)
    {
        ColourSplit = data;
        memset(dirtybuffer,1,videoram_size);
    }
*/
    if (errorlog) fprintf(errorlog,"Colour split set to %02x\n",data);
}

void colour_register_w(int offset, int data)
{
	if(Colour[offset] != data)
    {
		Colour[offset] = data;
        memset(dirtybuffer,1,videoram_size);
    }

    if (errorlog) fprintf(errorlog,"Colour %01x set to %02x\n",offset,data);
}

void wow_videoram_w(int offset,int data)
{
	if ((offset < 0x4000) && (wow_videoram[offset] != data))
	{
		wow_videoram[offset] = data;
        dirtybuffer[offset] = 1;
    }
}

void wow_magic_expand_color_w(int offset,int data)
{
if (errorlog) fprintf(errorlog,"%04x: magic_expand_color = %02x\n",cpu_getpc(),data);
	magic_expand_color = data;
}


void wow_magic_control_w(int offset,int data)
{
if (errorlog) fprintf(errorlog,"%04x: magic_control = %02x\n",cpu_getpc(),data);
	magic_control = data;
}


static void copywithflip(int offset,int data)
{
	if (magic_control & 0x40)	/* copy backwards */
	{
		int bits,stib,k;

		bits = data;
		stib = 0;
		for (k = 0;k < 4;k++)
		{
			stib >>= 2;
			stib |= (bits & 0xc0);
			bits <<= 2;
		}

		data = stib;
	}

	if (magic_control & 0x40)	/* copy backwards */
	{
		int shift,data1,mask;


		shift = magic_control & 3;
		data1 = 0;
		mask = 0xff;
		while (shift > 0)
		{
			data1 <<= 2;
			data1 |= (data & 0xc0) >> 6;
			data <<= 2;
			mask <<= 2;
			shift--;
		}

		if (magic_control & 0x30)
		{
			/* TODO: the collision detection should be made independently for */
			/* each of the four pixels */
			if ((mask & wow_videoram[offset]) || (~mask & wow_videoram[offset-1]))
				collision |= 0xff;
			else collision &= 0x0f;
		}

		if (magic_control & 0x20) data ^= wow_videoram[offset];	/* draw in XOR mode */
		else if (magic_control & 0x10) data |= wow_videoram[offset];	/* draw in OR mode */
		else data |= ~mask & wow_videoram[offset];	/* draw in copy mode */
		wow_videoram_w(offset,data);
		if (magic_control & 0x20) data1 ^= wow_videoram[offset-1];	/* draw in XOR mode */
		else if (magic_control & 0x10) data1 |= wow_videoram[offset-1];	/* draw in OR mode */
		else data1 |= mask & wow_videoram[offset-1];	/* draw in copy mode */
		wow_videoram_w(offset-1,data1);
	}
	else
	{
		int shift,data1,mask;


		shift = magic_control & 3;
		data1 = 0;
		mask = 0xff;
		while (shift > 0)
		{
			data1 >>= 2;
			data1 |= (data & 0x03) << 6;
			data >>= 2;
			mask >>= 2;
			shift--;
		}

		if (magic_control & 0x30)
		{
			/* TODO: the collision detection should be made independently for */
			/* each of the four pixels */
			if ((mask & wow_videoram[offset]) || (~mask & wow_videoram[offset+1]))
				collision |= 0xff;
			else collision &= 0x0f;
		}

		if (magic_control & 0x20)
			data ^= wow_videoram[offset];	/* draw in XOR mode */
		else if (magic_control & 0x10)
			data |= wow_videoram[offset];	/* draw in OR mode */
		else
			data |= ~mask & wow_videoram[offset];	/* draw in copy mode */
		wow_videoram_w(offset,data);
		if (magic_control & 0x20)
			data1 ^= wow_videoram[offset+1];	/* draw in XOR mode */
		else if (magic_control & 0x10)
			data1 |= wow_videoram[offset+1];	/* draw in OR mode */
		else
			data1 |= mask & wow_videoram[offset+1];	/* draw in copy mode */
		wow_videoram_w(offset+1,data1);
	}
}

void wow_magicram_w(int offset,int data)
{
	if (magic_control & 0x08)	/* expand mode */
	{
		int bits,bibits,k;
		static int count;


		bits = data;
		if (count) bits <<= 4;
		bibits = 0;
		for (k = 0;k < 4;k++)
		{
			bibits <<= 2;
			if (bits & 0x80) bibits |= (magic_expand_color >> 2) & 0x03;
			else bibits |= magic_expand_color & 0x03;
			bits <<= 1;
		}

		copywithflip(offset,bibits);

		count ^= 1;
	}
	else copywithflip(offset,data);
}


void wow_pattern_board_w(int offset,int data)
{
	static int src;
	static int mode;	/*  bit 0 = direction
							bit 1 = expand mode
							bit 2 = constant
							bit 3 = flush
							bit 4 = flip
							bit 5 = flop */
	static int skip;	/* bytes to skip after row copy */
	static int dest;
	static int length;	/* row length */
	static int loops;	/* rows to copy - 1 */

	switch (offset)
	{
		case 0:
			src = data;
			break;
		case 1:
			src = src + data * 256;
			break;
		case 2:
			mode = data & 0x3f;			/* register is 6 bit wide */
			break;
		case 3:
			skip = data;
			break;
		case 4:
			dest = skip + data * 256;	/* register 3 is shared between skip and dest */
			break;
		case 5:
			length = data;
			break;
		case 6:
			loops = data;
			break;
	}

	if (offset == 6)	/* trigger blit */
	{
		int i,j;

		if (errorlog) fprintf(errorlog,"%04x: blit src %04x mode %02x skip %d dest %04x length %d loops %d\n",
			cpu_getpc(),src,mode,skip,dest,length,loops);

        /* Special scroll screen for Gorf */

        if (src==(dest+0x4000))
        {
        	if(dest==0)
            {
				for (i=0x3FFF;i>=0;i--) wow_magicram_w(i,RAM[i+0x4000]);

                /* Redraw screen to avoid tears */

                for (i=203;i>=0;i--) Gorf_CopyLine(i);

		        /* Cycle Steal (slow scroll down!) */

		        Z80_ICount -= 65336;
            }
        }
        else
        {
		    for (i = 0; i <= loops;i++)
		    {
			    for (j = 0;j <= length;j++)
			    {
				    if (!(mode & 0x08) || j < length)
                        if (mode & 0x01)			/* Direction */
						    RAM[src]=RAM[dest];
                        else
						    if (dest >= 0) cpu_writemem16(dest,RAM[src]);	/* ASG 971005 */

				    if ((j & 1) || !(mode & 0x02))  /* Expand Mode - don't increment source on odd loops */
					    if (mode & 0x04) src++;		/* Constant mode - don't increment at all! */

				    if (mode & 0x20) dest++;		/* copy forwards */
				    else dest--;					/* backwards */
			    }

			    if ((j & 1) && (mode & 0x02))	    /* always increment source at end of line */
				    if (mode & 0x04) src++;			/* Constant mode - don't increment at all! */

			    if ((mode & 0x08) && (mode & 0x04)) /* Correct src if in flush mode */
				    src--;                          /* and NOT in Constant mode */

			    if (mode & 0x20) dest--;			/* copy forwards */
			    else dest++;						/* backwards */

			    dest += (int)((signed char)skip);	/* extend the sign of the skip register */

		    /* Note: actually the hardware doesn't handle the sign of the skip register, */
		    /* when incrementing the destination address the carry bit is taken from the */
		    /* mode register. To faithfully emulate the hardware I should do: */
#if 0
			    {
				    int lo,hi;

				    lo = dest & 0x00ff;
				    hi = dest & 0xff00;
				    lo += skip;
				    if (mode & 0x10)
				    {
					    if (lo < 0x100) hi -= 0x100;
				    }
				    else
				    {
					    if (lo > 0xff) hi += 0x100;
				    }
				    dest = hi | (lo & 0xff);
			    }
#endif
		    }
    	}
	}
}


/* GORF Special Registers
 *
 * These are data writes, done by IN commands
 *
 * The data is placed on the upper bits 8-11 bits of the address bus (B)
 * and is used to drive 2 8 bit addressable latches to control :-
 *
 * IO 15
 *   0
 *   1
 *   2 Star Field Blue/Black (it's always on)
 *   3 Sparkle 1
 *   4 Sparkle 2
 *   5 Sparkle 3
 *   6 Second Amp On/Off ?
 *   7 Drv7
 *
 * IO 16
 *   0
 *   1
 *   2
 *   3
 *   4
 *   5
 *   6
 *   7 Space Cadet Light ?
 *
 */

int Gorf_IO_r(int offset)
{
	Z80_Regs regs;
	int data;

	Z80_GetRegs(&regs);
	data = regs.BC.B.h & 0x0F;

    Latch[(offset << 3) + (data >> 1)] = (data & 0x01);

    if (errorlog) fprintf(errorlog,"Gorf Latch IO %02x set to %d (%02x)\n",(offset << 3) + (data >> 1),data & 0x01,data);

    return data;			/* Probably not used */
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

/****************************************************************************
 * Gorf specific routines
 ****************************************************************************/

int gorf_vh_start(void)
{
	int generator;
	int x,y;

	if (generic_vh_start() != 0)
		return 1;


	/* precalculate the star background */

	total_stars = 0;
	generator = 0;

	for (y = 319;y >= 0;y--)
	{
		for (x = 203;x >= 0;x--)
		{
			int bit1,bit2;

			generator <<= 1;
			bit1 = (~generator >> 17) & 1;
			bit2 = (generator >> 5) & 1;

			if (bit1 ^ bit2) generator |= 1;

			if (y >= Machine->drv->visible_area.min_y &&
				y <= Machine->drv->visible_area.max_y &&
				((~generator >> 16) & 1) &&
				(generator & 0x3f) == 0x3f)
			{
				int color;

				color = (~(generator >> 8)) & 0x07;
				if (color && (total_stars < MAX_STARS))
				{
               	    int ex,ey;

                    ex = x;
                    ey = y;

                    if (Machine->orientation & ORIENTATION_SWAP_XY)
                    {
                    	ey = x;
                        ex = y;
                    }

					stars[total_stars].x      = ex;
					stars[total_stars].y      = ey;
					stars[total_stars].colour = color-1;

					total_stars++;
				}
			}
		}
	}

	return 0;
}

void Gorf_CopyLine(int Line)
{
	/* Copy one line to bitmap, using current colour register settings */

    int memloc;
    int i,x;
    int data,color;

    memloc = Line * 80;

    for(i=0;i<80;i++,memloc++)
    {
    	if(dirtybuffer[memloc])
        {
			data = wow_videoram[memloc];

            for(x=316-(i*4);x<=319-(i*4);x++)
			{
            	color = data & 03;

                if (Machine->orientation & ORIENTATION_SWAP_XY)
                {
                	int ex,ey;

					if (Machine->orientation & ORIENTATION_FLIP_X)
						ex = 319 - x;
                    else
                    	ex = x;

  					if (Machine->orientation & ORIENTATION_FLIP_Y)
  						ey = 203 - Line;
                    else
                      	ey = Line;

					tmpbitmap->line[ey][ex] = Machine->pens[Colour[color]];
                }
                else
					tmpbitmap->line[x][Line] = Machine->pens[Colour[color]];

                data >>= 2;
            }

          	dirtybuffer[memloc] = 0;
        }
    }
}

void gorf_vh_screenrefresh(struct osd_bitmap *bitmap)
{
    static int Speed=0;
	int offs;

	/* copy the character mapped graphics */

  	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

    /* Plot the stars (on colour 0 only) */

    if (Colour[0] != 0)
    {
	    Speed = (Speed + 1) & 3;

	    for (offs = 0;offs < total_stars;offs++)
	    {
		    int x,y;

		    x = stars[offs].x;
		    y = stars[offs].y;

            if (errorlog) fprintf(errorlog,"X = %d, Y = %d\n",x,y);

            if (Speed==0) stars[offs].colour = (stars[offs].colour + 1) & 7;

		    if (bitmap->line[y][x] == Machine->pens[Colour[0]])
			    bitmap->line[y][x] = Machine->pens[Colour[0]+StarColour[stars[offs].colour]];
	    }
    }
}

/****************************************************************************
 * Seawolf specific routines
 ****************************************************************************/

void seawolf2_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	extern int Controller1;
	extern int Controller2;

    int x,y,centre;

	/* copy the character mapped graphics */

	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

    /* Draw a sight */

    if(RAM[0xc1fb] != 0)	/* Number of Players */
    {
    	/* Yellow sight for Player 1 */

        centre = 317 - (Controller1-18) * 10;

        if (centre<2)   centre=2;
        if (centre>317) centre=317;

        if (Machine->orientation & ORIENTATION_SWAP_XY)
        {
			if ((Machine->orientation & ORIENTATION_FLIP_X) == 0)
				centre = 319 - centre;

	        for(y=25;y<46;y++)
  			    if (Machine->orientation & ORIENTATION_FLIP_Y)
                	bitmap->line[centre][y] = Machine->pens[0x77];
                else
  				    bitmap->line[centre][203-y] = Machine->pens[0x77];

    	    for(x=centre-20;x<centre+21;x++)
        	    if((x>0) && (x<=319))
                    if (Machine->orientation & ORIENTATION_FLIP_Y)
                        bitmap->line[x][35] = Machine->pens[0x77];
                    else
  				        bitmap->line[x][203-35] = Machine->pens[0x77];
        }
        else
        {
	        for(y=25;y<46;y++) bitmap->line[y][centre] = Machine->pens[0x77];

    	    for(x=centre-20;x<centre+21;x++)
        	    if((x>0) && (x<=319)) bitmap->line[35][x] = Machine->pens[0x77];
        }

        /* Red sight for Player 2 */

        if(RAM[0xc1fb] == 2)
		{
            centre = 316 - (Controller2-18) * 10;

            if (centre<1)   centre=1;
            if (centre>316) centre=316;

            if (Machine->orientation & ORIENTATION_SWAP_XY)
            {
			    if ((Machine->orientation & ORIENTATION_FLIP_X) == 0)
				    centre = 319 - centre;

	            for(y=25;y<46;y++)
  			        if (Machine->orientation & ORIENTATION_FLIP_Y)
                	    bitmap->line[centre][y] = Machine->pens[0x58];
                    else
  				        bitmap->line[centre][203-y] = Machine->pens[0x58];

    	        for(x=centre-20;x<centre+21;x++)
        	        if((x>0) && (x<=319))
                        if (Machine->orientation & ORIENTATION_FLIP_Y)
                            bitmap->line[x][33] = Machine->pens[0x58];
                        else
  				            bitmap->line[x][203-33] = Machine->pens[0x58];
            }
            else
            {
	            for(y=25;y<46;y++) bitmap->line[y][centre] = Machine->pens[0x58];

    	        for(x=centre-20;x<centre+21;x++)
        	        if((x>0) && (x<=319)) bitmap->line[33][x] = Machine->pens[0x58];
            }
        }
    }
}

/****************************************************************************
 * Wizard of Wor
 ****************************************************************************/

int wow_vh_start(void)
{
	int generator;
	int x,y;

	if (generic_vh_start() != 0)
		return 1;


	/* precalculate the star background */

	total_stars = 0;
	generator = 0;

	for (y = 203;y >= 0;y--)
	{
		for (x = 319;x >= 0;x--)
		{
			int bit1,bit2;

			generator <<= 1;
			bit1 = (~generator >> 17) & 1;
			bit2 = (generator >> 5) & 1;

			if (bit1 ^ bit2) generator |= 1;

			if (y >= Machine->drv->visible_area.min_y &&
				y <= Machine->drv->visible_area.max_y &&
				((~generator >> 16) & 1) &&
				(generator & 0x3f) == 0x3f)
			{
				int color;

				color = (~(generator >> 8)) & 0x07;
				if (color && (total_stars < MAX_STARS))
				{
               	    int ex,ey;

                    ex = x;
                    ey = y;

                    if (Machine->orientation & ORIENTATION_SWAP_XY)
                    {
                    	ey = x;
                        ex = y;
                    }

					stars[total_stars].x      = ex;
					stars[total_stars].y      = ey;
					stars[total_stars].colour = color-1;

					total_stars++;
				}
			}
		}
	}

	return 0;
}

void wow_vh_screenrefresh_stars(struct osd_bitmap *bitmap)
{
    static int Speed=0;
	int offs;

	/* copy the character mapped graphics */

  	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

    /* Plot the stars (on colour 4 only) */

    if (Colour[4] != 0)
    {
	    Speed = (Speed + 1) & 3;

	    for (offs = 0;offs < total_stars;offs++)
	    {
		    int x,y;

		    x = stars[offs].x;
		    y = stars[offs].y;

            if (Speed==0) stars[offs].colour = (stars[offs].colour + 1) & 7;

		    if (bitmap->line[y][x] == Machine->pens[Colour[4]])
			    bitmap->line[y][x] = Machine->pens[Colour[4]+StarColour[stars[offs].colour]];
	    }
    }
}

/****************************************************************************
 * Standard WOW routines
 ****************************************************************************/

void CopyLine(int Line)
{
	/* Copy one line to bitmap, using current colour register settings */

    int memloc;
    int i,x;
    int data,color;

    memloc = Line * 80;

    for(i=0;i<80;i++,memloc++)
    {
      	if(dirtybuffer[memloc])
        {
        	dirtybuffer[memloc] = 0;
			data = wow_videoram[memloc];

            for(x=i*4+3;x>=i*4;x--)
            {
            	color = (data & 03) + 4;

                if (Machine->orientation & ORIENTATION_SWAP_XY)
                {
                	int ex,ey;

					if (Machine->orientation & ORIENTATION_FLIP_X)
						ex = x;
                    else
                    	ex = 319 - x;

  					if (Machine->orientation & ORIENTATION_FLIP_Y)
  						ey = Line;
                    else
                      	ey = 203 - Line;

					tmpbitmap->line[ex][ey] = Machine->pens[Colour[color]];
                }
                else
					tmpbitmap->line[Line][x] = Machine->pens[Colour[color]];

                data >>= 2;
            }
        }
    }
}

void wow_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	/* copy the character mapped graphics */

	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}

