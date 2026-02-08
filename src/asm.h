#ifndef _ASM_H
#define _ASM_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "integer.h"
#include "symbolic.h"

static inline long align_to(long value, long align)
{
	if((align & (align - 1)) == 0)
	{
		return (value + align - 1) & ~(align - 1);
	}
	else
	{
		value += align - 1;
		return value - value % align;
	}
}

#if USE_GMP
# define NO_ORDINAL NULL
#else
# define NO_ORDINAL -1
#endif

enum bitsize_t
{
	BITSIZE_ERROR = -1,
	BITSIZE_NONE = 0,
	BITSIZE8 = 8,
	BITSIZE16 = 16,
	BITSIZE24 = 24,
	BITSIZE32 = 32,
	BITSIZE64 = 64,
	BITSIZE80 = 80,
	BITSIZE128 = 128,
	BITSIZE256 = 256,
	BITSIZE512 = 512,
};
typedef enum bitsize_t bitsize_t;

#define BITSIN(__bitsize) ((size_t)(__bitsize))
#define OCTETSIN(__bitsize) (((size_t)(__bitsize) + 7) >> 3)

#define _DATA_MODE_SHIFT 10
#define _DATA_SIZE_MASK ((1 << _DATA_MODE_SHIFT) - 1)
enum
{
	DATA_LAYOUT_MASK = 3 << _DATA_MODE_SHIFT,
	DATA_LE = 1 << _DATA_MODE_SHIFT,
	DATA_BE = 2 << _DATA_MODE_SHIFT,
	DATA_PE = 3 << _DATA_MODE_SHIFT,
	_DATA_LAST = 5 << _DATA_MODE_SHIFT,
};

#define _DATA_LE(bytes) (DATA_LE | ((bytes) & _DATA_SIZE_MASK))
#define _DATA_BE(bytes) (DATA_BE | ((bytes) & _DATA_SIZE_MASK))
#define _DATA_PE(bytes) (DATA_PE | ((bytes) & _DATA_SIZE_MASK))

#define _GET_DATA_SIZE(data) ((data) & _DATA_SIZE_MASK)

enum
{
	_PSEUDO_MNEM_EQU,
	_PSEUDO_MNEM_LOCAL_LABEL,
	_PSEUDO_MNEM_EXTERNAL,
	_PSEUDO_MNEM_GLOBAL,
	_PSEUDO_MNEM_COMMON,
	_PSEUDO_MNEM_SECTION,
	_PSEUDO_MNEM_ORG,
	_PSEUDO_MNEM_SKIP,
	_PSEUDO_MNEM_FILL,
	_PSEUDO_MNEM_END_FILL,
	_PSEUDO_MNEM_TIMES,
	_PSEUDO_MNEM_END_TIMES,
	_PSEUDO_MNEM_IMPORT,
	_PSEUDO_MNEM_EXPORT,
};

#define _MAKE_PSEUDO_MNEMONIC(__name) __name = PSEUDO_MNEM_DATA(_DATA_LAST) - _##__name

#define _MNEMONIC_COMMON_ENUMERATORS \
	_MAKE_PSEUDO_MNEMONIC(PSEUDO_MNEM_EQU), \
	_MAKE_PSEUDO_MNEMONIC(PSEUDO_MNEM_LOCAL_LABEL), \
	_MAKE_PSEUDO_MNEMONIC(PSEUDO_MNEM_EXTERNAL), \
	_MAKE_PSEUDO_MNEMONIC(PSEUDO_MNEM_GLOBAL), \
	_MAKE_PSEUDO_MNEMONIC(PSEUDO_MNEM_COMMON), \
	_MAKE_PSEUDO_MNEMONIC(PSEUDO_MNEM_SECTION), \
	_MAKE_PSEUDO_MNEMONIC(PSEUDO_MNEM_ORG), \
	_MAKE_PSEUDO_MNEMONIC(PSEUDO_MNEM_SKIP), \
	_MAKE_PSEUDO_MNEMONIC(PSEUDO_MNEM_FILL), \
	_MAKE_PSEUDO_MNEMONIC(PSEUDO_MNEM_END_FILL), \
	_MAKE_PSEUDO_MNEMONIC(PSEUDO_MNEM_TIMES), \
	_MAKE_PSEUDO_MNEMONIC(PSEUDO_MNEM_END_TIMES), \
	_MAKE_PSEUDO_MNEMONIC(PSEUDO_MNEM_IMPORT), \
	_MAKE_PSEUDO_MNEMONIC(PSEUDO_MNEM_EXPORT), \

