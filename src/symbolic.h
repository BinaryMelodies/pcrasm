#ifndef _SYMBOLIC_C
#define _SYMBOLIC_C

#include "integer.h"

typedef struct instruction_t instruction_t;

enum expression_type_t
{
	EXP_IDENTIFIER,
	EXP_SECTION,
	EXP_INTEGER,
	EXP_STRING,
	EXP_DEFINED,
	EXP_HERE,
	EXP_SECTION_HERE,
	EXP_LABEL,
	EXP_LABEL_FORWARD, // this gets replaced by EXP_LABEL during processing
	EXP_LABEL_BACKWARD, // this gets replaced by EXP_LABEL during processing
	EXP_PLUS,
	EXP_MINUS,
	EXP_BITNOT,
	EXP_NOT,
	EXP_SEG,
	EXP_WRT,
	EXP_ALIGN,
	EXP_MUL,
	EXP_DIV,
	EXP_DIVU,
	EXP_MOD,
	EXP_MODU,
	EXP_SHL,
	EXP_SHLU,
	EXP_SHR,
	EXP_SHRU,
	EXP_ADD,
	EXP_SUB,
	EXP_BITAND,
	EXP_BITOR,
	EXP_BITXOR,
	EXP_BITORNOT,
	EXP_EQ,
	EXP_NE,
	EXP_LT,
	EXP_LTU,
	EXP_GT,
	EXP_GTU,
	EXP_LE,
	EXP_LEU,
	EXP_GE,
	EXP_GEU,
	EXP_CMP,
	EXP_CMPU,
	EXP_AND,
	EXP_XOR,
	EXP_OR,
	EXP_COND,

	EXP_LAST = EXP_COND,
};
// not appearing in expression statements
#define EXP_LIST (EXP_LAST + 1)
#define EXP_ASSIGN (EXP_LAST + 2)
typedef enum expression_type_t expression_type_t;

typedef struct expression_t expression_t;
struct expression_t
{
	expression_type_t type;
	union
	{
		char * s;
		integer_t i;
		instruction_t * l;
	} value;
	size_t argument_count;
	expression_t * argument[3];
};

enum variable_type_t
{
	VAR_NONE,
	VAR_SECTION,
	VAR_DEFINE,
};
typedef enum variable_type_t variable_type_t;

typedef struct definition_t definition_t;

typedef struct variable_t variable_t;
struct variable_t
{
	variable_type_t type;
	bool segment_of;
	union
	{
		definition_t * external;
		struct
		{
			size_t section_index;
		} internal;
	};
};

typedef struct reference_t reference_t;
struct reference_t
{
	integer_t value;
	variable_t var;
	size_t wrt_section; // needed for x86
#define WRT_DEFAULT ((size_t)-1)
#define WRT_NONE ((size_t)-2) // 8089 only
};

static inline bool is_scalar(reference_t * ref)
{
	return ref->var.type == VAR_NONE;
}

extern size_t current_section;

static inline bool is_self_relative(reference_t * ref)
{
	return ref->var.type == VAR_SECTION && ref->var.internal.section_index == current_section;
}

static inline void assert_scalar(reference_t * ref)
{
	if(ref->var.type != VAR_NONE)
	{
		fprintf(stderr, "Invalid arithmetic of non-scalar type\n");
assert(false);
	}
	ref->var.type = VAR_NONE;
	ref->wrt_section = WRT_DEFAULT;
}

extern void reference_clear(reference_t * ref);
extern void reference_set(reference_t * ref, integer_t value);
extern void reference_set_ui(reference_t * ref, uint64_t value);

typedef enum definition_type_t
{
	DEFTYPE_EQU,
	DEFTYPE_EXTERNAL,
	DEFTYPE_COMMON,
} definition_type_t;

typedef enum entry_mode_t
{
	ENTRY_NONE,
	ENTRY_BYNAME,
	ENTRY_BYORDINAL,
} entry_mode_t;

struct definition_t
{
	const char * name;
	bool global;
	definition_type_t deftype;
	reference_t ref; // align for COMMON
	definition_t * next;

	bool isfar;
	reference_t size;
	reference_t count; // when multiple elements are present

	struct
	{
		entry_mode_t mode;
		char * module;
		union
		{
			char * name;
			uint32_t ordinal;
		};
	} imported;

	struct
	{
		entry_mode_t mode;
		char * name;
		uint32_t ordinal; // optional
		uint8_t attributes;
		uint8_t parameter_count;
	} exported;

	size_t elf_symbol_index;
	size_t elf_segelf_symbol_index; // for segelf
	size_t elf_string_offset; // for the symbol name
};
extern definition_t * globals;

definition_t * definition_create(const char * name);

void evaluate_expression(expression_t * exp, reference_t * result, long here);

bool label_lookup(const char * name, reference_t * result);

void label_define(const char * name, reference_t * ref);
definition_t * label_define_external(const char * name);
definition_t * label_define_common(const char * name, integer_t size);
void local_label_define(instruction_t * ins);
definition_t * label_set_global(const char * name);
void local_label_bind_all(expression_t * exp);

#endif /* _SYMBOLIC_C */
