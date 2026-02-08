#ifndef _X65_ISA_H
#define _X65_ISA_H

#include "../asm.h"
#include "../elf.h"
#include "../coff.h"

#define TARGET_NAME "x65"

#define ARCH_BITS_IN_UNIT 8
#define ARCH_BITS_IN_CHAR 8

enum cpu_type_t
{
	CPU_6502 = 1,
	CPU_65C02,
	CPU_WDC65C02, // also Rockwell
	CPU_65CE02,
	CPU_65C816,
	CPU_LAST = CPU_65C816,
};
typedef enum cpu_type_t cpu_type_t;

enum operand_type_t
{
	OPD_NONE,
	OPD_IMM,
	OPD_REG_A,
	OPD_MEM,
	OPD_MEM_X,
	OPD_MEM_Y,
	OPD_IND, // JMP (i16) only
	OPD_IND_X, // (u8,X)
	OPD_IND_Y, // (u8),Y
	OPD_IND_Z, // (u8),Z
	OPD_LNG, // [i8] - 65c816
	OPD_LNG_Y, // [i8],Y = 65c816
	OPD_STK, // u8,S - 65c816
	OPD_STK_Y, // (u8,S),Y - 65c816
	OPD_PAIR, // u8,u8
};
typedef enum operand_type_t operand_type_t;

enum mnemonic_t
{
	MNEM_NONE,

	MNEM_ADC,
	MNEM_AND,
	MNEM_ASL,
	MNEM_ASR, // 65ce02
	MNEM_ASW, // 65ce02
	MNEM_AUG, // 65ce02
	MNEM_BBR, // wdc65c02
	MNEM_BBS, // wdc65c02
	MNEM_BCC,
	MNEM_BCS,
	MNEM_BEQ,
	MNEM_BIT,
	MNEM_BMI,
	MNEM_BNE,
	MNEM_BPL,
	MNEM_BRA,
	MNEM_BRK,
	MNEM_BRL,
	MNEM_BSR, // 65ce02
	MNEM_BVC,
	MNEM_BVS,
	MNEM_CLC,
	MNEM_CLD,
	MNEM_CLE, // 65ce02
	MNEM_CLI,
	MNEM_CLV,
	MNEM_CMP,
	MNEM_COP,
	MNEM_CPX,
	MNEM_CPY,
	MNEM_CPZ, // 65ce02
	MNEM_DEC,
	MNEM_DEW, // 65ce02
	MNEM_DEX,
	MNEM_DEY,
	MNEM_DEZ, // 65ce02
	MNEM_EOR,
	MNEM_INC,
	MNEM_INW, // 65ce02
	MNEM_INX,
	MNEM_INY,
	MNEM_INZ, // 65ce02
	MNEM_JML,
	MNEM_JMP,
	MNEM_JSL,
	MNEM_JSR,
	MNEM_LDA,
	MNEM_LDX,
	MNEM_LDY,
	MNEM_LDZ, // 65ce02
	MNEM_LSR,
	MNEM_MVN,
	MNEM_MVP,
	MNEM_NEG, // 65ce02
	MNEM_NOP,
	MNEM_ORA,
	MNEM_PEA,
	MNEM_PEI,
	MNEM_PER,
	MNEM_PHA,
	MNEM_PHB,
	MNEM_PHD,
	MNEM_PHK,
	MNEM_PHP,
	MNEM_PHW, // 65ce02
	MNEM_PHX,
	MNEM_PHY,
	MNEM_PHZ, // 65ce02
	MNEM_PLA,
	MNEM_PLB,
	MNEM_PLD,
	MNEM_PLP,
	MNEM_PLX,
	MNEM_PLY,
	MNEM_PLZ, // 65ce02
	MNEM_REP,
	MNEM_RMB, // 65ce02
	MNEM_ROL,
	MNEM_ROR,
	MNEM_ROW, // 65ce02
	MNEM_RTI,
	MNEM_RTL,
	MNEM_RTS,
	MNEM_SBC,
	MNEM_SEC,
	MNEM_SED,
	MNEM_SEE, // 65ce02
	MNEM_SEI,
	MNEM_SEP,
	MNEM_SMB, // 65ce02
	MNEM_STA,
	MNEM_STP,
	MNEM_STX,
	MNEM_STY,
	MNEM_STZ,
	MNEM_TAB, // 65ce02
	MNEM_TAX,
	MNEM_TAY,
	MNEM_TAZ, // 65ce02
	MNEM_TBA, // 65ce02
	MNEM_TCD,
	MNEM_TCS,
	MNEM_TDC,
	MNEM_TRB,
	MNEM_TSB,
	MNEM_TSC,
	MNEM_TSX,
	MNEM_TSY, // 65ce02
	MNEM_TXA,
	MNEM_TXS,
	MNEM_TXY,
	MNEM_TYA,
	MNEM_TYS, // 65ce02
	MNEM_TYX,
	MNEM_TZA, // 65ce02
	MNEM_WAI,
	MNEM_WDM,
	MNEM_XBA,
	MNEM_XCE,

