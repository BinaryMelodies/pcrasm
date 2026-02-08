
%code requires
{
#include "../../../src/preprocess.h"
}

%{
#include <assert.h>
#include <stdio.h>
#include "../../../src/syntax.h"
#include "../../../src/asm.h"

typedef enum operand_type_t operand_type_t;
typedef struct operand_t operand_t;

extern int yylex(void);
void yyerror(const char * s);
%}

%union
{
	long i;
	char * s;
	expression_t * x;

	integer_value_t j;

	instruction_t * ins;

	replacement_t * macro;
}

%token KWD_ALIGN
%token KWD_COMMON
%token <i> TOK_DATA
%token KWD_DEFINED
%token KWD_ELSE_IF
%token KWD_ELSE
%token KWD_ENDFILL
%token KWD_ENDIF
%token KWD_ENDMACRO
%token KWD_ENDREPEAT
%token KWD_ENDTIMES
%token KWD_ENTRY
%token KWD_EQU
%token KWD_EXPORT
%token KWD_EXTERN
%token KWD_FILL
%token KWD_GLOBAL
%token KWD_IF
%token KWD_IMPORT
%token KWD_MACRO
%token KWD_ORG
%token KWD_REPEAT
%token KWD_SECTION
%token KWD_SKIP
%token KWD_TIMES

%token <s> TOK_IDENTIFIER
%token <j> TOK_INTEGER TOK_LABEL_FORWARD TOK_LABEL_BACKWARD
%token <s> TOK_STRING
%token <macro> TOK_MACRONAME

%token DEL_AND "&&"
%token DEL_CMP "<=>"
%token DEL_EQ "=="
%token DEL_GE ">="
%token DEL_GE1 "=>"
%token DEL_LE "<="
%token DEL_LE1 "=<"
%token DEL_NE "!="
%token DEL_NE1 "<>"
%token DEL_NE2 "><"
%token DEL_OR "||"
%token DEL_SECT "$$"
%token DEL_SHL "<<"
%token DEL_SHR ">>"
%token DEL_XOR "^^"

%token DEL_CMP_U "#<=>"
%token DEL_GE_U "#>="
%token DEL_GE1_U "#=>"
%token DEL_LE_U "#<="
%token DEL_LE1_U "#=<"
%token DEL_LT_U "#<"
%token DEL_GT_U "#>"
%token DEL_SHL_S "<<<"
%token DEL_SHR_S ">>>"
%token DEL_DIV_S "//"
%token DEL_MOD_S "%%"

%right '?' ':'
%left "||"
%left "^^"
%left "&&"
%left "==" "!=" "<>" "><" '<' '>' "<=" "=<" ">=" "=>" "<=>" "#<" "#>" "#<=" "#=<" "#>=" "#=>" "#<=>"
%left '!' '|'
%left '^'
%left '&'
%left '+' '-'

%type <i> data
%type <x> prefix_expression_noparen prefix_expression multiplicative_expression multiplicative_expression_noparen multiplication addend expression expression_noparen operation_expression
%type <ins> section_directive common export_head export_directive

%%

file
	:
	| file line
	;

line
	: '\n'
		{
			advance_line();
		}
	| directive
	| data '\n'
		{
			advance_line();
		}
	| instruction
	| TOK_MACRONAME
		{
			invoke_macro($1);
			advance_line();
		}
	;