#define PSEUDO_MNEM_DATA(__format) (-(__format))
#define IS_PSEUDO_MNEM_DATA(__mnemonic) ((__mnemonic) < 0 && (__mnemonic) > PSEUDO_MNEM_DATA(_DATA_LAST))
#define DATA_FORMAT(__mnemonic) (-(__mnemonic))

typedef struct instruction_t instruction_t;
#define _INSTRUCTION_COMMON_FIELDS \
	size_t code_offset; \
	size_t code_size; \
	instruction_t * next; /* in sequence */ \
	instruction_t * following; /* in section */ \
	size_t line_number; \
	instruction_t * termination; /* for TIMES */ \
	union \
	{ \
		struct \
		{ \
			size_t count, current; \
		} repetition; \
		struct \
		{ \
			size_t count, old_limit; \
		} fill; \
	}; \
	size_t containing_section; \
	size_t operand_count; \
	operand_t operand[MAX_OPD_COUNT < 4 ? 4 : MAX_OPD_COUNT]; /* needs at least 4 operands */ \
	mnemonic_t mnemonic;

struct instruction_stream_t
{
	instruction_t * first_instruction;
	instruction_t ** last_instruction;
};
typedef struct instruction_stream_t instruction_stream_t;

typedef struct parser_state_t parser_state_t;

static inline size_t integer_get_size(integer_t source)
{
	if(int_sgn(source) == 0)
		return 0;
	else if(int_sgn(source) < 0)
	{
		if(int_cmp_si(source, -0x80) >= 0)
			return 1;
		else if(int_cmp_si(source, -0x8000L) >= 0)
			return 2;
		else if(int_cmp_si(source, -0x80000000LL) >= 0)
			return 4;
		else
			return 8;
	}
	else
	{
		if(int_cmp_ui(source, 0x7F) <= 0)
			return 1;
		else if(int_cmp_ui(source, 0x7FFF) <= 0)
			return 2;
		else if(int_cmp_ui(source, 0x7FFFFFFFL) <= 0)
			return 4;
		else
			return 8;
	}
}

static inline size_t uinteger_get_size(uinteger_t source)
{
	if(uint_sgn(source) == 0)
		return 0;
	else if(uint_sgn(source) < 0)
		return 8;
	else if(uint_cmp_ui(source, 0xFF) <= 0)
		return 1;
	else if(uint_cmp_ui(source, 0xFFFFL) <= 0)
		return 2;
	else if(uint_cmp_ui(source, 0xFFFFFFFFLL) <= 0)
		return 4;
	else
		return 8;
}

typedef struct operand_t operand_t;

void setup_lexer(parser_state_t * state);

instruction_t * instruction_clone(instruction_t * ins);
void instruction_clear(instruction_t * ins, parser_state_t * state);

typedef enum match_type_t
{
	MATCH_FAILED,
	MATCH_TRUNCATED,
	MATCH_PERFECT,
} match_type_t;

typedef struct match_result_t
{
	match_type_t type;
	int info; // optional
} match_result_t;

enum section_format_t
{
	SECTION_DATA,
	SECTION_ZERO_DATA,
	SECTION_STRTAB,
	SECTION_SYMTAB,
	SECTION_RELOC,
};
//#define SECTION_DATA_WITH_RELOC (SECTION_RELOC + 1) // not a real type
//#define SECTION_ZERO_DATA_WITH_RELOC (SECTION_RELOC + 2) // not a real type
#define SECTION_DATA_WITH_RELOC SECTION_DATA
#define SECTION_ZERO_DATA_WITH_RELOC SECTION_ZERO_DATA
#define SECTION_FAIL (-1) // not a real type
typedef enum section_format_t section_format_t;

struct relocation_t
{
	uint64_t offset;
	size_t size;
	bool pc_relative;
	variable_t var;
	integer_t addend; // for RELA
	size_t wrt_section;

	size_t hint; // hint to specialize the relocation type
	size_t retrolinker_symbol_index; // for RetroLinker segmentation
};
typedef struct relocation_t relocation_t;

typedef struct block_t block_t;
struct block_t
{
	uint64_t address;

	size_t size;
	size_t buffer_size;
	uint8_t * buffer;

	block_t * next;
};

typedef struct section_t section_t;
struct section_t
{
	const char * name;
	section_format_t format;
	uint64_t flags;
	uint64_t align;
	union
	{
		struct
		{
			size_t full_size;
			size_t current_address;
			block_t * first_block;
			block_t * current_block;
			instruction_t * first_instruction;
			instruction_t ** last_instruction;
			section_t * relocations;
		} data;
		struct
		{
			size_t count;
			size_t buffer_size;
			relocation_t * relocations;
			size_t extra_count; // for segelf
		} reloc;
		struct
		{
			size_t count;
			size_t buffer_size;
			definition_t ** symbols;
		} symtab;
		struct
		{
			size_t size;
			size_t buffer_size;
			char * buffer;
			// TODO: look up strings faster
		} strtab;
	};

