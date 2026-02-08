
#include "asm.h"
#include "symbolic.h"
#include "isa.h"

#if USE_GMP
static inline void integer_align_to(integer_result_t result, integer_t value, integer_t align)
{
	integer_t mask, tmp;
	mpz_init_set(mask, align);
	mpz_sub_ui(mask, mask, 1);
	mpz_init_set(tmp, mask);
	mpz_and(tmp, tmp, align);

	mpz_init_set(result, value);
	mpz_add(result, result, mask);

	if(mpz_sgn(tmp) == 0)
	{
		mpz_com(mask, mask);
		mpz_and(result, result, mask);
	}
	else
	{
		mpz_fdiv_q(tmp, value, align);
		mpz_sub(result, result, tmp);
	}
	mpz_clear(tmp);
}
#endif

void reference_clear(reference_t * ref)
{
	int_init(ref->value);
	ref->var.type = VAR_NONE;
	ref->wrt_section = WRT_DEFAULT;
}

void reference_set(reference_t * ref, integer_t value)
{
	int_init_set(ref->value, value);
	ref->var.type = VAR_NONE;
	ref->wrt_section = WRT_DEFAULT;
}

void reference_set_ui(reference_t * ref, uint64_t value)
{
	int_init_set_ui(ref->value, value);
	ref->var.type = VAR_NONE;
	ref->wrt_section = WRT_DEFAULT;
}

definition_t * globals = NULL;

definition_t * definition_create(const char * name)
{
	definition_t * definition = malloc(sizeof(definition_t));
	memset(definition, 0, sizeof(definition_t));
	definition->name = name;
	reference_clear(&definition->ref);
	reference_clear(&definition->size);
	reference_clear(&definition->count);
	return definition;
}

definition_t * definition_create_with(const char * name, integer_t value)
{
	definition_t * definition = malloc(sizeof(definition_t));
	memset(definition, 0, sizeof(definition_t));
	definition->name = name;
	int_init_set(definition->ref.value, value);
	reference_clear(&definition->size);
	reference_clear(&definition->count);
	return definition;
}

static inline definition_t ** label_locate(const char * name)
{
	definition_t ** current;
	for(current = &globals; *current != NULL; current = &(*current)->next)
	{
		if(strcmp((*current)->name, name) == 0)
			break;
	}
	return current;
}

void label_define(const char * name, reference_t * ref)
{
	definition_t ** current = label_locate(name);
	if(*current == NULL)
	{
		*current = definition_create_with(name, ref->value);
	}
	else
	{
		int_set((*current)->ref.value, ref->value);
	}
	(*current)->ref.var = ref->var;
	(*current)->ref.wrt_section = ref->wrt_section;
}

definition_t * label_set_global(const char * name)
{
	definition_t ** current = label_locate(name);
	if(*current == NULL)
	{
		*current = definition_create(name);
	}
	(*current)->global = true;
	return *current;
}

definition_t * label_define_external(const char * name)
{
	definition_t ** current = label_locate(name);
	if(*current == NULL)
	{
		*current = definition_create(name);
	}
	(*current)->deftype = DEFTYPE_EXTERNAL;
	return *current;
}

definition_t * label_define_common(const char * name, integer_t size)
{
	definition_t ** current = label_locate(name);
	if(*current == NULL)
	{
		*current = definition_create(name);
	}
	int_set((*current)->size.value, size);
	(*current)->deftype = DEFTYPE_COMMON;
	return *current;
}

bool label_lookup(const char * name, reference_t * result)
{
	definition_t * current;
	for(current = globals; current != NULL; current = current->next)
	{
		if(strcmp(current->name, name) == 0)
		{
			switch(current->deftype)
			{
			case DEFTYPE_EQU:
				reference_set(result, current->ref.value);
				result->var = current->ref.var;
				result->wrt_section = current->ref.wrt_section;
				break;
			case DEFTYPE_EXTERNAL:
				reference_clear(result);
				result->var.type = VAR_DEFINE;
				result->var.segment_of = false;
				result->var.external = current;
				result->wrt_section = WRT_DEFAULT;
				break;
			case DEFTYPE_COMMON:
				reference_clear(result);
				result->var.type = VAR_DEFINE;
				result->var.segment_of = false;
				result->var.external = current;
				result->wrt_section = WRT_DEFAULT;
				break;
			}
			return true;
		}
	}
	reference_clear(result);
	// TODO: sensible warning
	return false;
}