directive
	: section_directive '\n'
		{
			advance_line();
		}
	| KWD_EQU TOK_IDENTIFIER ',' expression '\n'
		{
			instruction_make_equ(&current_instruction, current_parser_state, $2, $4);
			instruction_stream_append(current_instruction_stream, &current_instruction);
			preprocessor_define($2, $4);
			advance_line();
		}
	| TOK_IDENTIFIER KWD_EQU expression '\n'
		{
			instruction_make_equ(&current_instruction, current_parser_state, $1, $3);
			instruction_stream_append(current_instruction_stream, &current_instruction);
			preprocessor_define($1, $3);
			advance_line();
		}
	| TOK_IDENTIFIER '=' expression '\n'
		{
			instruction_make_equ(&current_instruction, current_parser_state, $1, $3);
			instruction_stream_append(current_instruction_stream, &current_instruction);
			preprocessor_define($1, $3);
			advance_line();
		}
	| here KWD_EQU expression '\n'
		{
			instruction_make_set_location(&current_instruction, current_parser_state, $3);
			instruction_stream_append(current_instruction_stream, &current_instruction);
			advance_line();
		}
	| here '=' expression '\n'
		{
			instruction_make_set_location(&current_instruction, current_parser_state, $3);
			instruction_stream_append(current_instruction_stream, &current_instruction);
			advance_line();
		}
	| KWD_ORG expression '\n'
		{
			instruction_make_set_location(&current_instruction, current_parser_state, $2);
			instruction_stream_append(current_instruction_stream, &current_instruction);
			advance_line();
		}
	| KWD_ALIGN expression '\n'
		{
			instruction_make_skip(&current_instruction, current_parser_state,
				expression_binary(EXP_SUB,
					expression_binary(EXP_ALIGN, $2,
						expression_binary(EXP_SUB,
							expression_current_location(),
							expression_current_section())),
					expression_binary(EXP_SUB,
						expression_current_location(),
						expression_current_section())));
			instruction_stream_append(current_instruction_stream, &current_instruction);
			advance_line();
		}
	| KWD_ALIGN expression ','
		{
			instruction_make_fill(&current_instruction, current_parser_state,
				expression_binary(EXP_SUB,
					expression_binary(EXP_ALIGN, $2,
						expression_binary(EXP_SUB,
							expression_current_location(),
							expression_current_section())),
					expression_binary(EXP_SUB,
						expression_current_location(),
						expression_current_section())));
			instruction_stream_append(current_instruction_stream, &current_instruction);
		}
		data '\n'
		{
			instruction_make_end_fill(&current_instruction, current_parser_state);
			instruction_stream_append(current_instruction_stream, &current_instruction);
			advance_line();
		}
	| KWD_ALIGN expression ','
		{
			instruction_make_fill(&current_instruction, current_parser_state,
				expression_binary(EXP_SUB,
					expression_binary(EXP_ALIGN, $2,
						expression_binary(EXP_SUB,
							expression_current_location(),
							expression_current_section())),
					expression_binary(EXP_SUB,
						expression_current_location(),
						expression_current_section())));
			instruction_stream_append(current_instruction_stream, &current_instruction);
		}
		instruction
		{
			instruction_stream_append(current_instruction_stream, &current_instruction);

			instruction_make_end_fill(&current_instruction, current_parser_state);
			instruction_stream_append(current_instruction_stream, &current_instruction);
		}
	| KWD_TIMES expression '\n'
		{
			instruction_make_times(&current_instruction, current_parser_state, $2);
			instruction_stream_append(current_instruction_stream, &current_instruction);
			advance_line();
		}
	| KWD_ENDTIMES '\n'
		{
			instruction_make_end_times(&current_instruction, current_parser_state);
			instruction_stream_append(current_instruction_stream, &current_instruction);
			advance_line();
		}
	| KWD_REPEAT expression '\n'
		{
			/* TODO */
			advance_line();
		}
	| KWD_ENDREPEAT '\n'
		{
			/* TODO */
			advance_line();
		}
	| KWD_FILL expression '\n'
		{
			instruction_make_fill(&current_instruction, current_parser_state, $2);
			instruction_stream_append(current_instruction_stream, &current_instruction);
			advance_line();
		}
	| KWD_ENDFILL '\n'
		{
			instruction_make_end_fill(&current_instruction, current_parser_state);
			instruction_stream_append(current_instruction_stream, &current_instruction);
			advance_line();
		}
	| KWD_IF expression '\n'
		{
			handle_if_directive($2);
			// TODO: optionally delete $2
			advance_line();
		}
	| KWD_ELSE_IF expression '\n'
		{
			handle_else_if_directive($2);
			// TODO: optionally delete $2
			advance_line();
		}
	| KWD_ELSE '\n'
		{
			handle_else_directive();
			advance_line();
		}
	| KWD_ENDIF '\n'
		{
			handle_end_if_directive();
			advance_line();
		}
	| KWD_SKIP expression '\n'
		{
			instruction_make_skip(&current_instruction, current_parser_state, $2);
			instruction_stream_append(current_instruction_stream, &current_instruction);
			advance_line();
		}
	| KWD_TIMES expression ','
		{
			instruction_make_times(&current_instruction, current_parser_state, $2);
			instruction_stream_append(current_instruction_stream, &current_instruction);
		}
		data '\n'
		{
			instruction_make_end_times(&current_instruction, current_parser_state);
			instruction_stream_append(current_instruction_stream, &current_instruction);
			advance_line();
		}
	| KWD_TIMES expression ','
		{
			instruction_make_times(&current_instruction, current_parser_state, $2);
			instruction_stream_append(current_instruction_stream, &current_instruction);
		}
		instruction
		{
			instruction_stream_append(current_instruction_stream, &current_instruction);

			instruction_make_end_times(&current_instruction, current_parser_state);
			instruction_stream_append(current_instruction_stream, &current_instruction);
		}
	| KWD_ENTRY TOK_IDENTIFIER '\n'
		{
			if(entry_point_name == NULL)
			{
				entry_point_name = $2;
			}
			else
			{
				yyerror("Error: repeated .entry directive\n");
				if(!is_replacement)
					free($2);
			}
			advance_line();
		}
	| TOK_IDENTIFIER ':'
		{
			instruction_make_equ(&current_instruction, current_parser_state, $1, expression_current_location());
			instruction_stream_append(current_instruction_stream, &current_instruction);
			preprocessor_define($1, expression_current_location());
		}
	| TOK_INTEGER ':'
		{
			instruction_make_local_label(&current_instruction, current_parser_state, $1);
			instruction_stream_append(current_instruction_stream, &current_instruction);
			if(!is_replacement)
				int_delete($1);
		}
	| extern '\n'
		{
			advance_line();
		}
	| global '\n'
		{
			advance_line();
		}
	| common '\n'
		{
			advance_line();
		}
	| import '\n'
		{
			advance_line();
		}
	| export '\n'
		{
			advance_line();
		}
	| macro_definition
		{
			parse_macro_definition();
		}
		'\n'
		{
			advance_line();
		}
	;