	_MNEM_TOTAL,

	_MNEMONIC_COMMON_ENUMERATORS
};
typedef enum mnemonic_t mnemonic_t;

typedef enum operand_mode_t
{
	MODE_DEFAULT,
	MODE_LOW_BYTE, // <exp
	MODE_HIGH_BYTE, // >exp
	MODE_WORD, // !exp
	MODE_LONG, // @exp

	MODE_BANK,
} operand_mode_t;

struct operand_t
{
	operand_type_t type;
	operand_mode_t mode;
	union
	{
		expression_t * parameter;
		expression_t * parameters[2];
	};
};

#ifndef MAX_OPD_COUNT
# define MAX_OPD_COUNT 1
#endif

struct instruction_t
{
	_INSTRUCTION_COMMON_FIELDS

	cpu_type_t cpu;
	bitsize_t
		abits, // a or m
		xbits; // x or i
	int bit; // for BBR/BBS/RMB/SMB
//	condition_t condition;
};

typedef struct parser_state_t
{
	instruction_stream_t stream;
	size_t line_number;

	cpu_type_t cpu_type;
	bitsize_t abits, xbits;
} parser_state_t;

extern parser_state_t current_parser_state[1];
extern instruction_t current_instruction;

extern size_t x65_instruction_compute_length(instruction_t * ins, bool forgiving);
#ifndef instruction_compute_length
# define instruction_compute_length x65_instruction_compute_length
#endif

extern void x65_generate_instruction(instruction_t * ins);
#ifndef generate_instruction
# define generate_instruction x65_generate_instruction
#endif

#ifndef elf_machine_type
# define elf_machine_type() EM_MOS
#endif

#ifndef elf_default_byte_order
# define elf_default_byte_order() ELFDATA2LSB
#endif

#ifndef elf_backend_uses_rela
# define elf_backend_uses_rela() false
#endif

static inline int elf_get_relocation_type(relocation_t rel)
{
	if(rel.hint != 0)
		return rel.hint;

	switch(rel.size)
	{
	case BITSIZE8:
		if(rel.pc_relative)
			return R_MOS_PCREL_8;
		else
			return R_MOS_IMM8;
	case BITSIZE16:
		if(rel.pc_relative)
			return R_MOS_PCREL_16;
		else
			return R_MOS_IMM16;
	case BITSIZE24:
		return R_MOS_ADDR24;
	default:
		return -1;
	}
}

#ifndef nop_byte
# define nop_byte(__ins, __index) 0xEA
#endif

extern int x65_expression_get_hint(operand_t * opd, reference_t * ref, int fmt, size_t size, bool pcrel);
#ifndef expression_get_hint
# define expression_get_hint x65_expression_get_hint
#endif

#ifndef coff_magic_number
# define coff_magic_number() W65MAGIC
#endif

#ifndef coff_header_flags
# define coff_header_flags() 0x0004
#endif

#ifndef coff_relocation_size
# define coff_relocation_size() 16
#endif

static inline int coff_get_relocation_type(relocation_t rel)
{
	switch(rel.hint)
	{
	case R_MOS_ADDR16_HI:
		return R_W65_ABS8S8;
	case R_MOS_ADDR24_BANK:
		return R_W65_ABS8S16;
	}

	switch(rel.size)
	{
	case BITSIZE8:
		if(rel.pc_relative)
			return R_W65_PCR8;
		else
			return R_W65_ABS8;
	case BITSIZE16:
		if(rel.pc_relative)
			return R_W65_PCR16;
		else
			return R_W65_ABS16;
	case BITSIZE24:
		return R_W65_ABS24;
	default:
		return -1;
	}
}

#endif // _X65_ISA_H
