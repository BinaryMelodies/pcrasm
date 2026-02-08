
%include "../parser.y"

%{
#include "../../../src/x65/isa.h"

void operand_set(operand_t * opd, operand_type_t type);
void operand_set_immediate(operand_t * opd, expression_t * value);
void operand_set_memory(operand_t * opd);

parser_state_t current_parser_state[1];
extern instruction_t current_instruction;
#define current_operand (current_instruction.operand[current_instruction.operand_count])
#define current_instruction_stream (&current_parser_state->stream)
%}

%token <i> TOK_ARCH
%token KWD_WIDTH

%token KWD_LOW
%token KWD_HIGH
%token KWD_ADDR
%token KWD_BANK
%token TOK_AREG
%token TOK_SREG
%token TOK_XREG
%token TOK_YREG
%token TOK_ZREG
%token <i> TOK_MNEM

%%

%include "../parser.y"

directive
	: TOK_ARCH '\n'
		{
			current_parser_state->cpu_type = $1;
			if(current_parser_state->cpu_type != CPU_65C816)
			{
				current_parser_state->abits = BITSIZE8;
				current_parser_state->xbits = BITSIZE8;
			}
			setup_lexer(current_parser_state);
			advance_line();
		}
	| KWD_WIDTH width_declaration '\n'
		{
			advance_line();
		}
	;

data
	: TOK_DATA
		{
			instruction_make_data(&current_instruction, current_parser_state, $1, NULL);
			current_instruction.operand_count = 0;
		}
		sized_expression
		{
			instruction_stream_append(current_instruction_stream, &current_instruction);
			$$ = $1;
		}
	| data ','
		{
			instruction_make_data(&current_instruction, current_parser_state, $1, NULL);
			current_instruction.operand_count = 0;
		}
		sized_expression
		{
			instruction_stream_append(current_instruction_stream, &current_instruction);
			$$ = $1;
		}
	;

width_declaration
	: a_width
	| x_width
	| a_width ',' x_width
	| x_width ',' a_width
	;

a_width
	: TOK_AREG '=' TOK_INTEGER
		{
			if(!uint_fits(*$3))
			{
				yyerror("Invalid constant");
				YYERROR;
			}
			switch(uint_get(*$3))
			{
			case 8:
				current_parser_state->abits = BITSIZE8;
				break;
			case 16:
				current_parser_state->abits = BITSIZE16;
				break;
			default:
				yyerror("Invalid constant");
				YYERROR;
				break;
			}
			if(!is_replacement)
				int_delete($3);
		}
	;

x_width
	: TOK_XREG '=' TOK_INTEGER
		{
			if(!uint_fits(*$3))
			{
				yyerror("Invalid constant");
				YYERROR;
			}
			switch(uint_get(*$3))
			{
			case 8:
				current_parser_state->xbits = BITSIZE8;
				break;
			case 16:
				current_parser_state->xbits = BITSIZE16;
				break;
			default:
				yyerror("Invalid constant");
				YYERROR;
				break;
			}
			if(!is_replacement)
				int_delete($3);
		}
	;

mnemonic
	: TOK_MNEM
		{
			instruction_clear(&current_instruction, current_parser_state);
			current_instruction.mnemonic = $1 & 0xFFF;
			current_instruction.bit = $1 >> 12;
			/*if(current_parser_state->cpu_type == CPU_65CE02)
			{
				switch(current_instruction.mnemonic)
				{
				case MNEM_JSR:
					current_instruction.mnemonic = MNEM_65CE02_JSR;
					break;
				case MNEM_LDA:
					current_instruction.mnemonic = MNEM_65CE02_LDA;
					break;
				case MNEM_STA:
					current_instruction.mnemonic = MNEM_65CE02_STA;
					break;
				}
			}*/
		}
	;

instruction
	: mnemonic '\n'
		{
			operand_set(&current_operand, OPD_NONE);
			instruction_stream_append(current_instruction_stream, &current_instruction);
			advance_line();
		}
	| mnemonic operand '\n'
		{
			instruction_stream_append(current_instruction_stream, &current_instruction);
			advance_line();
		}
	;