macro_definition
	: macro_definition_head
	| macro_definition_arguments
	;

macro_definition_head
	: KWD_MACRO TOK_IDENTIFIER
		{
			begin_macro_definition($2);
		}
	;

macro_definition_arguments
	: macro_definition_head TOK_IDENTIFIER
		{
			append_macro_definition_parameter($2);
		}
	| macro_definition_arguments comma_opt TOK_IDENTIFIER
		{
			append_macro_definition_parameter($3);
		}
	;

section_directive
	: KWD_SECTION TOK_IDENTIFIER
		{
			instruction_make_section(&current_instruction, current_parser_state, $2);
			$$ = instruction_stream_append(current_instruction_stream, &current_instruction);
		}
	| section_directive comma_opt TOK_IDENTIFIER
		{
			expression_list_append($1->operand[1].parameter, expression_identifier($3));
		}
	| section_directive comma_opt TOK_IDENTIFIER '=' TOK_IDENTIFIER
		{
			expression_list_append($1->operand[1].parameter,
				expression_assignment(
					expression_identifier($3),
					expression_identifier($5)));
		}
	| section_directive comma_opt TOK_IDENTIFIER '=' TOK_INTEGER
		{
			expression_list_append($1->operand[1].parameter,
				expression_assignment(
					expression_identifier($3),
					expression_integer($5)));
		}
	;

comma_opt
	:
	| ','
	;

data
	: TOK_DATA expression
		{
			instruction_make_data(&current_instruction, current_parser_state, $1, $2);
			instruction_stream_append(current_instruction_stream, &current_instruction);
			$$ = $1;
		}
	| data ',' expression
		{
			instruction_make_data(&current_instruction, current_parser_state, $1, $3);
			instruction_stream_append(current_instruction_stream, &current_instruction);
			$$ = $1;
		}
	;

