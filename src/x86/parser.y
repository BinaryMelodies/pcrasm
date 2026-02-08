
%include "../parser.y"

%{
#include "../../../src/x86/isa.h"

bool validate_scale_strict(bitsize_t address_size, integer_value_t scale);
bool validate_scale_permissive(bitsize_t address_size, integer_value_t scale);

parser_state_t current_parser_state[1];
extern instruction_t current_instruction;
#define current_operand (current_instruction.operand[current_instruction.operand_count])
#define current_instruction_stream (&current_parser_state->stream)

void operand_set(operand_t * opd, operand_type_t type);
void operand_set_register(operand_t * opd, operand_type_t type, int number);
void operand_set_immediate(operand_t * opd, expression_t * value);
void operand_set_far_address(operand_t * opd, expression_t * segment, expression_t * offset);
void operand_set_memory(operand_t * opd);
%}

%token <i> TOK_ARCH
%token KWD_BITS
%token KWD_FAR
%token KWD_REL
%token KWD_SEG
%token KWD_WRT

%token TOK_286_UMOV_PREFIX
%token <i> TOK_FPU
%token <i> TOK_ASIZE
%token <i> TOK_BITS
%token <i> TOK_CREG
%token TOK_CYSYMBOL
%token TOK_DIRSYMBOL
%token <i> TOK_DREG
%token <i> TOK_GPR
%token <i> TOK_IPREG
%token TOK_IRAM
%token TOK_LOCK
%token <i> TOK_MNEMONIC
%token <i> TOK_MMREG
%token <i> TOK_OSIZE
%token TOK_PSWREG
%token <i> TOK_REP
%token TOK_RSYMBOL
%token <i> TOK_SEG
%token <i> TOK_STREG
%token <i> TOK_TREG
%token <i> TOK_XMMREG
%token <i> TOK_YMMREG
%token <i> TOK_ZMMREG
%token TOK_DW // required for NEC

%type <i> segment_prefix

%token <i> TOK_I8080_MNEM
%token <i> TOK_Z80_MNEM
%token <i> TOK_X80_REG
%token <i> TOK_X80_MEM
%token <i> TOK_X80_IDX

%token <i> TOK_I8089_MNEM
%token <i> TOK_I8089_VALREG
%token <i> TOK_I8089_IXREG
%token <i> TOK_I8089_PTRREG
%token <i> TOK_I8089_PPREG

%token TOK_BRACKET_DOT "]."

%%

%include "../parser.y"

data
	: TOK_DW expression
		{
			instruction_make_data(&current_instruction, current_parser_state, _DATA_LE(BITSIZE16), $2);
			instruction_stream_append(current_instruction_stream, &current_instruction);
			$$ = _DATA_LE(BITSIZE16);
		}
	;

