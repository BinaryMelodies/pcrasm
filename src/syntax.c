
#include "asm.h"
#include "isa.h"

instruction_t current_instruction;

expression_t * expression_allocate(expression_type_t type)
{
	expression_t * expression = malloc(sizeof(expression_t));
	memset(expression, 0, sizeof(expression_t));
	expression->type = type;
	return expression;
}

expression_t * expression_identifier(char * name)
{
	expression_t * expression = expression_allocate(EXP_IDENTIFIER);
	expression->value.s = name;
	return expression;
}

expression_t * expression_section(char * name)
{
	expression_t * expression = expression_allocate(EXP_SECTION);
	expression->value.s = name;
	return expression;
}

expression_t * expression_integer(integer_value_t value)
{
	expression_t * expression = expression_allocate(EXP_INTEGER);
	int_init_set(expression->value.i, INTVAL(value));
	return expression;
}

expression_t * expression_string(char * value)
{
	expression_t * expression = expression_allocate(EXP_STRING);
	expression->value.s = value;
	return expression;
}

expression_t * expression_is_defined(char * name)
{
	expression_t * expression = expression_allocate(EXP_DEFINED);
	expression->value.s = name;
	return expression;
}

expression_t * expression_current_location(void)
{
	return expression_allocate(EXP_HERE);
}

expression_t * expression_current_section(void)
{
	return expression_allocate(EXP_SECTION_HERE);
}

expression_t * expression_segment_of(char * name)
{
	expression_t * expression = expression_allocate(EXP_SEG);
	expression->value.s = name;
	return expression;
}

expression_t * expression_relative_to(char * name, char * section_name)
{
	expression_t * expression = expression_allocate(EXP_WRT);
	expression->value.s = name;
	expression->argument_count = 1;
	expression->argument[0] = expression_segment_of(section_name);
	return expression;
}

expression_t * expression_unary(expression_type_t type, expression_t * argument)
{
	expression_t * expression = expression_allocate(type);
	expression->argument_count = 1;
	expression->argument[0] = argument;
	return expression;
}

expression_t * expression_binary(expression_type_t type, expression_t * argument0, expression_t * argument1)
{
	expression_t * expression = expression_allocate(type);
	expression->argument_count = 2;
	expression->argument[0] = argument0;
	expression->argument[1] = argument1;
	return expression;
}

expression_t * expression_conditional(expression_t * argument0, expression_t * argument1, expression_t * argument2)
{
	expression_t * expression = expression_allocate(EXP_COND);
	expression->argument_count = 3;
	expression->argument[0] = argument0;
	expression->argument[1] = argument1;
	expression->argument[2] = argument2;
	return expression;
}

expression_t * expression_label_forward(integer_value_t label)
{
	expression_t * expression = expression_allocate(EXP_LABEL_FORWARD);
	int_init_set(expression->value.i, INTVAL(label));
	return expression;
}

expression_t * expression_label_backward(integer_value_t label)
{
	expression_t * expression = expression_allocate(EXP_LABEL_BACKWARD);
	int_init_set(expression->value.i, INTVAL(label));
	return expression;
}

expression_t * expression_assignment(expression_t * key, expression_t * value)
{
	expression_t * expression = expression_allocate(EXP_ASSIGN);
	expression->argument_count = 2;
	expression->argument[0] = key;
	expression->argument[1] = value;
	return expression;
}

/*
	lists are stored as singly linked lists of LISP-style CONS cells:
		argument[0]: initial element of list
		argument[1]: remainder of list or NULL
	furthermore, for the actual first CONS cell of the LISP:
		argument[2]: address of last CONS cell in list
	an empty list can be represented by setting the argument_count to 0, however the last CONS cell must always terminate with NULL, not an empty list
*/

expression_t * expression_make_list(void)
{
	expression_t * expression = expression_allocate(EXP_LIST);
	expression->argument_count = 0;
	expression->argument[0] = NULL;
	expression->argument[1] = NULL;
	expression->argument[2] = NULL;
	return expression;
}

static expression_t * expression_make_list_cons(expression_t * head, expression_t * tail)
{
	expression_t * expression = expression_allocate(EXP_LIST);
	expression->argument_count = 2;
	expression->argument[0] = head;
	expression->argument[1] = tail;
	return expression;
}

expression_t * expression_list_append(expression_t * expression, expression_t * next)
{
	if(expression->argument_count == 0)
	{
		expression->argument_count = 3;
		expression->argument[0] = next;
		expression->argument[1] = NULL;
		expression->argument[2] = expression;
	}
	else
	{
		expression->argument[2] =
			expression->argument[2]->argument[1] = expression_make_list_cons(next, NULL);
	}
	return expression;
}

instruction_t * instruction_clone(instruction_t * ins)
{
	instruction_t * instruction = malloc(sizeof(instruction_t));
	memcpy(instruction, ins, sizeof(instruction_t));
	return instruction;
}

void instruction_make_equ(instruction_t * ins, parser_state_t * state, char * name, expression_t * value)
{
	instruction_clear(ins, state);
	ins->mnemonic = PSEUDO_MNEM_EQU;
	ins->operand_count = 2;
	ins->operand[0].parameter = expression_identifier(name);
	ins->operand[1].parameter = value;
}

void instruction_make_data(instruction_t * ins, parser_state_t * state, int format, expression_t * value)
{
	instruction_clear(ins, state);
	ins->mnemonic = PSEUDO_MNEM_DATA(format);
	ins->operand_count = 1;
	ins->operand[0].parameter = value;
}