extern
	: KWD_EXTERN TOK_IDENTIFIER
		{
			instruction_make_external(&current_instruction, current_parser_state, $2);
			instruction_stream_append(current_instruction_stream, &current_instruction);
		}
	| extern ',' TOK_IDENTIFIER
		{
			instruction_make_external(&current_instruction, current_parser_state, $3);
			instruction_stream_append(current_instruction_stream, &current_instruction);
		}
	;

global
	: KWD_GLOBAL TOK_IDENTIFIER
		{
			instruction_make_global(&current_instruction, current_parser_state, $2);
			instruction_stream_append(current_instruction_stream, &current_instruction);
		}
	| global ',' TOK_IDENTIFIER
		{
			instruction_make_global(&current_instruction, current_parser_state, $3);
			instruction_stream_append(current_instruction_stream, &current_instruction);
		}
	;

common
	: KWD_COMMON TOK_IDENTIFIER expression
		{
			instruction_make_common(&current_instruction, current_parser_state, $2, $3);
			$$ = instruction_stream_append(current_instruction_stream, &current_instruction);
		}
	| common comma_opt TOK_IDENTIFIER
		{
			expression_list_append($1->operand[2].parameter, expression_identifier($3));
			$$ = $1;
		}
	| common comma_opt TOK_IDENTIFIER '=' TOK_IDENTIFIER
		{
			expression_list_append($1->operand[2].parameter,
				expression_assignment(
					expression_identifier($3),
					expression_identifier($5)));
			$$ = $1;
		}
	| common comma_opt TOK_IDENTIFIER '=' TOK_INTEGER
		{
			expression_list_append($1->operand[2].parameter,
				expression_assignment(
					expression_identifier($3),
					expression_integer($5)));
			$$ = $1;
		}
	;

import
	: KWD_IMPORT TOK_IDENTIFIER TOK_IDENTIFIER
		{
			instruction_make_import(&current_instruction, current_parser_state, $2, $3, NULL);
			instruction_stream_append(current_instruction_stream, &current_instruction);
		}
	| KWD_IMPORT TOK_IDENTIFIER TOK_IDENTIFIER TOK_IDENTIFIER
		{
			instruction_make_import(&current_instruction, current_parser_state, $2, $3, expression_identifier($4));
			instruction_stream_append(current_instruction_stream, &current_instruction);
		}
	| KWD_IMPORT TOK_IDENTIFIER TOK_IDENTIFIER TOK_INTEGER
		{
			instruction_make_import(&current_instruction, current_parser_state, $2, $3, expression_integer($4));
			instruction_stream_append(current_instruction_stream, &current_instruction);
		}
	;

export_head
	: KWD_EXPORT TOK_IDENTIFIER
		{
			instruction_make_export(&current_instruction, current_parser_state, $2, NULL, NO_ORDINAL);
			$$ = instruction_stream_append(current_instruction_stream, &current_instruction);
		}
	| KWD_EXPORT TOK_IDENTIFIER TOK_IDENTIFIER
		{
			instruction_make_export(&current_instruction, current_parser_state, $2, $3, NO_ORDINAL);
			$$ = instruction_stream_append(current_instruction_stream, &current_instruction);
		}
	| KWD_EXPORT TOK_IDENTIFIER TOK_INTEGER
		{
			instruction_make_export(&current_instruction, current_parser_state, $2, NULL, $3);
			$$ = instruction_stream_append(current_instruction_stream, &current_instruction);
			if(!is_replacement)
				int_delete($3);
		}
	| KWD_EXPORT TOK_IDENTIFIER TOK_IDENTIFIER TOK_INTEGER
		{
			instruction_make_export(&current_instruction, current_parser_state, $2, $3, $4);
			$$ = instruction_stream_append(current_instruction_stream, &current_instruction);
			if(!is_replacement)
				int_delete($4);
		}
	;

