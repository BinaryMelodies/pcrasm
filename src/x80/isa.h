#ifndef _X80_ISA_H
#define _X80_ISA_H

#include <stdio.h>
#include "../asm.h"
#include "../elf.h"
#include "../coff.h"
#include "../../obj/x80/x80/parser.tab.h"

#ifndef TARGET_NAME
# define TARGET_NAME "x80"
#endif

#define ARCH_BITS_IN_UNIT 8
#define ARCH_BITS_IN_CHAR 8

enum cpu_type_t
{
	CPU_DP2200 = 1,
	CPU_DP2200V2,
	CPU_8008,
	CPU_8080,
	CPU_8085,
	CPU_Z80,
	CPU_Z280, // TODO (Z280/R800 come before Z180, since Z380/eZ80 include Z180 instructions
	CPU_R800,
	CPU_Z180, // TODO
	CPU_Z380, // TODO
	CPU_EZ80, // TODO
	CPU_GBZ80, // comes at the end to include some Z80 instructions but exclude some i8080 instructions
};
typedef enum cpu_type_t cpu_type_t;

enum syntax_t
{
	SYNTAX_DP2200 = 1,
	SYNTAX_I8008,
	SYNTAX_INTEL,
	SYNTAX_ZILOG,
	SYNTAX_ASCII,
};
typedef enum syntax_t syntax_t;

enum operand_type_t
{
	OPD_IMM,
	OPD_MEM,
	OPD_TOKEN, // register or condition code (for Z80)
	OPD_HLD, // (hl-) or (hld) or [.hl--]
	OPD_HLI, // (hl+) or (hli) or [.hl++]
	OPD_DED, // [.de--]
	OPD_DEI, // [.de++]
};
typedef enum operand_type_t operand_type_t;

typedef enum regnumber_t
{
	X80_A,
	X80_AF,
	X80_AF2,
	X80_B,
	X80_BC,
	X80_C,
	X80_CND_M,
	X80_D,
	X80_DE,
	X80_E,
	X80_F,
	X80_H,
	X80_HL,
	X80_HLD,
	X80_HLI,
	X80_I,
	X80_IX,
	X80_IXH,
	X80_IXL,
	X80_IY,
	X80_IYH,
	X80_IYL,
	X80_L,
	X80_NC,
	X80_NZ,
	X80_P,
	X80_PE,
	X80_PO,
	X80_PSW,
	X80_R,
	X80_REG_M,
	X80_SP,
	X80_Z,

	REG_NONE = -1,
} regnumber_t;

enum condition_t
{
	X80_COND_NZ = 0,
	X80_COND_Z,
	X80_COND_NC,
	X80_COND_C,
	X80_COND_PO,
	X80_COND_PE,
	X80_COND_P,
	X80_COND_M,

	COND_NONE = -1,
};
typedef enum condition_t condition_t;

#define register_i80_to_i8(__r) (((__r) + 1) & 7)
#define register_i8_to_i80(__r) (((__r) - 1) & 7)
#define condition_i80_to_i8(__c) ((((__c) >> 1) ^ 1) | (((__c) & 1) << 1))
#define condition_i8_to_i80(__c) (((((__c) & 3) ^ 1) << 1) | ((__c) >> 2))

enum mnemonic_t
{
	MNEM_NONE,