directive
	: TOK_BITS '\n'
		{
			switch($1)
			{
			case BITSIZE16:
				current_parser_state->bit_size = $1;
				break;
			case BITSIZE32:
				if(current_parser_state->cpu_type < CPU_386)
				{
					yyerror("Current architecture does not support 32-bit");
					YYERROR;
				}
				else
				{
					current_parser_state->bit_size = $1;
				}
				break;
			case BITSIZE64:
				if(current_parser_state->cpu_type < CPU_X64)
				{
					yyerror("Current architecture does not support 64-bit");
					YYERROR;
				}
				else
				{
					current_parser_state->bit_size = $1;
				}
				break;
			default:
				assert(false);
			}
			setup_lexer(current_parser_state);
			advance_line();
		}
	| KWD_BITS TOK_INTEGER '\n'
		{
			if(!uint_fits(INTVAL($2)))
			{
				yyerror("Invalid instruction set bit length");
				YYERROR;
			}
			switch(uint_get(INTVAL($2)))
			{
			case 8:
				switch(current_parser_state->cpu_type)
				{
				case CPU_V30:
					current_parser_state->x80_cpu_type = CPU_8080;
					break;
				case CPU_9002:
					current_parser_state->x80_cpu_type = CPU_Z80;
					break;
				default:
					yyerror("Current architecture does not support 8080/Z80 emulation");
					YYERROR;
				}
				current_parser_state->bit_size = BITSIZE8;
				break;
			case 16:
				current_parser_state->bit_size = BITSIZE16;
				break;
			case 32:
				if(current_parser_state->cpu_type < CPU_386)
				{
					yyerror("Current architecture does not support 32-bit");
					YYERROR;
				}
				else
				{
					current_parser_state->bit_size = BITSIZE32;
				}
				break;
			case 64:
				if(current_parser_state->cpu_type < 64)
				{
					yyerror("Current architecture does not support 64-bit");
					YYERROR;
				}
				else
				{
					current_parser_state->bit_size = BITSIZE64;
				}
				break;
			default:
				yyerror("Invalid instruction set bit length");
				YYERROR;
				break;
			}
			setup_lexer(current_parser_state);
			advance_line();
		}
	| TOK_ARCH '\n'
		{
			if($1 == CPU_8089)
			{
				current_parser_state->cpu_type = $1;
				current_parser_state->bit_size = BITSIZE16;
			}
			else if($1 == CPU_8080 || $1 == CPU_Z80)
			{
				current_parser_state->x80_cpu_type = $1;
				current_parser_state->bit_size = BITSIZE8;
			}
			else
			{
				current_parser_state->cpu_type = $1;

				if(current_parser_state->cpu_type < CPU_386)
					current_parser_state->bit_size = BITSIZE16;
				else if(current_parser_state->cpu_type != CPU_X64 && BITSIN(current_parser_state->bit_size) == BITSIZE64)
					current_parser_state->bit_size = BITSIZE32;
				else if(current_parser_state->cpu_type != CPU_V30 && current_parser_state->cpu_type != CPU_9002 && BITSIN(current_parser_state->bit_size) == BITSIZE8)
					current_parser_state->bit_size = BITSIZE16;

				if(current_parser_state->cpu_type < CPU_286)
					current_parser_state->fpu_type = FPU_8087;
				else if(current_parser_state->cpu_type < CPU_386)
					current_parser_state->fpu_type = FPU_287;
				else
					current_parser_state->fpu_type = FPU_387;

				if(current_parser_state->cpu_type == CPU_9002)
					current_parser_state->x80_cpu_type = CPU_Z80;
				else
					current_parser_state->x80_cpu_type = CPU_8080;
			}

			setup_lexer(current_parser_state);
			advance_line();
		}
	| TOK_FPU '\n'
		{
			current_parser_state->fpu_type = $1;
			advance_line();
		}
	;

instruction
	: x86_instruction
		{
			instruction_stream_append(current_instruction_stream, &current_instruction);
//printf("%d\n", ((instruction_t *)((char *)current_instruction_stream->last_instruction - offsetof(instruction_t, next_instruction)))->operation_size);
		}
	;

segment_prefix
	: TOK_SEG
	| TOK_SEG ':'
	;

iram_prefix
	: TOK_IRAM
	| TOK_IRAM ':'
	;

x86_instruction
	: segment_prefix x86_instruction
		{
			if($1 == REG_DS3)
			{
				if(current_instruction.target_segment_prefix != REG_NOSEG)
				{
					yyerror("Duplicate segment prefix");
					YYERROR;
				}
				current_instruction.target_segment_prefix = $1;
				current_instruction.segment_prefix = $1;
			}
			else
			{
				if(current_instruction.source_segment_prefix != REG_NOSEG)
				{
					yyerror("Duplicate segment prefix");
					YYERROR;
				}
				current_instruction.target_segment_prefix = $1;
				current_instruction.segment_prefix = $1;
			}
		}
	| iram_prefix x86_instruction
		{
			if(current_instruction.target_segment_prefix != REG_NOSEG)
			{
				yyerror("Duplicate segment prefix");
				YYERROR;
			}
			current_instruction.target_segment_prefix = REG_IRAM;
			current_instruction.segment_prefix = REG_IRAM;
		}
	| TOK_REP x86_instruction
		{
			if(current_instruction.repeat_prefix != PREF_NOREP)
			{
				yyerror("Duplicate repeat prefix");
				YYERROR;
			}
			current_instruction.repeat_prefix = $1;
		}
	| TOK_LOCK x86_instruction
		{
			if(current_instruction.lock_prefix)
			{
				yyerror("Duplicate lock prefix");
				YYERROR;
			}
			current_instruction.lock_prefix = true;
		}
	| TOK_OSIZE x86_instruction
		{
			if(current_parser_state->cpu_type < CPU_386)
			{
				yyerror("Operation size prefix not permitted for this CPU type");
				YYERROR;
			}
			if($1 == BITSIZE64 && BITSIN(current_parser_state->bit_size) != BITSIZE64)
			{
				yyerror("Operation size prefix not permitted for this CPU mode");
				YYERROR;
			}
			if(current_instruction.operation_size != 0)
			{
				yyerror("Duplicate operation size prefix");
				YYERROR;
			}
			current_instruction.operation_size = $1;
		}
	| TOK_ASIZE x86_instruction
		{
			if(current_parser_state->cpu_type < CPU_386)
			{
				yyerror("Operation size prefix not permitted for this CPU type");
				YYERROR;
			}
			if($1 == BITSIZE64 && BITSIN(current_parser_state->bit_size) != BITSIZE64)
			{
				yyerror("Operation size prefix not permitted for this CPU mode");
				YYERROR;
			}
			if(current_instruction.address_size != 0)
			{
				yyerror("Duplicate address size prefix");
				YYERROR;
			}
			current_instruction.address_size = $1;
		}
	| TOK_286_UMOV_PREFIX x86_instruction
		{
			if(current_instruction.umov_prefix)
			{
				yyerror("Duplicate 286 user mode prefix");
				YYERROR;
			}
			current_instruction.umov_prefix = true;
		}
	| mnemonic '\n'
		{
			advance_line();
		}
	| mnemonic_operands '\n'
		{
			advance_line();
		}
	;

