#ifndef _SYNTAX_H
#define _SYNTAX_H

#include "asm.h"

expression_t * expression_allocate(expression_type_t type);
expression_t * expression_identifier(char * name);
expression_t * expression_section(char * name);
expression_t * expression_integer(integer_value_t value);
expression_t * expression_string(char * value);
expression_t * expression_is_defined(char * name);
expression_t * expression_current_location(void);
expression_t * expression_current_section(void);
expression_t * expression_segment_of(char * name);
expression_t * expression_relative_to(char * name, char * section_name);
expression_t * expression_unary(expression_type_t type, expression_t * argument);
expression_t * expression_binary(expression_type_t type, expression_t * argument0, expression_t * argument1);
expression_t * expression_conditional(expression_t * argument0, expression_t * argument1, expression_t * argument2);
expression_t * expression_label_forward(integer_value_t label);
expression_t * expression_label_backward(integer_value_t label);

expression_t * expression_assignment(expression_t * key, expression_t * value);
expression_t * expression_make_list(void);
expression_t * expression_list_append(expression_t * expression, expression_t * next);

instruction_t * instruction_clone(instruction_t * ins);
void instruction_clear(instruction_t * ins, parser_state_t * state);

void instruction_make_equ(instruction_t * ins, parser_state_t * state, char * name, expression_t * value);
void instruction_make_data(instruction_t * ins, parser_state_t * state, int format, expression_t * value);
void instruction_make_set_location(instruction_t * ins, parser_state_t * state, expression_t * target);
void instruction_make_fill(instruction_t * ins, parser_state_t * state, expression_t * count);
void instruction_make_end_fill(instruction_t * ins, parser_state_t * state);
void instruction_make_skip(instruction_t * ins, parser_state_t * state, expression_t * count);
void instruction_make_times(instruction_t * ins, parser_state_t * state, expression_t * count);
void instruction_make_end_times(instruction_t * ins, parser_state_t * state);
void instruction_make_if(instruction_t * ins, parser_state_t * state, expression_t * condition);
void instruction_make_else_if(instruction_t * ins, parser_state_t * state, expression_t * condition);
void instruction_make_else(instruction_t * ins, parser_state_t * state);
void instruction_make_end_if(instruction_t * ins, parser_state_t * state);
void instruction_make_local_label(instruction_t * ins, parser_state_t * state, integer_value_t label);
void instruction_make_section(instruction_t * ins, parser_state_t * state, char * name);
void instruction_make_external(instruction_t * ins, parser_state_t * state, char * name);
void instruction_make_global(instruction_t * ins, parser_state_t * state, char * name);
void instruction_make_common(instruction_t * ins, parser_state_t * state, char * name, expression_t * size);
void instruction_make_import(instruction_t * ins, parser_state_t * state, char * name, char * module_name, expression_t * import_name);
void instruction_make_export(instruction_t * ins, parser_state_t * state, char * name, char * export_name, integer_value_t optional_ordinal);
instruction_t * instruction_stream_append(instruction_stream_t * stream, instruction_t * ins);

#endif // _SYNTAX_H