	MNEM_I8080_ACI,
	MNEM_I8080_ADC,
	MNEM_I8080_ADD,
	MNEM_I8080_ADI,
	MNEM_I8080_ANA,
	MNEM_I8080_ANI,
	MNEM_I8080_CALL,
	MNEM_I8080_C_CC,
	MNEM_I8080_CMA,
	MNEM_Z80_CPL = MNEM_I8080_CMA,
	MNEM_I8080_CMC,
	MNEM_Z80_CCF = MNEM_I8080_CMC,
	MNEM_I8080_CMP,
	MNEM_I8080_CPI,
	MNEM_I8080_DAA,
	MNEM_Z80_DAA = MNEM_I8080_DAA,
	MNEM_I8080_DAD,
	MNEM_I8080_DCR,
	MNEM_I8080_DCX,
	MNEM_I8080_DI,
	MNEM_Z80_DI = MNEM_I8080_DI,
	MNEM_I8080_EI,
	MNEM_Z80_EI = MNEM_I8080_EI,
	MNEM_I8080_HLT,
	MNEM_Z80_HALT = MNEM_I8080_HLT,
	MNEM_I8080_IN,
	MNEM_I8080_INR,
	MNEM_I8080_INX,
	MNEM_I8080_J_CC,
	MNEM_I8080_JMP,
	MNEM_I8080_LDA,
	MNEM_I8080_LDAX,
	MNEM_I8080_LHLD,
	MNEM_I8080_LXI,
	MNEM_I8080_MOV,
	MNEM_I8080_MVI,
	MNEM_I8080_NOP,
	MNEM_Z80_NOP = MNEM_I8080_NOP,
	MNEM_I8080_ORA,
	MNEM_I8080_ORI,
	MNEM_I8080_OUT,
	MNEM_I8080_PCHL,
	MNEM_I8080_POP,
	MNEM_I8080_PUSH,
	MNEM_I8080_RAL,
	MNEM_Z80_RLA = MNEM_I8080_RAL,
	MNEM_I8080_RAR,
	MNEM_Z80_RRA = MNEM_I8080_RAR,
	MNEM_I8080_R_CC,
	MNEM_I8080_RET,
	MNEM_I8080_RLC,
	MNEM_Z80_RLCA = MNEM_I8080_RLC,
	MNEM_I8080_RRC,
	MNEM_Z80_RRCA = MNEM_I8080_RRC,
	MNEM_I8080_RST,
	MNEM_I8080_SBB,
	MNEM_I8080_SBI,
	MNEM_I8080_SHLD,
	MNEM_I8080_SPHL,
	MNEM_I8080_STA,
	MNEM_I8080_STAX,
	MNEM_I8080_STC,
	MNEM_Z80_SCF = MNEM_I8080_STC,
	MNEM_I8080_SUB,
	MNEM_I8080_SUI,
	MNEM_I8080_XCHG,
	MNEM_I8080_XRA,
	MNEM_I8080_XRI,
	MNEM_I8080_XTHL,

	MNEM_I8085_ARHL,
	MNEM_I8085_DSUB,
	MNEM_I8085_JNUI,
	MNEM_I8085_JUI,
	MNEM_I8085_LDHI,
	MNEM_I8085_LDSI,
	MNEM_I8085_LHLX,
	MNEM_I8085_RDEL,
	MNEM_I8085_RIM,
	MNEM_I8085_RSTV,
	MNEM_I8085_SHLX,
	MNEM_I8085_SIM,

	MNEM_Z80_ADC,
	MNEM_Z80_ADD,
	MNEM_Z80_AND,
	MNEM_Z80_BIT,
	MNEM_Z80_CALL,
	MNEM_Z80_CP,
	MNEM_Z80_CPD,
	MNEM_Z80_CPDR,
	MNEM_Z80_CPI,
	MNEM_Z80_CPIR,
	MNEM_Z80_DEC,
	MNEM_Z80_DJNZ,
	MNEM_Z80_EX,
	MNEM_Z80_EXX,
	MNEM_Z80_IM,
	MNEM_Z80_IN,
	MNEM_Z80_INC,
	MNEM_Z80_IND,
	MNEM_Z80_INDR,
	MNEM_Z80_INI,
	MNEM_Z80_INIR,
	MNEM_Z80_JP,
	MNEM_Z80_JR,
	MNEM_Z80_LD,
	MNEM_Z80_LDD,
	MNEM_Z80_LDDR,
	MNEM_Z80_LDI,
	MNEM_Z80_LDIR,
	MNEM_Z80_NEG,
	MNEM_Z80_OR,
	MNEM_Z80_OTDR,
	MNEM_Z80_OTIR,
	MNEM_Z80_OUT,
	MNEM_Z80_OUTD,
	MNEM_Z80_OUTI,
	MNEM_Z80_POP,
	MNEM_Z80_PUSH,
	MNEM_Z80_RES,
	MNEM_Z80_RET,
	MNEM_Z80_RETI,
	MNEM_Z80_RETN,
	MNEM_Z80_RL,
	MNEM_Z80_RLC,
	MNEM_Z80_RLD,
	MNEM_Z80_RR,
	MNEM_Z80_RRC,
	MNEM_Z80_RRD,
	MNEM_Z80_RST,
	MNEM_Z80_SBC,
	MNEM_Z80_SET,
	MNEM_Z80_SLA,
	MNEM_Z80_SLL,
	MNEM_Z80_SRA,
	MNEM_Z80_SRL,
	MNEM_Z80_SUB,
	MNEM_Z80_XOR,

