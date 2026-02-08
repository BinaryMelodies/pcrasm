
%include "../parser.y"

%{
#include "../../../src/680x/isa.h"

static const int reg_stack_flag[];

void operand_set(operand_t * opd, operand_type_t type);
void operand_set_immediate(operand_t * opd, expression_t * value);
void operand_set_pcrelative(operand_t * opd, expression_t * value);
void operand_set_address(operand_t * opd, operand_type_t type, int idx, expression_t * parameter);

parser_state_t current_parser_state[1];
extern instruction_t current_instruction;
#define current_operand (current_instruction.operand[current_instruction.operand_count])
#define current_instruction_stream (&current_parser_state->stream)
%}

%token <i> TOK_AREG TOK_BREG TOK_DREG
%token <i> TOK_IXREG /* u s x y */
%token <i> TOK_ARCH
%token <i> TOK_6800_MNEM0 TOK_6800_MNEM1 TOK_6800_MNEM1T TOK_6800_MNEM1R
%token <i> TOK_6809_MNEM0 TOK_6809_MNEM1 TOK_6809_MNEM1T TOK_6809_MNEM1I TOK_6809_MNEM1R TOK_6809_MNEM1X TOK_6809_MNEM2 TOK_6809_MNEML
%token <i> TOK_PCREL /* pcr */
%token <i> TOK_PCREG /* pc */
%token <i> TOK_REG /* dp cc */

%token KWD_LOW
%token KWD_HIGH
%token KWD_ADDR
%token KWD_PAGE // not currently used

%type <i> register register_list

%%

%include "../parser.y"

directive
	: TOK_ARCH '\n'
		{
			current_parser_state->cpu_type = $1;

			setup_lexer(current_parser_state);
			advance_line();
		}
	;

instruction
	: m6800_instruction '\n'
		{
			instruction_stream_append(current_instruction_stream, &current_instruction);
			advance_line();
		}
	| m6809_instruction '\n'
		{
			instruction_stream_append(current_instruction_stream, &current_instruction);
			advance_line();
		}
	;

m6800_instruction
	: TOK_6800_MNEM0
		{
			instruction_clear(&current_instruction, current_parser_state);
			current_instruction.mnemonic = $1;
			operand_set(&current_operand, OPD_NONE);
		}
	| TOK_6800_MNEM1
		{
			instruction_clear(&current_instruction, current_parser_state);
			current_instruction.mnemonic = $1;
		}
		m6800_operand
	| TOK_6800_MNEM1
		{
			instruction_clear(&current_instruction, current_parser_state);
			current_instruction.mnemonic = $1;
		}
		'#' expression
		{
			operand_set_immediate(&current_operand, $4);
		}
	| TOK_6800_MNEM1R
		{
			instruction_clear(&current_instruction, current_parser_state);
			current_instruction.mnemonic = $1;
		}
		expression
		{
			operand_set_pcrelative(&current_operand, $3);
		}
	| TOK_6800_MNEM1T
		{
			instruction_clear(&current_instruction, current_parser_state);
			current_instruction.mnemonic = $1;
		}
		m6800_operand
	;

m6809_instruction
	: TOK_6809_MNEM0
		{
			instruction_clear(&current_instruction, current_parser_state);
			current_instruction.mnemonic = $1;
			operand_set(&current_operand, OPD_NONE);
		}
	| TOK_6809_MNEM1
		{
			instruction_clear(&current_instruction, current_parser_state);
			current_instruction.mnemonic = $1;
		}
		operand
	| TOK_6809_MNEM1
		{
			instruction_clear(&current_instruction, current_parser_state);
			current_instruction.mnemonic = $1;
		}
		'#' expression
		{
			operand_set_address(&current_operand, OPD_IMMB, IGNORE, $4);
		}
	| TOK_6809_MNEM1R
		{
			instruction_clear(&current_instruction, current_parser_state);
			current_instruction.mnemonic = $1;
		}
		expression
		{
			operand_set_address(&current_operand, OPD_RELB, IGNORE, $3);
		}
	| TOK_6809_MNEM1T
		{
			instruction_clear(&current_instruction, current_parser_state);
			current_instruction.mnemonic = $1;
		}
		operand
	| TOK_6809_MNEM1I
		{
			instruction_clear(&current_instruction, current_parser_state);
			current_instruction.mnemonic = $1;
		}
		'#' expression
		{
			operand_set_address(&current_operand, OPD_IMMB, IGNORE, $4);
		}
	| TOK_6809_MNEM1X
		{
			instruction_clear(&current_instruction, current_parser_state);
			current_instruction.mnemonic = $1;
		}
		indexed_operand
	| TOK_6809_MNEM2
		{
			instruction_clear(&current_instruction, current_parser_state);
			current_instruction.mnemonic = $1;
		}
		register ',' register
		{
			operand_set_address(&current_operand, OPD_REG2, ($3 << 4) | $5, NULL);
		}
	| TOK_6809_MNEML
		{
			instruction_clear(&current_instruction, current_parser_state);
			current_instruction.mnemonic = $1;
		}
		register_list
		{
			operand_set_address(&current_operand, OPD_REGLIST, $3, NULL);
		}
	;

m6800_operand
	: expression
		{
			operand_set_address(&current_operand, OPD_DIR, IGNORE, $1);
		}
	| TOK_IXREG
		{
			operand_set_address(&current_operand, OPD_IND, IGNORE, NULL);
		}
	| ',' TOK_IXREG
		{
			operand_set_address(&current_operand, OPD_IND, IGNORE, NULL);
		}
	| expression ',' TOK_IXREG
		{
			operand_set_address(&current_operand, OPD_IND, IGNORE, $1);
		}
	;

