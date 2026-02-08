
%include "../parser.y"

%{
#if TARGET_X86
# include "../../../src/x86/isa.h"
#else
# include "../../../src/x80/isa.h"
#endif

void operand_set(operand_t * opd, operand_type_t type);
void operand_set_register(operand_t * opd, operand_type_t type, int number);
void operand_set_immediate(operand_t * opd, expression_t * value);
void operand_set_memory(operand_t * opd);
void operand_set_memory_type(operand_t * opd, operand_type_t type);

parser_state_t current_parser_state[1];
extern instruction_t current_instruction;
#define current_operand (current_instruction.operand[current_instruction.operand_count])
#define current_instruction_stream (&current_parser_state->stream)
%}

%token <i> TOK_ARCH TOK_SYNTAX
%token <i> TOK_I8080_MNEM
%token <i> TOK_Z80_MNEM
%token <i> TOK_R800_MNEM TOK_SHORT TOK_R800_BRANCH /* branches that can be prefixed with short */
%token <i> TOK_I8008_MNEM
%token <i> TOK_X80_REG
%token <i> TOK_X80_MEM TOK_X80_HL TOK_X80_HLD TOK_X80_HLI
%token <i> TOK_X80_IDX

%%

%include "../parser.y"

directive
	: TOK_ARCH '\n'
		{
			current_parser_state->cpu_type = $1;

			switch(current_parser_state->cpu_type)
			{
			case CPU_DP2200:
			case CPU_DP2200V2:
				current_parser_state->syntax = SYNTAX_DP2200;
				break;
			case CPU_8008:
				current_parser_state->syntax = SYNTAX_I8008;
				break;
			case CPU_8080:
			case CPU_8085:
				current_parser_state->syntax = SYNTAX_INTEL;
				break;
			case CPU_R800:
				current_parser_state->syntax = SYNTAX_ASCII;
				break;
			default:
				current_parser_state->syntax = SYNTAX_ZILOG;
				break;
			}

			setup_lexer(current_parser_state);
			advance_line();
		}
	| TOK_SYNTAX '\n'
		{
			current_parser_state->syntax = $1;

			setup_lexer(current_parser_state);
			advance_line();
		}
	;

instruction
	: i8008_instruction
		{
			instruction_stream_append(current_instruction_stream, &current_instruction);
		}
	| i8080_instruction
		{
			instruction_stream_append(current_instruction_stream, &current_instruction);
		}
	| z80_instruction
		{
			instruction_stream_append(current_instruction_stream, &current_instruction);
		}
	| r800_instruction
		{
			instruction_stream_append(current_instruction_stream, &current_instruction);
		}
	;

i8008_mnemonic
	: TOK_I8008_MNEM
		{
			instruction_clear(&current_instruction, current_parser_state);
			current_instruction.mnemonic = _GET_MNEM($1);
			current_instruction.condition = _GET_COND($1);
		}
	;

i8008_instruction
	: i8008_mnemonic '\n'
		{
			advance_line();
		}
	| i8008_mnemonic expression '\n'
		{
			operand_set_immediate(&current_operand, $2);
			current_instruction.operand_count ++;
			advance_line();
		}
	;

i8080_mnemonic
	: TOK_I8080_MNEM
		{
			instruction_clear(&current_instruction, current_parser_state);
			current_instruction.mnemonic = _GET_MNEM($1);
			switch(current_instruction.mnemonic)
			{
			case MNEM_I8080_C_CC:
			case MNEM_I8080_J_CC:
			case MNEM_I8080_R_CC:
				current_instruction.condition = _GET_COND($1);
				break;
			default:
				break;
			}
		}
	;

i8080_mnemonic_operands
	: i8080_mnemonic i8080_operand
		{
			current_instruction.operand_count ++;
		}
	| i8080_mnemonic_operands ',' i8080_operand
		{
			current_instruction.operand_count ++;
		}
	;

i8080_instruction
	: i8080_mnemonic '\n'
		{
			advance_line();
		}
	| i8080_mnemonic_operands '\n'
		{
			advance_line();
		}
	;

z80_mnemonic
	: TOK_Z80_MNEM
		{
			instruction_clear(&current_instruction, current_parser_state);
			current_instruction.mnemonic = $1;
		}
	;

z80_mnemonic_operands
	: z80_mnemonic z80_operand
		{
			current_instruction.operand_count ++;
		}
	| z80_mnemonic_operands ',' z80_operand
		{
			current_instruction.operand_count ++;
		}
	;

z80_instruction
	: z80_mnemonic '\n'
		{
			advance_line();
		}
	| z80_mnemonic_operands '\n'
		{
			advance_line();
		}
	;

r800_mnemonic
	: TOK_R800_MNEM
		{
			instruction_clear(&current_instruction, current_parser_state);
			current_instruction.mnemonic = _GET_MNEM($1);
			switch(current_instruction.mnemonic)
			{
			case MNEM_I8080_J_CC:
				current_instruction.condition = _GET_COND($1);
				break;
			default:
				break;
			}
		}
	| TOK_R800_BRANCH
		{
			instruction_clear(&current_instruction, current_parser_state);
			current_instruction.mnemonic = _GET_MNEM($1);
			switch(current_instruction.mnemonic)
			{
			case MNEM_I8080_J_CC:
				current_instruction.condition = _GET_COND($1);
				break;
			default:
				break;
			}
		}
	| TOK_SHORT TOK_R800_BRANCH
		{
			instruction_clear(&current_instruction, current_parser_state);
			switch(_GET_MNEM($2))
			{
			case MNEM_R800_BR:
				current_instruction.mnemonic = MNEM_R800_SHORT_BR;
				break;
			case MNEM_I8080_J_CC:
				current_instruction.mnemonic = MNEM_R800_SHORT_B_CC;
				current_instruction.condition = _GET_COND($2);
				break;
			default:
				yyerror("Invalid short branch\n");
				YYERROR;
				break;
			}
		}
	;

