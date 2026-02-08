
%include "../parser.y"

%{
#include "../../../src/dummy/isa.h"

parser_state_t current_parser_state[1];
extern instruction_t current_instruction;
#define current_operand (current_instruction.operand[current_instruction.operand_count])
#define current_instruction_stream (&current_parser_state->stream)
%}

%token INVALID // never generated

%%

%include "../parser.y"

instruction
	: INVALID
	;

%%

parser_state_t current_parser_state[1] =
{
	{
		.line_number = 1,
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
}

