
%include "../parser.y"

%{
#include "../../../src/i4/isa.h"

void operand_set_immediate(operand_t * opd, expression_t * value);

parser_state_t current_parser_state[1];
extern instruction_t current_instruction;
#define current_operand (current_instruction.operand[current_instruction.operand_count])
#define current_instruction_stream (&current_parser_state->stream)
%}

%token <i> TOK_ARCH
%token <i> TOK_MNEM

%%

%include "../parser.y"

directive
	: TOK_ARCH '\n'
		{
			current_parser_state->cpu_type = $1;
			advance_line();
		}
	;

mnemonic
	: TOK_MNEM
		{
			instruction_clear(&current_instruction, current_parser_state);
			current_instruction.mnemonic = $1;
		}
	;

instruction
	: mnemonic '\n'
		{
			instruction_stream_append(current_instruction_stream, &current_instruction);
			advance_line();
		}
	| mnemonic_operands '\n'
		{
			instruction_stream_append(current_instruction_stream, &current_instruction);
			advance_line();
		}
	;

mnemonic_operands
	: mnemonic operand
		{
			current_instruction.operand_count ++;
		}
	| mnemonic_operands ',' operand
		{
			current_instruction.operand_count ++;
		}
	;

operand
	: expression
		{
			operand_set_immediate(&current_operand, $1);
		}
	;

%%

parser_state_t current_parser_state[1] =
{
	{
		.line_number = 1,
		.cpu_type = CPU_4040,
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
}

void operand_set_immediate(operand_t * opd, expression_t * value)
{
	memset(opd, 0, sizeof(operand_t));
	opd->parameter = value;
}