	MNEM_GBZ80_LDHL,
	MNEM_GBZ80_STOP,
	MNEM_GBZ80_SWAP,

	MNEM_R800_ADJ,
	MNEM_R800_AND,
	MNEM_R800_BR,
	MNEM_R800_SHORT_BR,
	MNEM_R800_SHORT_B_CC,
	MNEM_R800_CMP,
	MNEM_R800_CMPM,
	MNEM_R800_IN,
	MNEM_R800_INM,
	MNEM_R800_MOVE,
	MNEM_R800_MOVEM,
	MNEM_R800_MULUB,
	MNEM_R800_MULUW,
	MNEM_R800_NEG,
	MNEM_R800_NOT,
	MNEM_R800_OR,
	MNEM_R800_OUT,
	MNEM_R800_OUTM,
	MNEM_R800_ROL4,
	MNEM_R800_ROR4,
	MNEM_R800_SUB,
	MNEM_R800_XOR,

	MNEM_Z80_CALLN,
	MNEM_Z80_RETEM,

	MNEM_I8008_AC_R,
	MNEM_I8008_AD_R,
	MNEM_I8008_CP_R,
	MNEM_I8008_DC_R,
	MNEM_I8008_IN_R,
	MNEM_I8008_L_R_I,
	MNEM_I8008_L_R_R,
	MNEM_I8008_ND_R,
	MNEM_I8008_OR_R,
	MNEM_I8008_SB_R,
	MNEM_I8008_SU_R,
	MNEM_I8008_XR_R,

	MNEM_DP2200_ALPHA,
	MNEM_DP2200_BETA,
	MNEM_DP2200_EX,
	MNEM_DP2200_INPUT,
	MNEM_DP2200_POP,
	MNEM_DP2200_PUSH,

	_MNEM_TOTAL,

	MNEM_I8008_ACI = MNEM_I8080_ACI,
	MNEM_I8008_ADI = MNEM_I8080_ADI,
	MNEM_I8008_CAL = MNEM_I8080_CALL,
	MNEM_I8008_C_CC = MNEM_I8080_C_CC,
	MNEM_I8008_CPI = MNEM_I8080_CPI,
	MNEM_I8008_NDI = MNEM_I8080_ANI,
	MNEM_I8008_HLT = MNEM_I8080_HLT,
	MNEM_I8008_INP = MNEM_I8080_IN,
	MNEM_I8008_J_CC = MNEM_I8080_J_CC,
	MNEM_I8008_JMP = MNEM_I8080_JMP,
	MNEM_I8008_NOP = MNEM_I8080_NOP,
	MNEM_I8008_ORI = MNEM_I8080_ORI,
	MNEM_I8008_OUT = MNEM_I8080_OUT,
	MNEM_I8008_RAL = MNEM_I8080_RAL,
	MNEM_I8008_RAR = MNEM_I8080_RAR,
	MNEM_I8008_RLC = MNEM_I8080_RLC,
	MNEM_I8008_RRC = MNEM_I8080_RRC,
	MNEM_I8008_R_CC = MNEM_I8080_R_CC,
	MNEM_I8008_RET = MNEM_I8080_RET,
	MNEM_I8008_RST = MNEM_I8080_RST,
	MNEM_I8008_SBI = MNEM_I8080_SBI,
	MNEM_I8008_SUI = MNEM_I8080_SUI,
	MNEM_I8008_XRI = MNEM_I8080_XRI,

	MNEM_DP2200_DI = MNEM_I8080_DI,
	MNEM_DP2200_EI = MNEM_I8080_EI,

