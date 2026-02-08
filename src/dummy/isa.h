#ifndef _DUMMY_ISA_H
#define _DUMMY_ISA_H

#include <stdbool.h>
#include <stdio.h>
#include "../asm.h"
#include "../elf.h"
#include "../../obj/dummy/dummy/parser.tab.h"

#ifndef TARGET_NAME
# define TARGET_NAME "dummy"
#endif

#define ARCH_BITS_IN_UNIT 8
#define ARCH_BITS_IN_CHAR 8

enum mnemonic_t
{
	MNEM_NONE,
	_MNEM_TOTAL,
	_MNEMONIC_COMMON_ENUMERATORS
};
typedef enum mnemonic_t mnemonic_t;

struct operand_t
{
	expression_t * parameter;
};

#ifndef MAX_OPD_COUNT
# define MAX_OPD_COUNT 0
#endif

struct instruction_t
{
	_INSTRUCTION_COMMON_FIELDS
};

typedef struct parser_state_t
{
	instruction_stream_t stream;
	size_t line_number;
} parser_state_t;

extern parser_state_t current_parser_state[1];
extern instruction_t current_instruction;

extern size_t dummy_instruction_compute_length(instruction_t * ins, bool forgiving);
#ifndef instruction_compute_length
# define instruction_compute_length dummy_instruction_compute_length
#endif

extern void dummy_generate_instruction(instruction_t * ins);
#ifndef generate_instruction
# define generate_instruction dummy_generate_instruction
#endif

#ifndef elf_machine_type
# define elf_machine_type() EM_NONE
#endif

#ifndef elf_default_byte_order
# define elf_default_byte_order() ELFDATA2LSB
#endif

#ifndef elf_backend_uses_rela
# define elf_backend_uses_rela() false
#endif

static inline int elf_get_relocation_type(relocation_t rel)
{
	return 0;
}

#ifndef nop_byte
# define nop_byte(__ins, __index) 0
#endif

#ifndef coff_magic_number
# define coff_magic_number() 0
#endif

#ifndef coff_header_flags
# define coff_header_flags() 0
#endif

#ifndef coff_relocation_size
# define coff_relocation_size() 0
#endif

#ifndef coff_get_relocation_type
# define coff_get_relocation_type(rel) 0
#endif

#endif // _DUMMY_ISA_H