operand
	: '<' expression
		{
			operand_set_address(&current_operand, OPD_DIR, IGNORE, $2);
		}
	| KWD_LOW expression
		{
			operand_set_address(&current_operand, OPD_DIR, IGNORE, $2);
		}
	// TODO: KWD_HIGH, KWD_PAGE
	| expression
		{
			operand_set_address(&current_operand, OPD_EXT, IGNORE, $1);
		}
	| KWD_ADDR expression
		{
			operand_set_address(&current_operand, OPD_EXT, IGNORE, $2);
		}
	| address1
		{
			current_operand.type = OPD_IND;
		}
	| '[' address2 ']'
		{
			current_operand.type = OPD_IND;
			current_operand.index |= 0x10;
		}
	;

indexed_operand
	: address1
		{
			current_operand.type = OPD_IND;
		}
	| '[' address2 ']'
		{
			current_operand.type = OPD_IND;
			current_operand.index |= 0x10;
		}
	;

address1
	: address_a
	| address_b
	;

address2
	: expression
		{
			operand_set_address(&current_operand, IGNORE, 0x9F, $1);
		}
	| address_a
	;

address_b
	: ',' TOK_IXREG '+'
		{
			operand_set_address(&current_operand, IGNORE, MAKE_IDX($2, 0x00), NULL);
		}
	| ',' '-' TOK_IXREG
		{
			operand_set_address(&current_operand, IGNORE, MAKE_IDX($3, 0x02), NULL);
		}
	| TOK_IXREG '+'
		{
			operand_set_address(&current_operand, IGNORE, MAKE_IDX($1, 0x00), NULL);
		}
	| '-' TOK_IXREG
		{
			operand_set_address(&current_operand, IGNORE, MAKE_IDX($2, 0x02), NULL);
		}
	;

address_a
	: TOK_IXREG
		{
			operand_set_address(&current_operand, IGNORE, MAKE_IDX($1, 0x04), NULL);
		}
	| ',' TOK_IXREG
		{
			operand_set_address(&current_operand, IGNORE, MAKE_IDX($2, 0x04), NULL);
		}
	| expression ',' TOK_IXREG
		{
			operand_set_address(&current_operand, IGNORE, MAKE_IDX($3, 0x04), $1);
		}
	| TOK_AREG ',' TOK_IXREG
		{
			operand_set_address(&current_operand, IGNORE, MAKE_IDX($3, 0x06), NULL);
		}
	| TOK_BREG ',' TOK_IXREG
		{
			operand_set_address(&current_operand, IGNORE, MAKE_IDX($3, 0x05), NULL);
		}
	| TOK_DREG ',' TOK_IXREG
		{
			operand_set_address(&current_operand, IGNORE, MAKE_IDX($3, 0x0B), NULL);
		}
	| ',' TOK_IXREG '+' '+'
		{
			operand_set_address(&current_operand, IGNORE, MAKE_IDX($2, 0x01), NULL);
		}
	| ',' '-' '-' TOK_IXREG
		{
			operand_set_address(&current_operand, IGNORE, MAKE_IDX($4, 0x03), NULL);
		}
	| TOK_IXREG '+' '+'
		{
			operand_set_address(&current_operand, IGNORE, MAKE_IDX($1, 0x01), NULL);
		}
	| '-' '-' TOK_IXREG
		{
			operand_set_address(&current_operand, IGNORE, MAKE_IDX($3, 0x03), NULL);
		}
	| expression ',' TOK_PCREL
		{
			operand_set_address(&current_operand, IGNORE, MAKE_IDX($3, 0x0C), $1);
		}
	;

register
	: TOK_AREG
	| TOK_BREG
	| TOK_DREG
	| TOK_IXREG
	| TOK_PCREG
	| TOK_REG
	;

register_list
	: register
		{
			$$ = reg_stack_flag[$1];
		}
	| register_list ',' register
		{
			$$ = $1 | reg_stack_flag[$3];
		}
	;

%%

parser_state_t current_parser_state[1] =
{
	{
		.line_number = 1,
		.cpu_type = CPU_6800,
		.stream =
		{
			.first_instruction = NULL,
			.last_instruction = &current_parser_state->stream.first_instruction,
		},
	}
};

static const int reg_stack_flag[] =
{
	[REG_D] = 0x06,
	[REG_X] = 0x10,
	[REG_Y] = 0x20,
	[REG_U] = 0x40,
	[REG_S] = 0x40,
	[REG_PC] = 0x80,
	[REG_A] = 0x02,
	[REG_B] = 0x04,
	[REG_CC] = 0x01,
	[REG_DP] = 0x08,
};

void instruction_clear(instruction_t * ins, parser_state_t * state)
{
	memset(ins, 0, sizeof(instruction_t));
	ins->line_number = state->line_number;
	ins->cpu = state->cpu_type;
}

void operand_set(operand_t * opd, operand_type_t type)
{
	memset(opd, 0, sizeof(operand_t));
	opd->type = type;
}

void operand_set_immediate(operand_t * opd, expression_t * value)
{
	operand_set(opd, OPD_IMMB);
	opd->parameter = value;
}

void operand_set_pcrelative(operand_t * opd, expression_t * value)
{
	operand_set(opd, OPD_RELB);
	opd->parameter = value;
}

void operand_set_address(operand_t * opd, operand_type_t type, int index, expression_t * parameter)
{
	operand_set(opd, type);
	opd->type = type;
	opd->index = index;
	opd->parameter = parameter;
}

