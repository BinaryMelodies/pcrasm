#ifndef _I4_ISA_H
#define _I4_ISA_H

#include <stdio.h>
#include "../asm.h"
#include "../elf.h"
#include "../coff.h"
#include "../../obj/i4/i4/parser.tab.h"

#ifndef TARGET_NAME
# define TARGET_NAME "i4"
#endif

#define ARCH_BITS_IN_UNIT 8 // TODO: 4?
#define ARCH_BITS_IN_CHAR 8

enum cpu_type_t
{
	CPU_4004 = 1,
	CPU_4040,
};
typedef enum cpu_type_t cpu_type_t;

enum mnemonic_t
{
	MNEM_NONE,

	MNEM_HLT = 0x01,
	MNEM_BBS = 0x02,
	MNEM_LCR = 0x03,
	MNEM_OR4 = 0x04,
	MNEM_OR5 = 0x05,
	MNEM_AN6 = 0x06,
	MNEM_AN7 = 0x07,
	MNEM_DB0 = 0x08,
	MNEM_DB1 = 0x09,
	MNEM_SB0 = 0x0A,
	MNEM_SB1 = 0x0B,
	MNEM_EIN = 0x0C,
	MNEM_DIN = 0x0D,
	MNEM_RPM = 0x0E,
	MNEM_JCN = 0x10,
	MNEM_FIM = 0x20,
	MNEM_SRC = 0x21,
	MNEM_FIN = 0x30,
	MNEM_JIN = 0x31,
	MNEM_JUN = 0x40,
	MNEM_JMS = 0x50,
	MNEM_INC = 0x60,
	MNEM_ISZ = 0x70,
	MNEM_ADD = 0x80,
	MNEM_SUB = 0x90,
	MNEM_LD  = 0xA0,
	MNEM_XCH = 0xB0,
	MNEM_BBL = 0xC0,
	MNEM_LDM = 0xD0,
	MNEM_WRM = 0xE0,
	MNEM_WMP = 0xE1,
	MNEM_WRR = 0xE2,
	MNEM_WPM = 0xE3,
	MNEM_WR0 = 0xE4,
	MNEM_WR1 = 0xE5,
	MNEM_WR2 = 0xE6,
	MNEM_WR3 = 0xE7,
	MNEM_SBM = 0xE8,
	MNEM_RDM = 0xE9,
	MNEM_RDR = 0xEA,
	MNEM_ADM = 0xEB,
	MNEM_RD0 = 0xEC,
	MNEM_RD1 = 0xED,
	MNEM_RD2 = 0xEE,
	MNEM_RD3 = 0xEF,
	MNEM_CLB = 0xF0,
	MNEM_CLC = 0xF1,
	MNEM_IAC = 0xF2,
	MNEM_CMC = 0xF3,
	MNEM_CMA = 0xF4,
	MNEM_RAL = 0xF5,
	MNEM_RAR = 0xF6,
	MNEM_TCC = 0xF7,
	MNEM_DAC = 0xF8,
	MNEM_TCS = 0xF9,
	MNEM_STC = 0xFA,
	MNEM_DAA = 0xFB,
	MNEM_KBP = 0xFC,
	MNEM_DCL = 0xFD,

	MNEM_NOP = 0x100,
	_MNEM_TOTAL,

	_MNEMONIC_COMMON_ENUMERATORS
};
typedef enum mnemonic_t mnemonic_t;

#ifndef MAX_OPD_COUNT
# define MAX_OPD_COUNT 2
#endif

struct operand_t
{
	expression_t * parameter;
};

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

extern size_t i4_instruction_compute_length(instruction_t * ins, bool forgiving);
#ifndef instruction_compute_length
# define instruction_compute_length i4_instruction_compute_length
#endif

extern void i4_generate_instruction(instruction_t * ins);
#ifndef generate_instruction
# define generate_instruction i4_generate_instruction
#endif

#ifndef elf_machine_type
# define elf_machine_type() 0 // TODO: undefined
#endif

#ifndef elf_default_byte_order
# define elf_default_byte_order() ELFDATA2MSB
#endif

#ifndef elf_backend_uses_rela
# define elf_backend_uses_rela() false
#endif

#ifndef elf_get_relocation_type
# define elf_get_relocation_type(__rel) 0 // TODO: undefined
#endif

#ifndef nop_byte
# define nop_byte(__ins, __index) 0x00
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
# define coff_get_relocation_type(__rel) 0 // TODO: undefined
#endif

#endif // _I4_ISA_H