void evaluate_expression(expression_t * exp, reference_t * result, long here)
{
	reference_t a[1];
	if(exp == NULL)
	{
		int_init(result->value);
		result->var.type = VAR_NONE;
		return;
	}

	switch(exp->type)
	{
	case EXP_IDENTIFIER:
		label_lookup(exp->value.s, result);
		return;
	case EXP_SECTION:
		reference_set_ui(result, 0);
		result->var.type = VAR_SECTION;
		result->var.segment_of = false;
		result->var.internal.section_index = objfile_locate_section(&output, exp->value.s, SECTION_FAIL, (section_t) { });
		return;
	case EXP_INTEGER:
		reference_set(result, exp->value.i);
		result->var.type = VAR_NONE;
		result->var.segment_of = false;
		return;
	case EXP_STRING:
		{
#if USE_GMP
			mpz_t tmp;
			reference_clear(result);
			mpz_init(tmp);
			for(size_t i = 0; exp->value.s[i] != '\0' && i < sizeof(long); i++)
			{
				mpz_set_ui(tmp, exp->value.s[i] & 0xFF);
				mpz_mul_2exp(tmp, tmp, 8 * i);
				mpz_ior(result->value, result->value, tmp);
			}
			mpz_clear(tmp);
#else
			long value = 0;
			for(size_t i = 0; exp->value.s[i] != '\0' && i < sizeof(long); i++)
			{
				value |= (exp->value.s[i] & 0xFF) << (8 * i);
			}
			result->value = value;
#endif
		}
		return;
	case EXP_DEFINED:
		reference_set_ui(result, label_lookup(exp->value.s, a));
		reference_clear(a);
		result->var.type = VAR_NONE;
		result->var.segment_of = false;
		return;
	case EXP_HERE:
		reference_set_ui(result, here);
		result->var.type = VAR_SECTION;
		result->var.segment_of = false;
		result->var.internal.section_index = current_section;
		return;
	case EXP_SECTION_HERE:
		reference_set_ui(result, 0);
		result->var.type = VAR_SECTION;
		result->var.segment_of = false;
		result->var.internal.section_index = current_section;
		return;
	case EXP_LABEL:
		reference_set_ui(result, exp->value.l->code_offset);
		result->var.type = VAR_SECTION;
		result->var.segment_of = false;
		result->var.internal.section_index = exp->value.l->containing_section;
		return;
	case EXP_LABEL_FORWARD:
	case EXP_LABEL_BACKWARD:
		// these should not be evaluated
		reference_clear(result);
		return;
	case EXP_PLUS:
		evaluate_expression(exp->argument[0], result, here);
		return;
	case EXP_MINUS:
		evaluate_expression(exp->argument[0], result, here);
		assert_scalar(result);
#if USE_GMP
		mpz_neg(result->value, result->value);
#else
		result->value = -result->value;
#endif
		return;
	case EXP_BITNOT:
		evaluate_expression(exp->argument[0], result, here);
		assert_scalar(result);
#if USE_GMP
		mpz_com(result->value, result->value);
#else
		result->value = ~result->value;
#endif
		return;
	case EXP_NOT:
		evaluate_expression(exp->argument[0], result, here);
		assert_scalar(result);
#if USE_GMP
		mpz_set_ui(result->value, mpz_sgn(result->value) == 0);
#else
		result->value = result->value == 0;
#endif
		return;
	case EXP_SEG:
		label_lookup(exp->value.s, result);
#if USE_GMP
		mpz_set_ui(result->value, 0);
#else
		result->value = 0;
#endif
		result->var.segment_of = true;
		return;
	case EXP_WRT:
		label_lookup(exp->value.s, result);
		result->wrt_section = objfile_locate_section(&output, exp->argument[0]->value.s, SECTION_FAIL, (section_t) { });
		return;
	case EXP_ALIGN:
		{
			reference_t b[1];
			evaluate_expression(exp->argument[0], a, here);
			assert_scalar(a);
			evaluate_expression(exp->argument[1], b, here);
			assert_scalar(b);
#if USE_GMP
			integer_align_to(result->value, b->value, a->value);
			mpz_clear(a->value);
			mpz_clear(b->value);
#else
			result->value = align_to(b->value, a->value);
#endif

			result->var.type = VAR_NONE;
			result->wrt_section = WRT_DEFAULT;
		}
		return;
	case EXP_MUL:
		evaluate_expression(exp->argument[0], result, here);
		assert_scalar(result);
		evaluate_expression(exp->argument[1], a, here);
		assert_scalar(a);
#if USE_GMP
		mpz_mul(result->value, result->value, a->value);
		mpz_clear(a->value);
#else
		result->value *= a->value;
#endif
		return;
	case EXP_DIV:
#if !USE_GMP
		evaluate_expression(exp->argument[0], result, here);
		assert_scalar(result);
		evaluate_expression(exp->argument[1], a, here);
		assert_scalar(a);
		result->value /= a->value;
		return;
#endif
	case EXP_DIVU:
		evaluate_expression(exp->argument[0], result, here);
		assert_scalar(result);
		evaluate_expression(exp->argument[1], a, here);
		assert_scalar(a);
#if USE_GMP
		mpz_tdiv_q(result->value, result->value, a->value);
		mpz_clear(a->value);
#else
		result->value = (unsigned long)result->value / (unsigned long)a->value;
#endif
		return;
	case EXP_MOD:
#if !USE_GMP
		evaluate_expression(exp->argument[0], result, here);
		assert_scalar(result);
		evaluate_expression(exp->argument[1], a, here);
		assert_scalar(a);
		result->value %= a->value;
		return;
#endif
	case EXP_MODU:
		evaluate_expression(exp->argument[0], result, here);
		assert_scalar(result);
		evaluate_expression(exp->argument[1], a, here);
		assert_scalar(a);
#if USE_GMP
		mpz_tdiv_r(result->value, result->value, a->value);
		mpz_clear(a->value);
#else
		result->value = (unsigned long)result->value % (unsigned long)a->value;
#endif
		return;
	case EXP_SHL:
	case EXP_SHLU:
		evaluate_expression(exp->argument[0], result, here);
		assert_scalar(result);
		evaluate_expression(exp->argument[1], a, here);
		assert_scalar(a);
#if USE_GMP
		mpz_mul_2exp(result->value, result->value, mpz_get_si(a->value));
		mpz_clear(a->value);
#else
		result->value <<= a->value;
#endif
		return;
	case EXP_SHR:
#if !USE_GMP
		evaluate_expression(exp->argument[0], result, here);
		assert_scalar(result);
		evaluate_expression(exp->argument[1], a, here);
		assert_scalar(a);
		result->value >>= a->value;
#endif
		return;
	case EXP_SHRU:
		evaluate_expression(exp->argument[0], result, here);
		assert_scalar(result);
		evaluate_expression(exp->argument[1], a, here);
		assert_scalar(a);
#if USE_GMP
		mpz_fdiv_q_2exp(result->value, result->value, mpz_get_si(a->value));
		mpz_clear(a->value);
#else
		result->value = (unsigned long)result->value >> (unsigned long)a->value;
#endif
		return;
	case EXP_ADD:
		evaluate_expression(exp->argument[0], result, here);
		evaluate_expression(exp->argument[1], a, here);
		if(!is_scalar(result))
		{
			assert_scalar(a);
		}
		else
		{
			result->var = a->var;
			result->wrt_section = a->wrt_section;
		}
#if USE_GMP
		mpz_add(result->value, result->value, a->value);
		mpz_clear(a->value);
#else
		result->value += a->value;
#endif
		return;
	case EXP_SUB:
		evaluate_expression(exp->argument[0], result, here);
		evaluate_expression(exp->argument[1], a, here);
		if(is_scalar(result))
		{
			assert_scalar(a);
		}
		else if(!is_scalar(a))
		{
			if(is_preprocessing_stage)
			{
				fprintf(stderr, "Invalid arithmetic of non-scalar types in preprocessing stage\n");
				exit(1);
			}
			if(result->var.type != a->var.type || result->var.segment_of || a->var.segment_of)
			{
				fprintf(stderr, "Invalid arithmetic of non-scalar type\n");
			}
			else switch(result->var.type)
			{
			case VAR_NONE:
				break;
			case VAR_DEFINE:
				if(result->var.external != a->var.external)
				{
					fprintf(stderr, "Invalid arithmetic of non-scalar type\n");
				}
				break;
			case VAR_SECTION:
				if(result->var.internal.section_index != a->var.internal.section_index)
				{
					fprintf(stderr, "Invalid arithmetic of non-scalar type\n");
				}
				break;
			}
			result->var.type = VAR_NONE;
		}
#if USE_GMP
		mpz_sub(result->value, result->value, a->value);
		mpz_clear(a->value);
#else
		result->value -= a->value;
#endif
		return;
	case EXP_BITAND:
		evaluate_expression(exp->argument[0], result, here);
		assert_scalar(result);
		evaluate_expression(exp->argument[1], a, here);
		assert_scalar(a);
#if USE_GMP
		mpz_and(result->value, result->value, a->value);
		mpz_clear(a->value);
#else
		result->value &= a->value;
#endif
		return;
	case EXP_BITOR:
		evaluate_expression(exp->argument[0], result, here);
		assert_scalar(result);
		evaluate_expression(exp->argument[1], a, here);
		assert_scalar(a);
#if USE_GMP
		mpz_ior(result->value, result->value, a->value);
		mpz_clear(a->value);
#else
		result->value |= a->value;
#endif
		return;
	case EXP_BITXOR:
		evaluate_expression(exp->argument[0], result, here);
		assert_scalar(result);
		evaluate_expression(exp->argument[1], a, here);
		assert_scalar(a);
#if USE_GMP
		mpz_xor(result->value, result->value, a->value);
		mpz_clear(a->value);
#else
		result->value ^= a->value;
#endif
		return;
	case EXP_BITORNOT:
		evaluate_expression(exp->argument[0], result, here);
		assert_scalar(result);
		evaluate_expression(exp->argument[1], a, here);
		assert_scalar(a);
#if USE_GMP
		mpz_neg(a->value, a->value);
		mpz_ior(result->value, result->value, a->value);
		mpz_clear(a->value);
#else
		result->value |= ~a->value;
#endif
		return;
	case EXP_EQ:
		evaluate_expression(exp->argument[0], result, here);
		assert_scalar(result);
		evaluate_expression(exp->argument[1], a, here);
		assert_scalar(a);
#if USE_GMP
		mpz_set_ui(result->value, mpz_cmp(result->value, a->value) == 0);
		mpz_clear(a->value);
#else
		result->value = result->value == a->value;
#endif
		return;
	case EXP_NE:
		evaluate_expression(exp->argument[0], result, here);
		assert_scalar(result);
		evaluate_expression(exp->argument[1], a, here);
		assert_scalar(a);
#if USE_GMP
		mpz_set_ui(result->value, mpz_cmp(result->value, a->value) != 0);
		mpz_clear(a->value);
#else
		result->value = result->value != a->value;
#endif
		return;
	case EXP_LT:
#if !USE_GMP
		evaluate_expression(exp->argument[0], result, here);
		assert_scalar(result);
		evaluate_expression(exp->argument[1], a, here);
		assert_scalar(a);
		result->value = result->value < a->value;
		return;
#endif
	case EXP_LTU:
		evaluate_expression(exp->argument[0], result, here);
		assert_scalar(result);
		evaluate_expression(exp->argument[1], a, here);
		assert_scalar(a);
#if USE_GMP
		mpz_set_ui(result->value, mpz_cmp(result->value, a->value) < 0);
		mpz_clear(a->value);
#else
		result->value = (unsigned long)result->value < (unsigned long)a->value;
#endif
		return;
	case EXP_GT:
#if !USE_GMP
		evaluate_expression(exp->argument[0], result, here);
		assert_scalar(result);
		evaluate_expression(exp->argument[1], a, here);
		assert_scalar(a);
		result->value = result->value > a->value;
		return;
#endif
	case EXP_GTU:
		evaluate_expression(exp->argument[0], result, here);
		assert_scalar(result);
		evaluate_expression(exp->argument[1], a, here);
		assert_scalar(a);
#if USE_GMP
		mpz_set_ui(result->value, mpz_cmp(result->value, a->value) > 0);
		mpz_clear(a->value);
#else
		result->value = (unsigned long)result->value > (unsigned long)a->value;
#endif
		return;
	case EXP_LE:
#if !USE_GMP
		evaluate_expression(exp->argument[0], result, here);
		assert_scalar(result);
		evaluate_expression(exp->argument[1], a, here);
		assert_scalar(a);
		result->value = result->value <= a->value;
		return;
#endif
	case EXP_LEU:
		evaluate_expression(exp->argument[0], result, here);
		assert_scalar(result);
		evaluate_expression(exp->argument[1], a, here);
		assert_scalar(a);
#if USE_GMP
		mpz_set_ui(result->value, mpz_cmp(result->value, a->value) <= 0);
		mpz_clear(a->value);
#else
		result->value = (unsigned long)result->value <= (unsigned long)a->value;
#endif
		return;
	case EXP_GE:
#if !USE_GMP
		evaluate_expression(exp->argument[0], result, here);
		assert_scalar(result);
		evaluate_expression(exp->argument[1], a, here);
		assert_scalar(a);
		result->value = result->value >= a->value;
		return;
#endif
	case EXP_GEU:
		evaluate_expression(exp->argument[0], result, here);
		assert_scalar(result);
		evaluate_expression(exp->argument[1], a, here);
		assert_scalar(a);
#if USE_GMP
		mpz_set_ui(result->value, mpz_cmp(result->value, a->value) >= 0);
		mpz_clear(a->value);
#else
		result->value = (unsigned long)result->value >= (unsigned long)a->value;
#endif
		return;
	case EXP_CMP:
#if !USE_GMP
		{
			evaluate_expression(exp->argument[0], result, here);
			assert_scalar(result);
			evaluate_expression(exp->argument[1], a, here);
			assert_scalar(a);
			if(result->value < a->value)
				result->value = -1;
			else if(result->value == a->value)
				result->value = 0;
			else
				result->value = 1;
		}
		return;
#endif
	case EXP_CMPU:
		{
			evaluate_expression(exp->argument[0], result, here);
			assert_scalar(result);
			evaluate_expression(exp->argument[1], a, here);
			assert_scalar(a);
#if USE_GMP
			mpz_set_ui(result->value, mpz_cmp(result->value, a->value));
			mpz_clear(a->value);
#else
			if((unsigned long)result->value < (unsigned long)a->value)
				result->value = -1;
			else if(result->value == a->value)
				result->value = 0;
			else
				result->value = 1;
#endif
		}
		return;
	case EXP_AND:
		evaluate_expression(exp->argument[0], result, here);
		assert_scalar(result);
#if USE_GMP
		if(mpz_sgn(result->value) != 0)
#else
		if(result->value != 0)
#endif
		{
			evaluate_expression(exp->argument[1], a, here);
			assert_scalar(a);
#if USE_GMP
			mpz_set_ui(result->value, mpz_sgn(a->value) != 0);
#else
			result->value = a->value != 0;
#endif
		}
		else
		{
#if USE_GMP
			mpz_set_ui(result->value, 0);
#else
			result->value = 0;
#endif
		}
#if USE_GMP
		mpz_clear(a->value);
#endif
		return;
	case EXP_XOR:
		evaluate_expression(exp->argument[0], result, here);
		assert_scalar(result);
		evaluate_expression(exp->argument[1], a, here);
		assert_scalar(a);
#if USE_GMP
		mpz_set_ui(result->value, mpz_sgn(result->value) ^ mpz_sgn(a->value));
		mpz_clear(a->value);
#else
		result->value = (result->value != 0) ^ (a->value != 0);
#endif
		return;
	case EXP_OR:
		evaluate_expression(exp->argument[0], result, here);
		assert_scalar(result);
#if USE_GMP
		if(mpz_sgn(result->value) == 0)
#else
		if(result->value == 0)
#endif
		{
			evaluate_expression(exp->argument[1], a, here);
			assert_scalar(a);
#if USE_GMP
			mpz_set_ui(result->value, mpz_sgn(a->value) != 0);
#else
			result->value = a->value != 0;
#endif
		}
		else
		{
#if USE_GMP
			mpz_set_ui(result->value, 1);
#else
			result->value = 1;
#endif
		}
#if USE_GMP
		mpz_clear(a->value);
#endif
		return;
	case EXP_COND:
		evaluate_expression(exp->argument[0], result, here);
		assert_scalar(result);
#if USE_GMP
		if(mpz_sgn(result->value) != 0)
#else
		if(result->value != 0)
#endif
		{
			evaluate_expression(exp->argument[1], result, here);
		}
		else
		{
			evaluate_expression(exp->argument[2], result, here);
		}
#if USE_GMP
		mpz_clear(a->value);
#endif
		return;
	}
	assert(false);
}