export_directive
	: export_head ',' TOK_IDENTIFIER
		{
			expression_list_append($1->operand[3].parameter, expression_identifier($3));
			$$ = $1;
		}
	| export_directive comma_opt TOK_IDENTIFIER
		{
			expression_list_append($1->operand[3].parameter, expression_identifier($3));
			$$ = $1;
		}
	| export_head ',' TOK_IDENTIFIER '=' TOK_IDENTIFIER
		{
			expression_list_append($1->operand[3].parameter,
				expression_assignment(
					expression_identifier($3),
					expression_identifier($5)));
			$$ = $1;
		}
	| export_directive comma_opt TOK_IDENTIFIER '=' TOK_IDENTIFIER
		{
			expression_list_append($1->operand[3].parameter,
				expression_assignment(
					expression_identifier($3),
					expression_identifier($5)));
			$$ = $1;
		}
	| export_head ',' TOK_IDENTIFIER '=' TOK_INTEGER
		{
			expression_list_append($1->operand[3].parameter,
				expression_assignment(
					expression_identifier($3),
					expression_integer($5)));
			$$ = $1;
		}
	| export_directive comma_opt TOK_IDENTIFIER '=' TOK_INTEGER
		{
			expression_list_append($1->operand[3].parameter,
				expression_assignment(
					expression_identifier($3),
					expression_integer($5)));
			$$ = $1;
		}
	;

export
	: export_head
	| export_directive
	;

here
	: '*' // xa
	| '.' // gnu
	| '$' // nasm
	;

prefix_expression_noparen
	: TOK_IDENTIFIER
		{
			$$ = expression_identifier($1);
		}
	| KWD_SECTION TOK_IDENTIFIER
		{
			$$ = expression_section($2);
		}
	| TOK_LABEL_FORWARD
		{
			$$ = expression_label_forward($1);
			if(!is_replacement)
				int_delete($1);
		}
	| TOK_LABEL_BACKWARD
		{
			$$ = expression_label_backward($1);
			if(!is_replacement)
				int_delete($1);
		}
	| TOK_INTEGER
		{
			$$ = expression_integer($1);
			if(!is_replacement)
				int_delete($1);
		}
	| TOK_STRING
		{
			$$ = expression_string($1);
		}
	| KWD_DEFINED TOK_IDENTIFIER
		{
			$$ = expression_is_defined($2);
		}
	| here
		{
			$$ = expression_current_location();
		}
	| "$$" // nasm
		{
			$$ = expression_current_section();
		}
	| '+' prefix_expression // nasm
		{
			$$ = expression_unary(EXP_PLUS, $2);
		}
	| '-' prefix_expression
		{
			$$ = expression_unary(EXP_MINUS, $2);
		}
	| '~' prefix_expression
		{
			$$ = expression_unary(EXP_BITNOT, $2);
		}
	| '!' prefix_expression // nasm
		{
			$$ = expression_unary(EXP_NOT, $2);
		}
	| KWD_ALIGN '(' expression ')'
		{
			$$ = expression_binary(EXP_ALIGN, $3, expression_current_location());
		}
	| KWD_ALIGN '(' expression ',' expression ')'
		{
			$$ = expression_binary(EXP_ALIGN, $3, $5);
		}
	;

prefix_expression
	: prefix_expression_noparen
	| '(' expression ')'
		{
			$$ = $2;
		}
	;

multiplication
	: multiplicative_expression '*' prefix_expression
		{
			$$ = expression_binary(EXP_MUL, $1, $3);
		}
	| multiplicative_expression '/' prefix_expression
		{
			$$ = expression_binary(EXP_DIVU, $1, $3);
		}
	| multiplicative_expression "//" prefix_expression
		{
			$$ = expression_binary(EXP_DIV, $1, $3);
		}
	| multiplicative_expression '%' prefix_expression
		{
			$$ = expression_binary(EXP_MODU, $1, $3);
		}
	| multiplicative_expression "%%" prefix_expression
		{
			$$ = expression_binary(EXP_MOD, $1, $3);
		}
	| multiplicative_expression "<<" prefix_expression
		{
			$$ = expression_binary(EXP_SHLU, $1, $3);
		}
	| multiplicative_expression "<<<" prefix_expression
		{
			$$ = expression_binary(EXP_SHL, $1, $3);
		}
	| multiplicative_expression ">>" prefix_expression
		{
			$$ = expression_binary(EXP_SHRU, $1, $3);
		}
	| multiplicative_expression ">>>" prefix_expression
		{
			$$ = expression_binary(EXP_SHR, $1, $3);
		}
	;

