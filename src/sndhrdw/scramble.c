#include "driver.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



static int scramble_portB_r(int offset)
{
	int clock;

#define TIMER_RATE (512*2)

	clock = cpu_gettotalcycles() / TIMER_RATE;

	clock = ((clock & 0x01) << 4) | ((clock & 0x02) << 6) |
			((clock & 0x08) << 2) | ((clock & 0x10) << 2);

	return clock;
}



void scramble_sh_irqtrigger_w(int offset,int data)
{
	static int last;


	if (last == 0 && (data & 0x08) != 0)
	{
		/* setting bit 3 low then high triggers IRQ on the sound CPU */
		cpu_cause_interrupt(1,0xff);
	}

	last = data & 0x08;
}



int scramble_sh_interrupt(void)
{
	AY8910_update();

	/* interrupts don't happen here, the handler is used only to update the 8910 */
	return ignore_interrupt();
}



static struct AY8910interface interface =
{
	2,	/* 2 chips */
	10,	/* 10 updates per video frame (good quality) */
	1789750000,	/* 1.78975 Mhz (? the crystal is 14.318 MHz) */
	{ 255, 255 },
	{ soundlatch_r },
	{ scramble_portB_r },
	{ 0 },
	{ 0 }
};



int scramble_sh_start(void)
{
	return AY8910_sh_start(&interface);
}
