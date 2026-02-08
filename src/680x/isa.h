#ifndef _680X_ISA_H
#define _680X_ISA_H

#include <stdbool.h>
#include <stdio.h>
#include "../asm.h"
#include "../elf.h"
#include "../../obj/680x/680x/parser.tab.h"

#ifndef TARGET_NAME
# define TARGET_NAME "680x"
#endif

#define ARCH_BITS_IN_UNIT 8
#define ARCH_BITS_IN_CHAR 8

#define IGNORE 0

enum regnumber_t
{
	REG_D = 0,
	REG_X = 1,
	REG_Y = 2,
	REG_U = 3,
	REG_S = 4,
	REG_PC = 5,
	REG_A = 8,
	REG_B = 9,
	REG_CC = 10,
	REG_DP = 11,
};
typedef enum regnumber_t regnumber_t;

enum cpu_type_t
{
	CPU_6800 = 1,
	CPU_6809,
};
typedef enum cpu_type_t cpu_type_t;

enum operand_type_t
{
	OPD_NONE,
	OPD_IMMB,
	OPD_IMMW,
	OPD_DIR,
	OPD_IND,
	OPD_EXT,
	OPD_RELB,
	OPD_RELW,
	OPD_REG2,
	OPD_REGLIST,
};
#define _OPD_TYPE_COUNT (OPD_REGLIST + 1)
typedef enum operand_type_t operand_type_t;

enum mnemonic_t
{
	MNEM_NONE,

#include "../../obj/680x/680x/ins.h"

	_MNEM_TOTAL,

	_MNEMONIC_COMMON_ENUMERATORS
};
typedef enum mnemonic_t mnemonic_t;

struct operand_t
{
	operand_type_t type;
	int index;
	expression_t * parameter;
};

#define MAKE_IDX(reg, mode) ((((reg) - REG_X) << 5) | (mode))

#ifndef MAX_OPD_COUNT
# define MAX_OPD_COUNT 1
#endif

struct instruction_t
{
	_INSTRUCTION_COMMON_FIELDS

	cpu_type_t cpu;
};

typedef struct parser_state_t
{
	instruction_stream_t stream;
	size_t line_number;

	cpu_type_t cpu_type;
} parser_state_t;

extern parser_state_t current_parser_state[1];
extern instruction_t current_instruction;

extern size_t m680x_instruction_compute_length(instruction_t * ins, bool forgiving);
#ifndef instruction_compute_length
# define instruction_compute_length m680x_instruction_compute_length
#endif

extern void m680x_generate_instruction(instruction_t * ins);
#ifndef generate_instruction
# define generate_instruction m680x_generate_instruction
#endif

#ifndef elf_machine_type
# define elf_machine_type() EM_68HC11
#endif

#ifndef elf_default_byte_order
# define elf_default_byte_order() ELFDATA2MSB
#endif

#ifndef elf_backend_uses_rela
# define elf_backend_uses_rela() true
#endif

static inline int elf_get_relocation_type(relocation_t rel)
{
	switch(rel.size)
	{
	case BITSIZE8:
		if(rel.pc_relative)
			return R_M68HC11_PCREL_8;
		else if(rel.hint != 0)
			return rel.hint; // R_M68HC11_HI8 or R_M68HC11_LO8
		else
			return R_M68HC11_8;
	case BITSIZE16:
		if(rel.pc_relative)
			return R_M68HC11_PCREL_16;
		else
			return R_M68HC11_16;
	case BITSIZE24:
		return R_M68HC11_24;
	case BITSIZE32:
		return R_M68HC11_32;
	default:
		return -1;
	}
}

#ifndef nop_byte
# define nop_byte(__ins, __index) ((__ins) != NULL && (__ins)->cpu != CPU_6809 ? 0x01 : 0x12)
#endif

#ifndef coff_magic_number
# define coff_magic_number() 0 // TODO: undefined
#endif

#ifndef coff_header_flags
# define coff_header_flags() 0 // TODO: undefined
#endif

#ifndef coff_relocation_size
# define coff_relocation_size() 0 // TODO: undefined
#endif

#ifndef coff_get_relocation_type
# define coff_get_relocation_type(rel) 0 // TODO: undefined
#endif

#endif // _680X_ISA_H
