#include "driver.h"
#include "M6502.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



static struct AY8910interface interface =
{
	2,	/* 2 chips */
	1,	/* 1 update per video frame (low quality) */
	1536000000,	/* 1 MHZ ? */
	{ 255, 255 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};



static int interrupt_enable;


void btime_sh_interrupt_enable_w(int offset,int data)
{
	interrupt_enable = data;
}



int btime_sh_interrupt(void)
{
	if (cpu_getiloops() % 2 == 0)
	{
		if (interrupt_enable != 0) return nmi_interrupt();
		else return ignore_interrupt();
	}
	else
	{
		if (pending_commands) return interrupt();
		else return ignore_interrupt();
	}
}



int btime_sh_start(void)
{
	pending_commands = 0;

	return AY8910_sh_start(&interface);
}