	uint64_t file_offset;
	size_t elf_section_index;
	size_t elf_symbol_index;
	size_t elf_string_offset; // for the header section name
	size_t segment_section; // for segelf relocations
};

static inline section_t section_attributes(uint64_t flags, uint64_t align)
{
	return (section_t) {
		.flags = flags,
		.align = align,
	};
}

void section_append(section_t * section, size_t count, void * data);
size_t section_add_string(section_t * section, const char * string);
size_t section_add_symbol(section_t * section, definition_t * definition);
void section_add_relocation(section_t * section, uint64_t offset, size_t size, bool pc_relative, variable_t * var, integer_t addend, size_t wrt_section, size_t hint);

enum output_format_t
{
	FORMAT_DEBUG = -1,
	FORMAT_BINARY,
	FORMAT_HEX16, // x80, 8089, 8086
	FORMAT_HEX32, // 386
	FORMAT_REL, // x80
	FORMAT_OMF80, // x80
	FORMAT_OMF86, // 8086, 386
	FORMAT_COFF, // 386, ...
	FORMAT_WIN32, // 386, ...
	FORMAT_WIN64, // x86-64, ...
	FORMAT_ELF32, // 8086, 386, ...
	FORMAT_ELF64, // x86-64, ...
};
typedef enum output_format_t output_format_t;

typedef struct objfile_t objfile_t;
struct objfile_t
{
	output_format_t format;
	FILE * file;
	size_t section_count;
	section_t ** section; // array of actual sections
	uint64_t entry;
};

void section_init(section_t * section, char * name, section_format_t format, section_t _default);
size_t objfile_new_section(objfile_t * file, const char * name, section_format_t format, section_t _default);
size_t objfile_locate_section(objfile_t * file, const char * name, section_format_t format, section_t _default);

extern objfile_t output;
extern size_t output_limit;

typedef enum compilation_result_t
{
	RESULT_FAILED,
	RESULT_CHANGED,
	RESULT_COMPLETE,
} compilation_result_t;

void output_set_location(uint64_t address);
void output_byte(uint8_t value);
void output_skip(instruction_t * ins, uint64_t count);
void output_word_type(reference_t * ref, int fmt, bitsize_t size, bool pc_relative, size_t hint);

static inline void output_word16be(uint16_t value)
{
	output_byte(value >> 8);
	output_byte(value);
}

/*static inline void output_word(reference_t * ref, int fmt, size_t size)
{
	output_word_type(ref, fmt, size * ARCH_BITS_IN_UNIT, false, 0);
}

static inline void output_word_pcrel(reference_t * ref, int fmt, size_t size)
{
	output_word_type(ref, fmt, size * ARCH_BITS_IN_UNIT, true, 0);
}*/

#define output_word_as(ref, fmt, size, pcrel, hint) output_word_type(ref, fmt, (size) * ARCH_BITS_IN_UNIT, pcrel, hint)
#define output_word(ref, fmt, size)                 output_word_type(ref, fmt, (size) * ARCH_BITS_IN_UNIT, false, 0)
#define output_word_pcrel(ref, fmt, size)           output_word_type(ref, fmt, (size) * ARCH_BITS_IN_UNIT, true, 0)

static inline void fwrite8(FILE * file, uint8_t value)
{
	fwrite(&value, 1, 1, file);
}

static inline void fwrite16le(FILE * file, uint16_t value)
{
	value = htole16(value);
	fwrite(&value, 2, 1, file);
}

static inline void fwrite16be(FILE * file, uint16_t value)
{
	value = htobe16(value);
	fwrite(&value, 2, 1, file);
}

static inline void fwrite32le(FILE * file, uint32_t value)
{
	value = htole32(value);
	fwrite(&value, 4, 1, file);
}

static inline void fwrite32be(FILE * file, uint32_t value)
{
	value = htobe32(value);
	fwrite(&value, 4, 1, file);
}

static inline void fwrite64le(FILE * file, uint64_t value)
{
	value = htole64(value);
	fwrite(&value, 8, 1, file);
}

static inline void fwrite64be(FILE * file, uint64_t value)
{
	value = htobe64(value);
	fwrite(&value, 8, 1, file);
}

extern const char * entry_point_name;

extern bool is_preprocessing_stage;

#endif // _ASM_H