// forward and backward references
typedef struct expression_list_t expression_list_t;
struct expression_list_t
{
	expression_t * expression;
	expression_list_t * next;
};

typedef struct local_label_definition_t local_label_definition_t;
struct local_label_definition_t
{
	integer_t label;
	instruction_t * backwards;
	expression_list_t * forwards;
	local_label_definition_t * next;
};

local_label_definition_t * local_label_definitions;

static void local_label_forwards_insert(local_label_definition_t * definition, expression_t * expression)
{
	expression_list_t * link = malloc(sizeof(expression_list_t));
	link->expression = expression;
	link->next = definition->forwards;
	definition->forwards = link;
}

static void local_label_forwards_insert_all(local_label_definition_t * definition)
{
	expression_list_t * link = definition->forwards;
	definition->forwards = NULL;
	while(link != NULL)
	{
		link->expression->type = EXP_LABEL;
		int_clear(link->expression->value.i);
		link->expression->value.l = definition->backwards;

		expression_list_t * tmp = link;
		link = link->next;
		free(tmp);
	}
}

static local_label_definition_t * local_label_locate(uinteger_t label)
{
	local_label_definition_t ** current;
	for(
		current = &local_label_definitions;
		*current != NULL &&
			uint_cmp((*current)->label, label) < 0;
		current = &(*current)->next)
	{
	}

	if(*current == NULL || uint_cmp((*current)->label, label) != 0)
	{
		local_label_definition_t * definition = malloc(sizeof(local_label_definition_t));
		memset(definition, 0, sizeof(local_label_definition_t));
		int_init_set(definition->label, label);
		definition->backwards = NULL;
		definition->next = *current;
		*current = definition;
	}
	return *current;
}

