/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/vector.h"
#include "Z80.h"

int omegrace_vg_start(void)
{
	if (vg_init(0x2000, USE_DVG,0))
		return 1;
	return 0;
}

int omegrace_vg_go(int data)
{
	vg_go(cpu_gettotalcycles()>>1);
	return 0;
}

int omegrace_watchdog_r(int offset)
{
	return 0;
}

int omegrace_vg_status_r(int offset)
{
/*	if (errorlog) fprintf(errorlog,"reading vg_halt\n");*/
	if (vg_done(cpu_gettotalcycles()>>1))
		return 0;
	else 
		return 0x80;
}

/*
 * Encoder bit mappings
 * The encoder is a 64 way switch, with the inputs scrambled
 * on the input port (and shifted 2 bits to the left for the
 * 1 player encoder
 *
 * 3 6 5 4 7 2 for encoder 1 (shifted two bits left..)
 *
 *
 * 5 4 3 2 1 0 for encoder 2 (not shifted..)
 */

static char spinnerTable[64] = {
	0x00, 0x04, 0x14, 0x10, 0x18, 0x1c, 0x5c, 0x58,
	0x50, 0x54, 0x44, 0x40, 0x48, 0x4c, 0x6c, 0x68,
	0x60, 0x64, 0x74, 0x70, 0x78, 0x7c, 0xfc, 0xf8,
	0xf0, 0xf4, 0xe4, 0xe0, 0xe8, 0xec, 0xcc, 0xc8,
	0xc0, 0xc4, 0xd4, 0xd0, 0xd8, 0xdc, 0x9c, 0x98,
	0x90, 0x94, 0x84, 0x80, 0x88, 0x8c, 0xac, 0xa8,
	0xa0, 0xa4, 0xb4, 0xb0, 0xb8, 0xbc, 0x3c, 0x38,
	0x30, 0x34, 0x24, 0x20, 0x28, 0x2c, 0x0c, 0x08 };

#define SPINNER_SENSITY 5 

int omegrace_spinner1_r(int offset)
{
	static int spinner=0;
	static int spintime=SPINNER_SENSITY;
	int res;

	res=readtrakport(0);
	if (res != NO_TRAK) spinner+=res;
	
	if ((spintime--) == 0) {
		res=readinputport(4);
		if (res & 0x01)
			spinner--;
		if (res & 0x02)
			spinner++;
		spintime=SPINNER_SENSITY;
	}
	return (spinnerTable[spinner & 0x3f]);
}

int omegrace_spinner2_r(int offset)
{
	if (errorlog) fprintf(errorlog,"reading spinner 2\n");
	return (0);
}