r800_mnemonic_operands
	: r800_mnemonic r800_operand
		{
			current_instruction.operand_count ++;
		}
	| r800_mnemonic_operands ',' r800_operand
		{
			current_instruction.operand_count ++;
		}
	;

r800_instruction
	: r800_mnemonic '\n'
		{
			advance_line();
		}
	| r800_mnemonic_operands '\n'
		{
			advance_line();
		}
	;

i8080_operand
	: TOK_X80_REG
		{
			operand_set_register(&current_operand, OPD_TOKEN, $1);
		}
	| TOK_X80_MEM
		{
			operand_set_register(&current_operand, OPD_TOKEN, $1);
		}
	| expression
		{
			operand_set_immediate(&current_operand, $1);
		}
	;

z80_operand
	: TOK_X80_REG
		{
			operand_set_register(&current_operand, OPD_TOKEN, $1);
		}
	| TOK_X80_MEM
		{
			operand_set_register(&current_operand, OPD_TOKEN, $1);
		}
	| TOK_X80_HL
		{
			operand_set_register(&current_operand, OPD_TOKEN, $1);
		}
	| TOK_X80_IDX
		{
			operand_set_register(&current_operand, OPD_TOKEN, $1);
		}
	| expression_noparen
		{
			operand_set_immediate(&current_operand, $1);
		}
	| '(' expression ')'
		{
			operand_set_memory(&current_operand);
			current_operand.parameter = $2;
		}
	| '(' TOK_X80_MEM ')'
		{
			operand_set_memory(&current_operand);
			current_operand.base = $2;
		}
	| '(' TOK_X80_HL ')'
		{
			operand_set_memory(&current_operand);
			current_operand.base = $2;
		}
	| '(' TOK_X80_IDX ')'
		{
			operand_set_memory(&current_operand);
			current_operand.base = $2;
		}
	| '(' TOK_X80_IDX addend ')'
		{
			operand_set_memory(&current_operand);
			current_operand.base = $2;
			current_operand.parameter = $3;
		}
	| '(' TOK_X80_HLD ')'
		{
			operand_set_memory_type(&current_operand, OPD_HLD);
		}
	| '(' TOK_X80_HLI ')'
		{
			operand_set_memory_type(&current_operand, OPD_HLI);
		}
	| '(' TOK_X80_HL '+' ')'
		{
			operand_set_memory_type(&current_operand, OPD_HLI);
		}
	| '(' TOK_X80_HL '-' ')'
		{
			operand_set_memory_type(&current_operand, OPD_HLD);
		}
	;

r800_operand
	: TOK_X80_REG
		{
			operand_set_register(&current_operand, OPD_TOKEN, $1);
		}
	| TOK_X80_MEM
		{
			operand_set_register(&current_operand, OPD_TOKEN, $1);
		}
	| TOK_X80_IDX
		{
			operand_set_register(&current_operand, OPD_TOKEN, $1);
		}
	| expression
		{
			operand_set_immediate(&current_operand, $1);
		}
	| '[' expression ']'
		{
			operand_set_memory(&current_operand);
			current_operand.parameter = $2;
		}
	| '[' TOK_X80_MEM ']'
		{
			operand_set_memory(&current_operand);
			current_operand.base = $2;
		}
	| '[' TOK_X80_IDX ']'
		{
			operand_set_memory(&current_operand);
			current_operand.base = $2;
		}
	| '[' TOK_X80_IDX addend ']'
		{
			operand_set_memory(&current_operand);
			current_operand.base = $2;
			current_operand.parameter = $3;
		}
	| '[' TOK_X80_MEM '+' '+' ']'
		{
			switch($2)
			{
			case X80_HL:
				operand_set_memory_type(&current_operand, OPD_HLI);
				break;
			case X80_DE:
				operand_set_memory_type(&current_operand, OPD_DEI);
				break;
			default:
				yyerror("Invalid register");
				YYERROR;
			}
		}
	| '[' TOK_X80_MEM '-' '-' ']'
		{
			switch($2)
			{
			case X80_HL:
				operand_set_memory_type(&current_operand, OPD_HLD);
				break;
			case X80_DE:
				operand_set_memory_type(&current_operand, OPD_DED);
				break;
			default:
				yyerror("Invalid register");
				YYERROR;
			}
		}
	;

%%

parser_state_t current_parser_state[1] =
{
	{
		.line_number = 1,
		.cpu_type = CPU_8080,
		.syntax = SYNTAX_INTEL,
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
	ins->condition = COND_NONE;
}

void operand_set(operand_t * opd, operand_type_t type)
{
	memset(opd, 0, sizeof(operand_t));
	opd->type = type;
}

void operand_set_register(operand_t * opd, operand_type_t type, int number)
{
	operand_set(opd, type);
	opd->base = number;
}

void operand_set_immediate(operand_t * opd, expression_t * value)
{
	memset(opd, 0, sizeof(operand_t));
	opd->parameter = value;
}

void operand_set_memory(operand_t * opd)
{
	operand_set_memory_type(opd, OPD_MEM);
}

void operand_set_memory_type(operand_t * opd, operand_type_t type)
{
	operand_set(opd, type);
	opd->base = REG_NONE;
}