multiplicative_expression
	: prefix_expression
	| multiplication
	;

multiplicative_expression_noparen
	: prefix_expression_noparen
	| multiplication
	;

addend
	: '+' multiplicative_expression
		{
			$$ = $2;
		}
	| '-' multiplicative_expression
		{
			$$ = expression_unary(EXP_MINUS, $2);
		}
	| addend '+' multiplicative_expression
		{
			$$ = expression_binary(EXP_ADD, $1, $3);
		}
	| addend '-' multiplicative_expression
		{
			$$ = expression_binary(EXP_SUB, $1, $3);
		}
	;

operation_expression
	: expression '+' expression
		{
			$$ = expression_binary(EXP_ADD, $1, $3);
		}
	| expression '-' expression
		{
			$$ = expression_binary(EXP_SUB, $1, $3);
		}
	| expression '&' expression
		{
			$$ = expression_binary(EXP_BITAND, $1, $3);
		}
	| expression '|' expression
		{
			$$ = expression_binary(EXP_BITOR, $1, $3);
		}
	| expression '^' expression
		{
			$$ = expression_binary(EXP_BITXOR, $1, $3);
		}
	| expression '!' expression // gas
		{
			$$ = expression_binary(EXP_BITORNOT, $1, $3);
		}
	| expression "==" expression
		{
			$$ = expression_binary(EXP_EQ, $1, $3);
		}
	| expression "!=" expression
		{
			$$ = expression_binary(EXP_NE, $1, $3);
		}
	| expression "<>" expression
		{
			$$ = expression_binary(EXP_NE, $1, $3);
		}
	| expression "><" expression // xa
		{
			$$ = expression_binary(EXP_NE, $1, $3);
		}
	| expression '<' expression
		{
			$$ = expression_binary(EXP_LT, $1, $3);
		}
	| expression '>' expression
		{
			$$ = expression_binary(EXP_GT, $1, $3);
		}
	| expression "<=" expression
		{
			$$ = expression_binary(EXP_LE, $1, $3);
		}
	| expression "=<" expression // xa
		{
			$$ = expression_binary(EXP_LE, $1, $3);
		}
	| expression ">=" expression
		{
			$$ = expression_binary(EXP_GE, $1, $3);
		}
	| expression "=>" expression // xa
		{
			$$ = expression_binary(EXP_GE, $1, $3);
		}
	| expression "<=>" expression // nasm
		{
			$$ = expression_binary(EXP_CMP, $1, $3);
		}
	| expression "#<" expression
		{
			$$ = expression_binary(EXP_LTU, $1, $3);
		}
	| expression "#>" expression
		{
			$$ = expression_binary(EXP_GTU, $1, $3);
		}
	| expression "#<=" expression
		{
			$$ = expression_binary(EXP_LEU, $1, $3);
		}
	| expression "#=<" expression // xa
		{
			$$ = expression_binary(EXP_LEU, $1, $3);
		}
	| expression "#>=" expression
		{
			$$ = expression_binary(EXP_GEU, $1, $3);
		}
	| expression "#=>" expression // xa
		{
			$$ = expression_binary(EXP_GEU, $1, $3);
		}
	| expression "#<=>" expression // nasm
		{
			$$ = expression_binary(EXP_CMPU, $1, $3);
		}
	| expression "&&" expression
		{
			$$ = expression_binary(EXP_AND, $1, $3);
		}
	| expression "^^" expression // nasm
		{
			$$ = expression_binary(EXP_XOR, $1, $3);
		}
	| expression "||" expression
		{
			$$ = expression_binary(EXP_OR, $1, $3);
		}
	| expression '?' expression ':' expression
		{
			$$ = expression_conditional($1, $3, $5);
		}
	;

expression
	: multiplicative_expression
	| operation_expression
	;

expression_noparen
	: multiplicative_expression_noparen
	| operation_expression
	;

%%

void yyerror(const char * s)
{
	fprintf(stderr, "Error in line %ld: %s\n", current_parser_state->line_number, s);
}