void local_label_define(instruction_t * ins)
{
	local_label_definition_t * definition = local_label_locate(ins->operand[0].parameter->value.i);

	definition->backwards = ins;

	if(definition->backwards != NULL)
	{
		local_label_forwards_insert_all(definition);
	}
}

static void local_label_lookup_backwards(expression_t * exp)
{
	assert(exp->type == EXP_LABEL_BACKWARD);

	local_label_definition_t * current;
	for(
		current = local_label_definitions;
		current != NULL &&
			uint_cmp(current->label, exp->value.i) < 0;
		current = current->next)
	{
	}

	if(
		current == NULL ||
		uint_cmp(current->label, exp->value.i) != 0)
	{
		fprintf(stderr, "Error: backward label not found\n");
		return;
	}

	exp->type = EXP_LABEL;
	int_clear(exp->value.i);
	exp->value.l = current->backwards;
}

void local_label_bind_all(expression_t * exp)
{
	if(exp == NULL)
		return;
	switch((int)exp->type)
	{
	case EXP_LABEL_BACKWARD:
		local_label_lookup_backwards(exp);
		break;
	case EXP_LABEL_FORWARD:
		local_label_forwards_insert(
			local_label_locate(exp->value.i),
			exp);
		break;
	case EXP_LIST:
		break;
	default:
		for(size_t argument_index = 0; argument_index < exp->argument_count; argument_index++)
			local_label_bind_all(exp->argument[argument_index]);
		break;
	}
}

