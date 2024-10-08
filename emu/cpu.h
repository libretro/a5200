#ifndef _CPU_H_
#define _CPU_H_

#include "config.h"
#include "atari.h"

#define N_FLAG 0x80
#define V_FLAG 0x40
#define B_FLAG 0x10
#define D_FLAG 0x08
#define I_FLAG 0x04
#define Z_FLAG 0x02
#define C_FLAG 0x01

void CPU_Initialise(void);		/* used in the assembler version of cpu.c only */
void CPU_GetStatus(void);
void CPU_PutStatus(void);
void CPU_Reset(void);
void NMI(void);
void GO(int limit);
#define GenerateIRQ() (IRQ = 1)

extern UWORD regPC;
extern UBYTE regA;
extern UBYTE regP;
extern UBYTE regS;
extern UBYTE regY;
extern UBYTE regX;

#define SetN regP |= N_FLAG
#define ClrN regP &= (~N_FLAG)
#define SetV regP |= V_FLAG
#define ClrV regP &= (~V_FLAG)
#define SetB regP |= B_FLAG
#define ClrB regP &= (~B_FLAG)
#define SetD regP |= D_FLAG
#define ClrD regP &= (~D_FLAG)
#define SetI regP |= I_FLAG
#define ClrI regP &= (~I_FLAG)
#define SetZ regP |= Z_FLAG
#define ClrZ regP &= (~Z_FLAG)
#define SetC regP |= C_FLAG
#define ClrC regP &= (~C_FLAG)

extern UBYTE IRQ;

#define REMEMBER_PC_STEPS 64
extern UWORD remember_PC[REMEMBER_PC_STEPS];
extern unsigned int remember_PC_curpos;
extern int remember_xpos[REMEMBER_PC_STEPS];

#define REMEMBER_JMP_STEPS 16
extern UWORD remember_JMP[REMEMBER_JMP_STEPS];
extern unsigned int remember_jmp_curpos;

#endif /* _CPU_H_ */