void instruction_make_set_location(instruction_t * ins, parser_state_t * state, expression_t * target)
{
	instruction_clear(ins, state);
	ins->mnemonic = PSEUDO_MNEM_ORG;
	ins->operand_count = 1;
	ins->operand[0].parameter = target;
}

void instruction_make_skip(instruction_t * ins, parser_state_t * state, expression_t * count)
{
	instruction_clear(ins, state);
	ins->mnemonic = PSEUDO_MNEM_SKIP;
	ins->operand_count = 1;
	ins->operand[0].parameter = count;
}

void instruction_make_fill(instruction_t * ins, parser_state_t * state, expression_t * count)
{
	instruction_clear(ins, state);
	ins->mnemonic = PSEUDO_MNEM_FILL;
	ins->operand_count = 1;
	ins->operand[0].parameter = count;
}

void instruction_make_end_fill(instruction_t * ins, parser_state_t * state)
{
	instruction_clear(ins, state);
	ins->mnemonic = PSEUDO_MNEM_END_FILL;
	ins->operand_count = 0;
}

void instruction_make_times(instruction_t * ins, parser_state_t * state, expression_t * count)
{
	instruction_clear(ins, state);
	ins->mnemonic = PSEUDO_MNEM_TIMES;
	ins->operand_count = 1;
	ins->operand[0].parameter = count;
}

void instruction_make_end_times(instruction_t * ins, parser_state_t * state)
{
	instruction_clear(ins, state);
	ins->mnemonic = PSEUDO_MNEM_END_TIMES;
	ins->operand_count = 0;
}

#if 0
void instruction_make_if(instruction_t * ins, parser_state_t * state, expression_t * condition)
{
	instruction_clear(ins, state);
	ins->mnemonic = PSEUDO_MNEM_IF;
	ins->operand_count = 1;
	ins->operand[0].parameter = condition;
}

void instruction_make_else_if(instruction_t * ins, parser_state_t * state, expression_t * condition)
{
	instruction_clear(ins, state);
	ins->mnemonic = PSEUDO_MNEM_ELSE_IF;
	ins->operand_count = 1;
	ins->operand[0].parameter = condition;
}

void instruction_make_else(instruction_t * ins, parser_state_t * state)
{
	instruction_clear(ins, state);
	ins->mnemonic = PSEUDO_MNEM_ELSE;
	ins->operand_count = 0;
}

void instruction_make_end_if(instruction_t * ins, parser_state_t * state)
{
	instruction_clear(ins, state);
	ins->mnemonic = PSEUDO_MNEM_END_IF;
	ins->operand_count = 0;
}
#endif

void instruction_make_local_label(instruction_t * ins, parser_state_t * state, integer_value_t label)
{
	instruction_clear(ins, state);
	ins->mnemonic = PSEUDO_MNEM_LOCAL_LABEL;
	ins->operand_count = 1;
	ins->operand[0].parameter = expression_integer(label);
}

void instruction_make_section(instruction_t * ins, parser_state_t * state, char * name)
{
	instruction_clear(ins, state);
	ins->mnemonic = PSEUDO_MNEM_SECTION;
	ins->operand_count = 2;
	ins->operand[0].parameter = expression_string(name);
	ins->operand[1].parameter = expression_make_list();
}

void instruction_make_external(instruction_t * ins, parser_state_t * state, char * name)
{
	instruction_clear(ins, state);
	ins->mnemonic = PSEUDO_MNEM_EXTERNAL;
	ins->operand_count = 1;
	ins->operand[0].parameter = expression_string(name);
}

void instruction_make_global(instruction_t * ins, parser_state_t * state, char * name)
{
	instruction_clear(ins, state);
	ins->mnemonic = PSEUDO_MNEM_GLOBAL;
	ins->operand_count = 1;
	ins->operand[0].parameter = expression_string(name);
}

void instruction_make_common(instruction_t * ins, parser_state_t * state, char * name, expression_t * size)
{
	instruction_clear(ins, state);
	ins->mnemonic = PSEUDO_MNEM_COMMON;
	ins->operand_count = 3;
	ins->operand[0].parameter = expression_string(name);
	ins->operand[1].parameter = size;
	ins->operand[2].parameter = expression_make_list();
}

void instruction_make_import(instruction_t * ins, parser_state_t * state, char * name, char * module_name, expression_t * import_name)
{
	instruction_clear(ins, state);
	ins->mnemonic = PSEUDO_MNEM_IMPORT;
	ins->operand_count = 3;
	ins->operand[0].parameter = expression_string(name);
	ins->operand[1].parameter = expression_string(module_name);
	ins->operand[2].parameter = import_name;
}

void instruction_make_export(instruction_t * ins, parser_state_t * state, char * name, char * export_name, integer_value_t optional_ordinal)
{
	instruction_clear(ins, state);
	ins->mnemonic = PSEUDO_MNEM_EXPORT;
	ins->operand_count = 4;
	ins->operand[0].parameter = expression_string(name);
	ins->operand[1].parameter = expression_string(export_name);
	if(optional_ordinal == NO_ORDINAL)
		ins->operand[2].parameter = NULL;
	else
		ins->operand[2].parameter = expression_integer(optional_ordinal);
	ins->operand[3].parameter = expression_make_list();
}

instruction_t * instruction_stream_append(instruction_stream_t * stream, instruction_t * ins)
{
	*stream->last_instruction = ins = instruction_clone(ins);
	ins->next = NULL;
	stream->last_instruction = &ins->next;
	return ins;
}