mnemonic
	: TOK_MNEMONIC
		{
			instruction_clear(&current_instruction, current_parser_state);
			current_instruction.isa = ISA_X86;
			current_instruction.mnemonic = _GET_MNEM($1);
			switch(current_instruction.mnemonic)
			{
			case MNEM_CMOV_CC:
			case MNEM_FCMOV_CC:
			case MNEM_J_CC:
			case MNEM_SET_CC:
				current_instruction.condition = _GET_COND($1);
				break;
			default:
				break;
			}
		}
	;

mnemonic_operands
	: mnemonic operand
		{
			current_instruction.operand_count ++;
		}
	| mnemonic_operands
		{
			if(current_instruction.operand_count >= MAX_OPD_COUNT)
			{
				yyerror("Too many operands to instruction");
				YYERROR;
			}
		}
		',' operand
		{
			current_instruction.operand_count ++;
		}
	;

operand
	: TOK_GPR
		{
			operand_set_register(&current_operand, _OPD_TYPE($1), _OPD_VALUE($1));
		}
	| TOK_DW
		{
			operand_set_register(&current_operand, OPD_GPRW, REG_DX);
		}
	| TOK_SEG
		{
			operand_set_register(&current_operand, OPD_SEG, $1);
		}
	| TOK_STREG
		{
			operand_set_register(&current_operand, OPD_STREG, $1);
		}
	| TOK_CREG
		{
			operand_set_register(&current_operand, OPD_CREG, $1);
		}
	| TOK_DREG
		{
			operand_set_register(&current_operand, OPD_DREG, $1);
		}
	| TOK_TREG
		{
			operand_set_register(&current_operand, OPD_TREG, $1);
		}
	| TOK_MMREG
		{
			operand_set_register(&current_operand, OPD_MMREG, $1);
		}
	| TOK_XMMREG
		{
			operand_set_register(&current_operand, OPD_XMMREG, $1);
		}
	| TOK_YMMREG
		{
			operand_set_register(&current_operand, OPD_YMMREG, $1);
		}
	| TOK_ZMMREG
		{
			operand_set_register(&current_operand, OPD_ZMMREG, $1);
		}
	| expression
		{
			operand_set_immediate(&current_operand, $1);
		}
	| TOK_DATA expression
		{
			operand_set_immediate(&current_operand, $2);
			current_operand.size = _GET_DATA_SIZE($1);
		}
	| address_operand
	| TOK_DATA address_operand
		{
			current_operand.size = _GET_DATA_SIZE($1);
		}
	| KWD_FAR address_operand
		{
			current_operand.type = OPD_FARMEM;
		}
	| KWD_FAR TOK_DATA address_operand
		{
			current_operand.type = OPD_FARMEM;
			current_operand.size = _GET_DATA_SIZE($2);
		}
	| expression ':' expression
		{
			operand_set_far_address(&current_operand, $1, $3);
		}
	| expression ':' TOK_DATA expression
		{
			operand_set_far_address(&current_operand, $1, $4);
			current_operand.size = _GET_DATA_SIZE($3);
		}
	| TOK_PSWREG // nec
		{
			operand_set(&current_operand, OPD_PSW);
		}
	| TOK_RSYMBOL // nec
		{
			operand_set(&current_operand, OPD_RREG);
		}
	| TOK_CYSYMBOL // nec
		{
			operand_set(&current_operand, OPD_CY);
		}
	| TOK_DIRSYMBOL // nec
		{
			operand_set(&current_operand, OPD_DIR);
		}
	;