	_MNEMONIC_COMMON_ENUMERATORS
};
typedef enum mnemonic_t mnemonic_t;

#define _COND_MNEM(cond, mnem) (((mnem) & 0x03FF) | ((cond) << 10))
#define _GET_MNEM(value) ((value) & 0x03FF)
#define _GET_COND(value) ((value) >> 10)
struct operand_t
{
	operand_type_t type;
//	bitsize_t size; // TODO
//	bitsize_t address_size; // TODO
	regnumber_t base;
//	regnumber_t index; // TODO: maybe z280 and similar
	expression_t * parameter;
};

#ifndef MAX_OPD_COUNT
# define MAX_OPD_COUNT 3
#endif

typedef struct instruction_pattern_t instruction_pattern_t;

struct instruction_patterns_t
{
	const instruction_pattern_t * pattern;
	size_t count;
};
typedef struct instruction_patterns_t instruction_patterns_t;

typedef struct pattern_t pattern_t;
struct pattern_t
{
	struct instruction_patterns_t pattern[MAX_OPD_COUNT + 1];
};

struct instruction_t
{
	_INSTRUCTION_COMMON_FIELDS

	cpu_type_t cpu;
//	bitsize_t bits; // TODO: maybe z380 and ez80
	condition_t condition;
		/*
			Note: this is intended to store the condition field for instructions such as jCC
			It is also used for the 8008 instructions with the register stored in the mnemonic
			To keep things consistent, conditions are always stored according to their 8080 values
			However, registers are stored according to their 8008 values, since that is where they get used the most
		*/
//	bitsize_t operation_size; // TODO
//	bitsize_t address_size; // TODO
};

typedef struct parser_state_t
{
	instruction_stream_t stream;
	size_t line_number;

	cpu_type_t cpu_type;
	syntax_t syntax;
//	bitsize_t bit_size; // TODO: maybe z380 and ez80
} parser_state_t;

extern parser_state_t current_parser_state[1];
extern instruction_t current_instruction;

extern size_t x80_instruction_compute_length(instruction_t * ins, bool forgiving);
#ifndef instruction_compute_length
# define instruction_compute_length x80_instruction_compute_length
#endif

extern void x80_generate_instruction(instruction_t * ins);
#ifndef generate_instruction
# define generate_instruction x80_generate_instruction
#endif

#ifndef elf_machine_type
# define elf_machine_type() EM_Z80
#endif

#ifndef elf_default_byte_order
# define elf_default_byte_order() ELFDATA2LSB
#endif

#ifndef elf_backend_uses_rela
# define elf_backend_uses_rela() false
#endif

static inline int elf_get_relocation_type(relocation_t rel)
{
	switch(rel.size)
	{
	case BITSIZE8:
		if(rel.pc_relative)
			return R_Z80_8_PCREL;
		else if(rel.hint != 0)
			return rel.hint; // R_Z80_8_DIS;
		else
			return R_Z80_8; // TODO: or R_Z80_BYTE0
	case BITSIZE16:
		return R_Z80_16; // TODO: or R_Z80_WORD0
	case BITSIZE24:
		return R_Z80_24;
	case BITSIZE32:
		return R_Z80_32;
	default:
		return -1;
	}
}

#ifndef nop_byte
# define nop_byte(__ins, __index) 0x00
#endif

#ifndef coff_magic_number
# define coff_magic_number() Z80MAGIC
#endif

#ifndef coff_header_flags
# define coff_header_flags() 0x1104
#endif

#ifndef coff_relocation_size
# define coff_relocation_size() 16
#endif

static inline int coff_get_relocation_type(relocation_t rel)
{
	switch(rel.size)
	{
	case BITSIZE8:
		if(rel.pc_relative)
			return R_JR;
		else if(rel.hint == R_Z80_8_DIS)
			return R_OFF8;
		else
			return R_IMM8;
	case BITSIZE16:
		return R_IMM16;
	case BITSIZE24:
		return R_IMM24;
	case BITSIZE32:
		return R_IMM32;
	default:
		return -1;
	}
}

#endif // _X80_ISA_H
