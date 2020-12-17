#include "cpudefs.h"
#ifndef macintosh /* LBO - I think this next line can be removed */
#include <go32.h>
#endif
#include "driver.h"
#include "cpuintrf.h"
#include "readcpu.h"
#include <stdio.h>

/* LBO 090597 - added these lines and made them extern in cpudefs.h */
int movem_index1[256];
int movem_index2[256];
int movem_next[256];
UBYTE *actadr;

regstruct regs, lastint_regs;

union flagu intel_flag_lookup[256];
union flagu regflags;

extern int cpu_interrupt(void);
extern MC68000_disasm(CPTR, CPTR*, int);
extern void BuildCPU(void);

#define MC68000_interrupt() (cpu_interrupt())
#define ReadMEM(A) (cpu_readmem(A))
#define WriteMEM(A,V) (cpu_writemem(A,V))

static int icount=0;
static int MC68000_IPeriod=0;

static int InitStatus=0;

extern FILE * errorlog;



void m68k_dumpstate()
{
   int i;
   CPTR nextpc;
   for(i = 0; i < 8; i++){
      printf("D%d: %08x ", i, regs.d[i]);
      if ((i & 3) == 3) printf("\n");
   }
   for(i = 0; i < 8; i++){
      printf("A%d: %08x ", i, regs.a[i]);
      if ((i & 3) == 3) printf("\n");
   }
    if (regs.s == 0) regs.usp = regs.a[7];
   if (regs.s && regs.m) regs.msp = regs.a[7];
   if (regs.s && regs.m == 0) regs.isp = regs.a[7];
   printf("USP=%08x ISP=%08x MSP=%08x VBR=%08x SR=%08x\n",
	  regs.usp,regs.isp,regs.msp,regs.vbr,regs.sr);
   printf ("T=%d%d S=%d M=%d N=%d Z=%d V=%d C=%d IMASK=%d\n",
	   regs.t1, regs.t0, regs.s, regs.m,
	   NFLG, ZFLG, VFLG, CFLG, regs.intmask);
   for(i = 0; i < 8; i++){
	printf("FP%d: %g ", i, regs.fp[i]);
      if ((i & 3) == 3) printf("\n");
   }
   printf("N=%d Z=%d I=%d NAN=%d\n",
	  (regs.fpsr & 0x8000000) != 0,
	  (regs.fpsr & 0x4000000) != 0,
	  (regs.fpsr & 0x2000000) != 0,
	  (regs.fpsr & 0x1000000) != 0);
   MC68000_disasm(m68k_getpc(), &nextpc, 1);
   printf("Next PC = 0x%0x\n", nextpc);
}





#ifdef ASM_MEMORY
UBYTE get_byte(LONG a);
void put_byte(LONG a, UBYTE b);
void put_word(LONG a, UWORD b);
UWORD get_word(LONG a);
void put_long(LONG a, ULONG b);
ULONG get_long(LONG a);

#else

UBYTE get_byte(LONG a) {
   return( ReadMEM(a) );
}

void put_byte(LONG a, UBYTE b) {
   WriteMEM(a, b);
}

UWORD get_word(LONG a) {

     return ( (ReadMEM(a)<<8) | ReadMEM(a+1) );
}

void put_word(LONG a, UWORD b) {
   WriteMEM(a, (UBYTE)((b&0xFF00)>>8));
   WriteMEM(a+1, (UBYTE)(b&0x00FF));
}

ULONG get_long(LONG a) {

     return ( (ReadMEM(a)<<24) + (ReadMEM(a+1)<<16)
              + (ReadMEM(a+2)<<8) + ReadMEM(a+3) );

}

void put_long(LONG a, ULONG b) {
   WriteMEM(a, (UBYTE)((b&0xFF000000)>>24));
   WriteMEM(a+1, (UBYTE)((b&0x00FF0000)>>16));
   WriteMEM(a+2, (UBYTE)((b&0x0000FF00)>>8));
   WriteMEM(a+3, (UBYTE)(b&0x000000FF));
}

#endif


/*
UWORD get_word(LONG a) {
   UBYTE bank=(a&0xFF0000)>>16;
   UWORD offs=(a&0xFFFF);
   if (bank==0xC4) CheckInput(offs);
   return((MRAM[bank][offs]*256)+MRAM[bank][offs+1]);
}
*/







static void initCPU(void)
{
    int i,j;

    for (i = 0 ; i < 256 ; i++) {
      for (j = 0 ; j < 8 ; j++) {
       if (i & (1 << j)) break;
      }
     movem_index1[i] = j;
     movem_index2[i] = 7-j;
     movem_next[i] = i & (~(1 << j));
    }
    for (i = 0; i < 256; i++) {
         intel_flag_lookup[i].flags.c = !!(i & 1);
         intel_flag_lookup[i].flags.z = !!(i & 64);
         intel_flag_lookup[i].flags.n = !!(i & 128);
         intel_flag_lookup[i].flags.v = 0;
    }

}

void inline Exception(int nr, CPTR oldpc)
{
   MakeSR();

   if(!regs.s) {
      regs.a[7]=regs.isp;
      regs.s=1;
   }
   
   regs.a[7] -= 4;
   put_long (regs.a[7], m68k_getpc ());
   regs.a[7] -= 2;
   put_word (regs.a[7], regs.sr);
   m68k_setpc(get_long(regs.vbr + 4*nr));

   regs.t1 = regs.t0 = regs.m = 0;
}

void inline Interrupt68k(int level)
{
   int ipl=(regs.sr&0xf00)>>8;
   if(level>=ipl) Exception(24+level,0);
}

void Initialisation() {
   /* Init 68000 emulator */
   BuildCPU();
   initCPU();
}

void MC68000_reset(long IPeriod)
{
if (!InitStatus)
{
	Initialisation();
	InitStatus=1;
}

  MC68000_IPeriod = IPeriod;
  icount = IPeriod;

   regs.a[7]=get_long(0);
   m68k_setpc(get_long(4));

   regs.s = 1;
   regs.m = 0;
   regs.stopped = 0;
   regs.t1 = 0;
   regs.t0 = 0;
   ZFLG = CFLG = NFLG = VFLG = 0;
   regs.intmask = 7;
   regs.vbr = regs.sfc = regs.dfc = 0;
   regs.fpcr = regs.fpsr = regs.fpiar = 0;
}




/* Execute one 68000 instruction */

void MC68000_Execute(void)
{
        UWORD opcode;

	do
	{
	      #ifdef ASM_MEMORY
		opcode=nextiword_opcode();
	      #else
		opcode=nextiword();
	      #endif   

		icount -= 15;
		cpufunctbl[opcode](opcode);

	}
	while (icount>0);
	icount += MC68000_IPeriod;
        Interrupt68k(MC68000_interrupt());
}








UBYTE *allocmem(long size, UBYTE init_value) {
   UBYTE *p;
   p=(UBYTE *)malloc(size);
   if (!p) {
      printf("No enough memory !\n");
   }
   return(p);
}