address_operand
	: '['
		{
			operand_set_memory(&current_operand);
		}
		address ']'
	| TOK_SEG ':' '['
		{
			operand_set_memory(&current_operand);
		}
		address_offset ']'
		{
			current_operand.segment = $1;
		}
	| TOK_IRAM ':' '['
		{
			operand_set_memory(&current_operand);
		}
		address_offset ']'
		{
			current_operand.segment = REG_IRAM;
		}
	;

address
	: address_offset
	| TOK_SEG ':' address_offset
		{
			current_operand.segment = $1;
		}
	| TOK_IRAM ':' address_offset
		{
			current_operand.segment = REG_IRAM;
		}
	;

address_offset
	: expression
		{
			current_operand.address_size = current_parser_state->bit_size;
			current_operand.parameter = $1;
		}
	| TOK_DATA expression
		{
			current_operand.address_size = _GET_DATA_SIZE($1);
			if(BITSIN(current_parser_state->bit_size) != BITSIZE64
				? BITSIN(current_operand.address_size) == BITSIZE64
				: BITSIN(current_operand.address_size) == BITSIZE16)
			{
				yyerror("Invalid address size\n");
				YYERROR;
			}
			current_operand.parameter = $2;
		}
	| TOK_GPR
		{
			current_operand.address_size = gprsize(_OPD_TYPE($1));
			current_operand.base = _OPD_VALUE($1);
		}
	| TOK_GPR addend
		{
			current_operand.address_size = gprsize(_OPD_TYPE($1));
			current_operand.base = _OPD_VALUE($1);
			current_operand.parameter = $2;
		}
	| TOK_GPR '*' TOK_INTEGER
		{
			current_operand.address_size = gprsize(_OPD_TYPE($1));
			if(!validate_scale_permissive(current_operand.address_size, $3))
			{
				yyerror("Invalid scale in memory address");
				YYERROR;
			}
			current_operand.index = _OPD_VALUE($1);
			current_operand.scale = uint_get(INTVAL($3));
			if(current_operand.scale != 1 && (current_operand.scale & 1) != 0)
			{
				current_operand.base = _OPD_VALUE($1);
				current_operand.scale --;
			}
			if(!is_replacement)
				int_delete($3);
		}
	| TOK_GPR '*' TOK_INTEGER addend
		{
			current_operand.address_size = gprsize(_OPD_TYPE($1));
			if(!validate_scale_permissive(current_operand.address_size, $3))
			{
				yyerror("Invalid scale in memory address");
				YYERROR;
			}
			current_operand.index = _OPD_VALUE($1);
			current_operand.scale = uint_get(INTVAL($3));
			if(current_operand.scale != 1 && (current_operand.scale & 1) != 0)
			{
				current_operand.base = _OPD_VALUE($1);
				current_operand.scale --;
			}
			current_operand.parameter = $4;
			if(!is_replacement)
				int_delete($3);
		}
	| TOK_GPR '+' TOK_GPR
		{
			current_operand.address_size = gprsize(_OPD_TYPE($1));
			if(current_operand.address_size != gprsize(_OPD_TYPE($3)))
			{
				yyerror("Base/index register size mismatch");
				YYERROR;
			}
			current_operand.base = _OPD_VALUE($1);
			current_operand.index = _OPD_VALUE($3);
		}
	| TOK_GPR '+' TOK_GPR addend
		{
			current_operand.address_size = gprsize(_OPD_TYPE($1));
			if(current_operand.address_size != gprsize(_OPD_TYPE($3)))
			{
				yyerror("Base/index register size mismatch");
				YYERROR;
			}
			current_operand.base = _OPD_VALUE($1);
			current_operand.index = _OPD_VALUE($3);
			current_operand.parameter = $4;
		}
	| TOK_GPR '+' TOK_GPR '*' TOK_INTEGER
		{
			current_operand.address_size = gprsize(_OPD_TYPE($1));
			if(current_operand.address_size != gprsize(_OPD_TYPE($3)))
			{
				yyerror("Base/index register size mismatch");
				YYERROR;
			}
			if(!validate_scale_strict(current_operand.address_size, $5))
			{
				yyerror("Invalid scale in memory address");
				YYERROR;
			}
			current_operand.base = _OPD_VALUE($1);
			current_operand.index = _OPD_VALUE($3);
			current_operand.scale = uint_get(INTVAL($5));
			if(!is_replacement)
				int_delete($5);
		}
	| TOK_GPR '+' TOK_GPR '*' TOK_INTEGER addend
		{
			current_operand.address_size = gprsize(_OPD_TYPE($1));
			if(current_operand.address_size != gprsize(_OPD_TYPE($3)))
			{
				yyerror("Base/index register size mismatch");
				YYERROR;
			}
			if(!validate_scale_strict(current_operand.address_size, $5))
			{
				yyerror("Invalid scale in memory address");
				YYERROR;
			}
			current_operand.base = _OPD_VALUE($1);
			current_operand.index = _OPD_VALUE($3);
			current_operand.scale = uint_get(INTVAL($5));
			if(!is_replacement)
				int_delete($5);
			current_operand.parameter = $6;
		}
	| TOK_IPREG
		{
			current_operand.address_size = gprsize(_OPD_TYPE($1));
			current_operand.base = REG_IP;
		}
	| TOK_IPREG addend
		{
			current_operand.address_size = gprsize(_OPD_TYPE($1));
			current_operand.base = REG_IP;
			current_operand.parameter = $2;
		}
	| KWD_REL expression
		{
			current_operand.address_size = current_parser_state->bit_size;
			current_operand.base = REG_IPREL;
			current_operand.parameter = $2;
		}
	| TOK_DATA KWD_REL expression
		{
			current_operand.address_size = _GET_DATA_SIZE($1);
			current_operand.base = REG_IPREL;
			current_operand.parameter = $3;
		}
	| KWD_REL TOK_DATA expression
		{
			current_operand.address_size = _GET_DATA_SIZE($2);
			current_operand.base = REG_IPREL;
			current_operand.parameter = $3;
		}
	;

