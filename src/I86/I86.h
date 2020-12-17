/****************************************************************************/
/*                   i86 emulator by Fabrice Frances                        */
/*           (initial work based on David Hedley's pcemu)                   */
/*                                                                          */
/****************************************************************************/

typedef enum { ES, CS, SS, DS } SREGS;
typedef enum { AX, CX, DX, BX, SP, BP, SI, DI } WREGS;

#ifdef LSB_FIRST
typedef enum { AL,AH,CL,CH,DL,DH,BL,BH,SPL,SPH,BPL,BPH,SIL,SIH,DIL,DIH } BREGS;
#else
typedef enum { AH,AL,CH,CL,DH,DL,BH,BL,SPH,SPL,BPH,BPL,SIH,SIL,DIH,DIL } BREGS;
#endif

/* parameter x = result, y = source 1, z = source 2 */

#define SetTF(x)        (TF = (x))
#define SetIF(x)        (IF = (x))
#define SetDF(x)        (DF = (x))

#define SetOFW_Add(x,y,z)   (OverVal = ((x) ^ (y)) & ((x) ^ (z)) & 0x8000)
#define SetOFB_Add(x,y,z)   (OverVal = ((x) ^ (y)) & ((x) ^ (z)) & 0x80)
#define SetOFW_Sub(x,y,z)   (OverVal = ((z) ^ (y)) & ((z) ^ (x)) & 0x8000)
#define SetOFB_Sub(x,y,z)   (OverVal = ((z) ^ (y)) & ((z) ^ (x)) & 0x80)

#define SetCFB(x)       (CarryVal = (x) & 0x100)
#define SetCFW(x)       (CarryVal = (x) & 0x10000)
#define SetAF(x,y,z)    (AuxVal = ((x) ^ ((y) ^ (z))) & 0x10)
#define SetSF(x)        (SignVal = (x))
#define SetZF(x)        (ZeroVal = (x))
#define SetPF(x)        (ParityVal = (x))

#define SetSZPF_Byte(x) (SetSF((INT8)(x)),SetZF((INT8)(x)),SetPF(x))
#define SetSZPF_Word(x) (SetSF((INT16)(x)),SetZF((INT16)(x)),SetPF(x))

#define CF      (CarryVal!=0)
#define SF      (SignVal<0)
#define ZF      (ZeroVal==0)
#define PF      parity_table[(BYTE)ParityVal]
#define AF      (AuxVal!=0)
#define OF      (OverVal!=0)
/************************************************************************/

/* drop lines A16-A19 for a 64KB memory (yes, I know this should be done after adding the offset 8-) */
#define SegBase(Seg) ((sregs[Seg] << 4) & 0xFFFF)

#define GetMemB(Seg,Off) (cycle_count-=6,(BYTE)cpu_readmem((Seg)+(Off)))
#define GetMemW(Seg,Off) (cycle_count-=10,(WORD)GetMemB(Seg,Off)+(WORD)(GetMemB(Seg,(Off)+1)<<8))
#define PutMemB(Seg,Off,x) { cycle_count-=7; cpu_writemem((Seg)+(Off),(x)); }
#define PutMemW(Seg,Off,x) { cycle_count-=11; PutMemB(Seg,Off,(BYTE)(x)); PutMemB(Seg,(Off)+1,(BYTE)((x)>>8)); }

#define ReadByte(ea) (cycle_count-=6,(BYTE)cpu_readmem(ea))
#define ReadWord(ea) (cycle_count-=10,cpu_readmem(ea)+(cpu_readmem((ea)+1)<<8))
#define WriteByte(ea,val) { cycle_count-=7; cpu_writemem(ea,val); }
#define WriteWord(ea,val) { cycle_count-=11; cpu_writemem(ea,(BYTE)(val)); cpu_writemem(ea+1,(val)>>8); }

#define read_port(port) cpu_readport(port)
#define write_port(port,val) cpu_writeport(port,val)

/* no need to go through cpu_readmem for these ones... */
#define FETCH ((BYTE)Memory[base[CS]+ip++])
#define FETCHWORD(var) { var=Memory[base[CS]+ip]+(Memory[base[CS]+ip+1]<<8); ip+=2; }
#define PUSH(val) { WORD tmptmp=val; cycle_count-=11; regs.w[SP]-=2; \
		Memory[base[SS]+regs.w[SP]]=tmptmp; \
		Memory[base[SS]+regs.w[SP]+1]=tmptmp>>8; }
#define POP(var) { WORD tmptmp=regs.w[SP]; cycle_count-=10; regs.w[SP]+=2; \
		var=Memory[base[SS]+tmptmp]+(Memory[base[SS]+tmptmp+1]<<8); }
/************************************************************************/
#define CompressFlags() (WORD)(CF | (PF << 2) | (AF << 4) | (ZF << 6) \
			    | (SF << 7) | (TF << 8) | (IF << 9) \
			    | (DF << 10) | (OF << 11))

#define ExpandFlags(f) \
{ \
      CarryVal = (f) & 1; \
      ParityVal = !((f) & 4); \
      AuxVal = (f) & 16; \
      ZeroVal = !((f) & 64); \
      SignVal = (f) & 128 ? -1 : 0; \
      TF = ((f) & 256) == 256; \
      IF = ((f) & 512) == 512; \
      DF = ((f) & 1024) == 1024; \
      OverVal = (f) & 2048; \
}
