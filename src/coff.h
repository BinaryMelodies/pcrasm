#ifndef _COFF_H
#define _COFF_H

#define I386MAGIC 0x014C
#define AMD64MAGIC 0x8664
#define MC68MAGIC 0x0150
#define MC68MAGIC_PE 0x0268
#define Z80MAGIC 0x805A // GNU binutils
#define W65MAGIC 0x6500 // GNU binutils
#define Z8KMAGIC 0x8000 // GNU binutils

#define N_UNDEF 0
#define N_ABS (-1)
#define N_DEBUG (-2)

#define C_EXT 2
#define C_STAT 3
#define C_LABEL 6
#define C_FILE 103 /* 0x67 */

// x86
#define R_ABS 0
#define R_DIR16 1 /* 286 only */
#define R_REL16 2 /* 286 only */
#define R_DIR32 6
#define R_SEG12 9 /* 286 only */
#define R_PCRLONG 20 /* 386 only */
#define R_REL32 R_PCRLONG

#define IMAGE_REL_AMD64_ADDR64 1
#define IMAGE_REL_AMD64_ADDR32 2
#define IMAGE_REL_AMD64_REL32 4 /* +1, +2, +3, +4, +5 valid */

// z8k, z80
#define R_IMM8 34
#define R_OFF8 50
#define R_JR 2
#define R_IMM16 1
#define R_IMM24 51
#define R_IMM32 17

// w65
#define R_W65_ABS8 1
#define R_W65_ABS8S8 4
#define R_W65_ABS8S16 5
#define R_W65_PCR8 8
#define R_W65_DP 10
#define R_W65_ABS16 2
#define R_W65_PCR16 9
#define R_W65_ABS24 3

void coff_generate(const char * input_filename);

#endif // _COFF_H