instruction
	: i8080_instruction
		{
			instruction_stream_append(current_instruction_stream, &current_instruction);
		}
	| z80_instruction
		{
			instruction_stream_append(current_instruction_stream, &current_instruction);
		}
	;

i8080_mnemonic
	: TOK_I8080_MNEM
		{
			instruction_clear(&current_instruction, current_parser_state);
			current_instruction.isa = ISA_X80;
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
			current_instruction.isa = ISA_X80;
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
	;

instruction
	: i8089_instruction
		{
			instruction_stream_append(current_instruction_stream, &current_instruction);
		}
	;

i8089_mnemonic
	: TOK_I8089_MNEM
		{
			instruction_clear(&current_instruction, current_parser_state);
			current_instruction.isa = ISA_8089;
			current_instruction.mnemonic = $1;
		}
	;

i8089_mnemonic_operand
	: i8089_mnemonic i8089_operand
		{
			current_instruction.operand_count ++;
		}
	| i8089_mnemonic_operand ',' i8089_operand
		{
			current_instruction.operand_count ++;
		}
	;

i8089_instruction
	: i8089_mnemonic '\n'
		{
			advance_line();
		}
	| i8089_mnemonic_operand '\n'
		{
			advance_line();
		}
	;

i8089_operand
	: TOK_I8089_VALREG
		{
			operand_set_register(&current_operand, OPD_GPRW, $1);
		}
	| TOK_I8089_IXREG
		{
			operand_set_register(&current_operand, OPD_GPRW, $1);
		}
	| TOK_I8089_PTRREG
		{
			operand_set_register(&current_operand, OPD_GPRW, $1);
		}
	| '[' i8089_base_address ']'
	| '[' i8089_base_address ']' '.' expression
		{
			current_operand.x89_mode = X89_OFFSET;
			current_operand.parameter = $5;
		}
	| '[' i8089_base_address "]." expression
		{
			current_operand.x89_mode = X89_OFFSET;
			current_operand.parameter = $4;
		}
	| '[' i8089_base_address '+' expression ']'
		{
			current_operand.x89_mode = X89_OFFSET;
			current_operand.parameter = $4;
		}
	| '[' i8089_base_address '+' TOK_I8089_IXREG ']'
		{
			current_operand.x89_mode = X89_INDEX;
		}
	| '[' i8089_base_address '+' TOK_I8089_IXREG '+' ']'
		{
			current_operand.x89_mode = X89_INCREMENT;
		}
	| expression
		{
			operand_set_immediate(&current_operand, $1);
		}
	| expression ':' expression
		{
			operand_set_far_address(&current_operand, $1, $3);
		}
	;

i8089_base_address
	: TOK_I8089_PTRREG
		{
			operand_set_memory(&current_operand);
			current_operand.base = $1;
		}
	| TOK_I8089_PPREG
		{
			operand_set_memory(&current_operand);
			current_operand.base = $1;
		}
	;

expression
	: KWD_SEG TOK_IDENTIFIER
		{
			$$ = expression_segment_of($2);
		}
	| TOK_IDENTIFIER KWD_WRT TOK_IDENTIFIER
		{
			$$ = expression_relative_to($1, $3);
		}
	;

%%

bool validate_scale_strict(bitsize_t address_size, integer_value_t scale)
{
	if(!uint_fits(INTVAL(scale)))
		return false;
	switch(uint_get(INTVAL(scale)))
	{
	case 1:
		return true;
	case 2:
	case 4:
	case 8:
		return address_size != BITSIZE16;
	default:
		return false;
	}
}

bool validate_scale_permissive(bitsize_t address_size, integer_value_t scale)
{
	if(!uint_fits(INTVAL(scale)))
		return false;
	switch(uint_get(INTVAL(scale)))
	{
	case 1:
		return true;
	case 2:
	case 3:
	case 4:
	case 5:
	case 8:
	case 9:
		return address_size != BITSIZE16;
	default:
		return false;
	}
}

parser_state_t current_parser_state[1] =
{
	{
		.line_number = 1,
		.cpu_type = CPU_8086,
		.x80_cpu_type = CPU_8080,
		.bit_size = BITSIZE16,
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
	ins->bits = state->bit_size;
	ins->fpu = state->fpu_type;
	ins->condition = COND_NONE;
	ins->segment_prefix = REG_NOSEG;
	ins->source_segment_prefix = REG_NOSEG;
	ins->target_segment_prefix = REG_NOSEG;
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
	switch(opd->type)
	{
	case OPD_GPRB:
	case OPD_GPRH:
		opd->size = BITSIZE8;
		break;
	case OPD_GPRW:
#if 0
	case OPD_SEG:
#endif
		opd->size = BITSIZE16;
		break;
	case OPD_GPRD:
#if 0
	case OPD_TREG:
#endif
		opd->size = BITSIZE32;
		break;
	case OPD_GPRQ:
	case OPD_MMREG:
		opd->size = BITSIZE64;
		break;
#if 0
	case OPD_CREG:
	case OPD_DREG:
		opd->size = state->bit_size;
		break;
	case OPD_STREG:
		opd->size = BITSIZE80;
		break;
#endif
	case OPD_XMMREG:
		opd->size = BITSIZE128;
		break;
	case OPD_YMMREG:
		opd->size = BITSIZE256;
		break;
	case OPD_ZMMREG:
		opd->size = BITSIZE512;
		break;
	default:
		break;
	}
}

void operand_set_immediate(operand_t * opd, expression_t * value)
{
	memset(opd, 0, sizeof(operand_t));
	opd->parameter = value;
}

void operand_set_far_address(operand_t * opd, expression_t * segment, expression_t * offset)
{
	operand_set(opd, OPD_FARIMM);
	opd->parameter = offset;
	opd->segment_value = segment;
}

void operand_set_memory(operand_t * opd)
{
	operand_set(opd, OPD_MEM);
	opd->segment = REG_NOSEG;
	opd->base = REG_NONE;
	opd->index = REG_NONE;
	opd->scale = 1;
}