operand
	: '#' expression_noparen
		{
			operand_set(&current_operand, OPD_IMM);
			current_operand.parameter = $2;
		}
	| '#' sized_expression
		{
			current_operand.type = OPD_IMM;
		}
	| TOK_AREG
		{
			operand_set(&current_operand, OPD_REG_A);
		}
	| expression_noparen
		{
			operand_set(&current_operand, OPD_MEM);
			current_operand.parameter = $1;
		}
	| sized_expression
		{
		}
	| expression_noparen ',' TOK_XREG
		{
			operand_set(&current_operand, OPD_MEM_X);
			current_operand.parameter = $1;
		}
	| sized_expression ',' TOK_XREG
		{
			current_operand.type = OPD_MEM_X;
		}
	| expression_noparen ',' TOK_YREG
		{
			operand_set(&current_operand, OPD_MEM_Y);
			current_operand.parameter = $1;
		}
	| sized_expression ',' TOK_YREG
		{
			current_operand.type = OPD_MEM_Y;
		}
	| expression_noparen ',' TOK_SREG
		{
			if(current_parser_state->cpu_type != CPU_65C816)
			{
				yyerror("Invalid addressing mode");
				YYERROR;
			}
			operand_set(&current_operand, OPD_STK);
			current_operand.parameter = $1;
		}
	| '(' expression ')'
		{
			operand_set(&current_operand, OPD_IND);
			current_operand.parameter = $2;
		}
	| '(' sized_expression ')'
		{
			current_operand.type = OPD_IND;
		}
	| '(' address ',' TOK_XREG ')'
		{
			current_operand.type = OPD_IND_X;
		}
	| '(' expression ')' ',' TOK_YREG
		{
			operand_set(&current_operand, OPD_IND_Y);
			current_operand.parameter = $2;
		}
	| '(' sized_expression ')' ',' TOK_YREG
		{
			current_operand.type = OPD_IND_Y;
		}
	| '(' expression ')' ',' TOK_ZREG
		{
			if(current_parser_state->cpu_type != CPU_65CE02)
			{
				yyerror("Invalid addressing mode");
				YYERROR;
			}
			operand_set(&current_operand, OPD_IND_Z);
			current_operand.parameter = $2;
		}
	| '(' sized_expression ')' ',' TOK_ZREG
		{
			if(current_parser_state->cpu_type != CPU_65CE02)
			{
				yyerror("Invalid addressing mode");
				YYERROR;
			}
			current_operand.type = OPD_IND_Z;
		}
	| '[' address ']'
		{
			if(current_parser_state->cpu_type != CPU_65C816)
			{
				yyerror("Invalid addressing mode");
				YYERROR;
			}
			current_operand.type = OPD_LNG;
		}
	| '[' address ']' ',' TOK_YREG
		{
			if(current_parser_state->cpu_type != CPU_65C816)
			{
				yyerror("Invalid addressing mode");
				YYERROR;
			}
			current_operand.type = OPD_LNG_Y;
		}
	| '(' address ',' TOK_SREG ')' ',' TOK_YREG
		{
			if(current_parser_state->cpu_type != CPU_65CE02 && current_parser_state->cpu_type != CPU_65C816)
			{
				yyerror("Invalid addressing mode");
				YYERROR;
			}
			current_operand.type = OPD_STK_Y;
		}
	| expression_noparen ',' expression
		{
			if(current_parser_state->cpu_type == CPU_6502)
			{
				yyerror("Invalid addressing mode");
				YYERROR;
			}
			operand_set(&current_operand, OPD_PAIR);
			current_operand.parameters[0] = $1;
			current_operand.parameters[1] = $3;
		}
	;

address
	: expression
		{
			operand_set(&current_operand, OPD_MEM);
			current_operand.parameter = $1;
		}
	| sized_expression
	;

sized_expression
	: '<' expression
		{
			operand_set(&current_operand, OPD_MEM);
			current_operand.mode = MODE_LOW_BYTE;
			current_operand.parameter = $2;
		}
	| KWD_LOW expression
		{
			operand_set(&current_operand, OPD_MEM);
			current_operand.mode = MODE_LOW_BYTE;
			current_operand.parameter = $2;
		}
	| '>' expression
		{
			operand_set(&current_operand, OPD_MEM);
			current_operand.mode = MODE_HIGH_BYTE;
			current_operand.parameter = $2;
		}
	| KWD_HIGH expression
		{
			operand_set(&current_operand, OPD_MEM);
			current_operand.mode = MODE_HIGH_BYTE;
			current_operand.parameter = $2;
		}
	| KWD_ADDR expression
		{
			operand_set(&current_operand, OPD_MEM);
			current_operand.mode = MODE_WORD;
			current_operand.parameter = $2;
		}
	| '@' expression
		{
			operand_set(&current_operand, OPD_MEM);
			current_operand.mode = MODE_LONG;
			current_operand.parameter = $2;
		}
	| KWD_BANK expression
		{
			operand_set(&current_operand, OPD_MEM);
			current_operand.mode = MODE_BANK;
			current_operand.parameter = $2;
		}
	| TOK_DATA expression
		{
			operand_set(&current_operand, OPD_MEM);
			switch(_GET_DATA_SIZE($1))
			{
			case BITSIZE8:
				current_operand.mode = MODE_LOW_BYTE;
				break;
			case BITSIZE16:
				current_operand.mode = MODE_WORD;
				break;
			case BITSIZE24:
				current_operand.mode = MODE_LONG;
				break;
			default:
				yyerror("Invalid data width specification");
				YYERROR;
			}
			current_operand.parameter = $2;
		}
	;

%%

parser_state_t current_parser_state[1] =
{
	{
		.line_number = 1,
		.cpu_type = CPU_6502,
		.abits = BITSIZE8,
		.xbits = BITSIZE8,
		.stream =
		{
			.first_instruction = NULL,
			.last_instruction = &current_parser_state->stream.first_instruction,
		},
	}
};

void instruction_clear(instruction_t * ins, parser_state_t * state)
{
	memset(ins, 0, sizeof(instruction_t));
	ins->line_number = state->line_number;
	ins->cpu = state->cpu_type;
	ins->abits = state->abits;
	ins->xbits = state->xbits;
}

// TODO: operand types

void operand_set(operand_t * opd, operand_type_t type)
{
	memset(opd, 0, sizeof(operand_t));
	opd->type = type;
}

void operand_set_immediate(operand_t * opd, expression_t * value)
{
	memset(opd, 0, sizeof(operand_t));
	opd->parameter = value;
}

void operand_set_memory(operand_t * opd)
{
	operand_set(opd, OPD_MEM);
}


