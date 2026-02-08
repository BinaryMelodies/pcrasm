
#include "../asm.h"
#if TARGET_X86
# include "../x86/isa.h"
#else
# include "../x80/isa.h"
#endif

typedef enum constraint_t
{
	CST_ANY,
	CST_UI3, // values 0 to 7
	CST_UI5_GE8, // values 8 to 31
	CST_SI8,
	CST_UI8,
	CST_I8,
	CST_UI16,
	CST_I16,
	CST_REL8,
	CST_IR8, // 8080 8-bit register (including m)
	CST_IR8_NOTA, // 8080 8-bit register, excluding a (including m)
	CST_IR8_NOT_BOTH_M, // 8080 8-bit register, cannot be m together with operand 0
	CST_IR8_NOT_SAME, // 8080 8-bit register, cannot be same as operand 0
	CST_ZR8, // Z80 8-bit register (not including memory operands)
	CST_ZR8_NOTA, // Z80 8-bit register, excluding a
	CST_ZR8_NOPREF, // Z80 8-bit register that does not use DD/FD prefixes
	CST_ZR8_NOT_SAME, // 8080 8-bit register, cannot be same as operand 0
	CST_IR16, // 8080 16-bit register (not including psw)
	CST_IR16_NOSP, // 8080 16-bit register (not including sp/psw)
	CST_IMR16, // b or d
	CST_ZR16, // Z80 16-bit register (not including af)
	CST_ZR16_NOPREF, // Z80 16-bit register that does not use DD/FD prefixes
	CST_ZR16_NOSP, // 8080 16-bit register (not including sp/af)
	CST_ZIDX, // Z80 index register (hl, ix, iy)
	CST_ZM, // Z80 memory operand (must be either (hl), (ix+d), (iy+d))
	CST_ZMHL, // (hl)
	CST_ZMHLD, // (hl-) or (hld) or [.hl--]
	CST_ZMHLI, // (hl+) or (hli) or [.hl++]
	CST_ZMDED, // [.de--]
	CST_ZMDEI, // [.de++]
	CST_ZM_NOHL, // Z80 memory operand (must be either (ix+d), (iy+d))
	CST_ZMR16, // (bc) or (de)
	CST_ZMI3, // (imm) between 0 and 7
	CST_ZMI5_GE8, // (imm) between 8 and 31
	CST_ZMI8, // (imm)
	CST_ZMI8_HIGH, // (0xFF00) to (0xFFFF)
	CST_ZMI16, // (imm)
	CST_ZMIDX, // Z80 index register ((hl), (ix), (iy))
	CST_AREG,
	CST_HREG,
	CST_FREG, // .f register
	CST_DEREG,
	CST_HLREG,
	CST_AFREG,
	CST_AF2REG,
	CST_PSWREG,
	CST_SPREG,
	CST_SPMEM, // (sp)
	CST_IREG,
	CST_RREG,
	CST_CMEM, // (c) or (bc)
	CST_EQ0,
	CST_EQ1,
	CST_EQ2,
	CST_RST,
	CST_COND,
	CST_COND2, // only nz/z/nc/c permitted
} constraint_t;

typedef enum action_t
{
	ACT_END,
	ACT_PUT, // puts a single byte
	ACT_PUT2, // puts 2 bytes
	ACT_ADD_R0, // adds register 0
	ACT_ADD_R1, // adds register 1
	ACT_ADD_R0S3, // adds register 0 shifted left by 3
	ACT_ADD_R1S3, // adds register 1 shifted left by 3
	ACT_ADD_R0R1, // adds register 0 shifted left by 3 and register 1
	ACT_ADD_COND, // adds condition shifted left by 3
	ACT_ADD_REG, // adds register (stored in condition field)
	ACT_ADD_REGS, // adds pair of registers (stored in condition field)
	ACT_ADD_REGS3, // adds register (stored in condition field) shifted left by 3
	ACT_ADD_COND0S3, // adds condition (operand 0) shifted left by 3
	ACT_ADD_VALS1, // adds value (stored in condition field) shifted left by 1
	ACT_ADD_I0, // adds value of operand 0 (for RST)
	ACT_ADD_I0S1, // adds value of operand 0 shifted left by 1
	ACT_ADD_I0S3, // adds value of operand 0 shifted left by 3
	ACT_ADD_I1S1, // adds value of operand 1 shifted left by 1
	ACT_ADD_I0R1, // adds value of operand 0 shifted left by 3 and register 1
	ACT_ADD_I0R2, // adds value of operand 0 shifted left by 3 and register 2
	ACT_I8_0,
	ACT_I8_1,
	ACT_R8_0, // relative offset
	ACT_R8_1, // relative offset
	ACT_I16_0,
	ACT_I16_1,
	ACT_VAL, // write value (stored in condition field) as a byte
} action_t;

#define MAX_ACTION_COUNT 5

struct instruction_pattern_t
{
	cpu_type_t min_cpu, max_cpu;
	constraint_t constraint[MAX_OPD_COUNT + 1];
	action_t action[MAX_ACTION_COUNT];
#if !TARGET_X86
	action_t mcs8_action[MAX_ACTION_COUNT];
#endif
};

#if TARGET_X86
# define _MCS8(...)
#else
# define _MCS8(...) __VA_ARGS__
#endif

static const unsigned register_index[] =
{
	[X80_B] = 0,
	[X80_C] = 1,
	[X80_D] = 2,
	[X80_E] = 3,
	[X80_H] = 4,
	[X80_IXH] = 4,
	[X80_IYH] = 4,
	[X80_L] = 5,
	[X80_IXL] = 5,
	[X80_IYL] = 5,
	[X80_REG_M] = 6,
	[X80_A] = 7,

	[X80_BC] = 0,
	[X80_DE] = 2,
	[X80_HL] = 4,
	[X80_IX] = 4,
	[X80_IY] = 4,
	[X80_SP] = 6,
	[X80_AF] = 6,
	[X80_PSW] = 6,
};

static const unsigned condition_index[] =
{
	[X80_C] = 3,
	[X80_CND_M] = 7,
	[X80_NC] = 2,
	[X80_NZ] = 0,
	[X80_P] = 6,
	[X80_PE] = 5,
	[X80_PO] = 4,
	[X80_Z] = 1,
};

enum
{
	OPDMODE_IR8 = 0x01,
	OPDMODE_ZR8 = 0x02,
	OPDMODE_IR16 = 0x04,
	OPDMODE_ZR16 = 0x08,
	OPDMODE_COND = 0x10,
};

static const unsigned operand_mode[] =
{
	[X80_A] = OPDMODE_IR8 | OPDMODE_ZR8,
	[X80_AF] = 0,
	[X80_AF2] = 0,
	[X80_B] = OPDMODE_IR8 | OPDMODE_IR16 | OPDMODE_ZR8,
	[X80_BC] = OPDMODE_ZR16,
	[X80_C] = OPDMODE_IR8 | OPDMODE_ZR8 | OPDMODE_COND,
	[X80_CND_M] = OPDMODE_COND,
	[X80_D] = OPDMODE_IR8 | OPDMODE_IR16 | OPDMODE_ZR8,
	[X80_DE] = OPDMODE_ZR16,
	[X80_E] = OPDMODE_IR8 | OPDMODE_ZR8,
	[X80_H] = OPDMODE_IR8 | OPDMODE_IR16 | OPDMODE_ZR8,
	[X80_HL] = OPDMODE_ZR16,
	[X80_I] = 0,
	[X80_IX] = OPDMODE_ZR16,
	[X80_IXH] = OPDMODE_ZR8,
	[X80_IXL] = OPDMODE_ZR8,
	[X80_IY] = OPDMODE_ZR16,
	[X80_IYH] = OPDMODE_ZR8,
	[X80_IYL] = OPDMODE_ZR8,
	[X80_L] = OPDMODE_IR8 | OPDMODE_ZR8,
	[X80_NC] = OPDMODE_COND,
	[X80_NZ] = OPDMODE_COND,
	[X80_P] = OPDMODE_COND,
	[X80_PE] = OPDMODE_COND,
	[X80_PO] = OPDMODE_COND,
	[X80_PSW] = 0,
	[X80_R] = 0,
	[X80_REG_M] = OPDMODE_IR8,
	[X80_SP] = OPDMODE_IR16 | OPDMODE_ZR16,
	[X80_Z] = OPDMODE_COND,
};

#define PREFIX_IX 0xDD
#define PREFIX_IY 0xFD
#define NO_PREFIX ((uint8_t)-1)

static const uint8_t register_prefix[] =
{
	[X80_H] = NO_PREFIX,
	[X80_IXH] = PREFIX_IX,
	[X80_IYH] = PREFIX_IY,
	[X80_L] = NO_PREFIX,
	[X80_IXL] = PREFIX_IX,
	[X80_IYL] = PREFIX_IY,

	[X80_HL] = NO_PREFIX,
	[X80_IX] = PREFIX_IX,
	[X80_IY] = PREFIX_IY,
};

static const pattern_t x80_patterns[_MNEM_TOTAL];

enum error_type
{
	_ERROR_TYPE_SUCCESS = 0, // not an error
	ERROR_TYPE_INTERNAL, // internal error
	ERROR_TYPE_OPCOUNT_ERROR, // invalid number of operands
	ERROR_TYPE_INVALID_OPERAND, // invalid operand
	ERROR_TYPE_OUT_OF_RANGE, // operand value outside permitted range
	ERROR_TYPE_WRONG_CPU, // not supported on specified CPU
	_ERROR_TYPE_COUNT,
};

static match_result_t instruction_pattern_match(const instruction_pattern_t * pattern, instruction_t * ins, bool forgiving)
{
	match_result_t result = { .type = MATCH_PERFECT };

#if TARGET_X80
	if(ins->cpu < pattern->min_cpu || ins->cpu > pattern->max_cpu)
	{
		result.type = MATCH_FAILED;
		result.info = ERROR_TYPE_WRONG_CPU;
		return result;
	}
#endif

	uint8_t prefix = 0;
	for(size_t operand_index = 0; operand_index < ins->operand_count; operand_index++)
	{
		reference_t ref[1];
		switch(pattern->constraint[operand_index])
		{
		case CST_ANY:
			break;
		case CST_UI3:
			if(ins->operand[operand_index].type != OPD_IMM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(forgiving)
				break;
			evaluate_expression(ins->operand[operand_index].parameter, ref, ins->code_offset);
			if(!is_scalar(ref) || !uint_fits(ref->value) || uint_get(ref->value) > 7)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_OUT_OF_RANGE;
			}
			int_clear(ref->value);
			break;
		case CST_UI5_GE8:
			if(ins->operand[operand_index].type != OPD_IMM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(forgiving)
				break;
			evaluate_expression(ins->operand[operand_index].parameter, ref, ins->code_offset);
			if(!is_scalar(ref) || !uint_fits(ref->value) || uint_get(ref->value) < 8 || uint_get(ref->value) > 31)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_OUT_OF_RANGE;
			}
			int_clear(ref->value);
			break;
		case CST_SI8:
			if(ins->operand[operand_index].type != OPD_IMM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(forgiving)
				break;
			evaluate_expression(ins->operand[operand_index].parameter, ref, ins->code_offset);
			if(!is_scalar(ref) || integer_get_size(ref->value) > 1)
			{
				result.type = MATCH_TRUNCATED;
			}
			int_clear(ref->value);
			break;
		case CST_UI8:
			if(ins->operand[operand_index].type != OPD_IMM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(forgiving)
				break;
			evaluate_expression(ins->operand[operand_index].parameter, ref, ins->code_offset);
			if(!is_scalar(ref) || uinteger_get_size(ref->value) > 1)
			{
				result.type = MATCH_TRUNCATED;
			}
			int_clear(ref->value);
			break;
		case CST_I8:
			if(ins->operand[operand_index].type != OPD_IMM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(forgiving)
				break;
			evaluate_expression(ins->operand[operand_index].parameter, ref, ins->code_offset);
			if(!is_scalar(ref) || (integer_get_size(ref->value) > 1 && uinteger_get_size(ref->value) > 1))
			{
				result.type = MATCH_TRUNCATED;
			}
			int_clear(ref->value);
			break;
		case CST_UI16:
			if(ins->operand[operand_index].type != OPD_IMM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(forgiving)
				break;
			evaluate_expression(ins->operand[operand_index].parameter, ref, ins->code_offset);
			if(!is_scalar(ref) || uinteger_get_size(ref->value) > 2)
			{
				result.type = MATCH_TRUNCATED;
			}
			int_clear(ref->value);
			break;
		case CST_I16:
			if(ins->operand[operand_index].type != OPD_IMM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(forgiving)
				break;
			evaluate_expression(ins->operand[operand_index].parameter, ref, ins->code_offset);
			if(!is_scalar(ref) || (integer_get_size(ref->value) > 2 && uinteger_get_size(ref->value) > 2))
			{
				result.type = MATCH_TRUNCATED;
			}
			int_clear(ref->value);
			break;
		case CST_REL8:
			if(ins->operand[operand_index].type != OPD_IMM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(forgiving)
				break;
			evaluate_expression(ins->operand[operand_index].parameter, ref, ins->code_offset);
			uint_sub_ui(ref->value, ins->following->code_offset);

			current_section = ins->containing_section;
			if(!is_self_relative(ref))
			{
				result.type = MATCH_TRUNCATED;
			}
			else if(integer_get_size(ref->value) > 1)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_OUT_OF_RANGE;
			}

			int_clear(ref->value);
			break;
		case CST_IR8:
		case CST_IR8_NOTA:
		case CST_IR8_NOT_BOTH_M:
		case CST_IR8_NOT_SAME:
			if(ins->operand[operand_index].type != OPD_TOKEN || (operand_mode[ins->operand[operand_index].base] & OPDMODE_IR8) == 0)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(pattern->constraint[operand_index] == CST_IR8_NOTA && (int)ins->operand[operand_index].base == X80_A)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(pattern->constraint[operand_index] == CST_IR8_NOT_BOTH_M && (int)ins->operand[operand_index].base == X80_REG_M)
			{
				if((int)ins->operand[0].base == X80_REG_M)
				{
					result.type = MATCH_FAILED;
					result.info = ERROR_TYPE_INVALID_OPERAND;
					return result;
				}
			}
			if(pattern->constraint[operand_index] == CST_IR8_NOT_SAME)
			{
				if(ins->operand[0].base == ins->operand[operand_index].base)
				{
					result.type = MATCH_FAILED;
					result.info = ERROR_TYPE_INVALID_OPERAND;
					return result;
				}
			}
			break;
		case CST_ZR8:
		case CST_ZR8_NOTA:
		case CST_ZR8_NOT_SAME:
			if(ins->operand[operand_index].type != OPD_TOKEN || (operand_mode[ins->operand[operand_index].base] & OPDMODE_ZR8) == 0)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(prefix == 0)
				prefix = register_prefix[ins->operand[operand_index].base];
			else if(register_prefix[ins->operand[operand_index].base] != 0 && register_prefix[ins->operand[operand_index].base] != prefix)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(pattern->constraint[operand_index] == CST_ZR8_NOTA && (int)ins->operand[operand_index].base == X80_A)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(pattern->constraint[operand_index] == CST_ZR8_NOT_SAME)
			{
				if(ins->operand[0].base == ins->operand[operand_index].base)
				{
					result.type = MATCH_FAILED;
					result.info = ERROR_TYPE_INVALID_OPERAND;
					return result;
				}
			}
			break;
		case CST_ZR8_NOPREF:
			if(ins->operand[operand_index].type != OPD_TOKEN || (operand_mode[ins->operand[operand_index].base] & OPDMODE_ZR8) == 0)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(register_prefix[ins->operand[operand_index].base] != 0 && register_prefix[ins->operand[operand_index].base] != NO_PREFIX)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_IR16:
			if(ins->operand[operand_index].type != OPD_TOKEN || (operand_mode[ins->operand[operand_index].base] & OPDMODE_IR16) == 0)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_IR16_NOSP:
			if(ins->operand[operand_index].type != OPD_TOKEN || (operand_mode[ins->operand[operand_index].base] & OPDMODE_IR16) == 0 || (int)ins->operand[operand_index].base == X80_SP)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_IMR16:
			if(ins->operand[operand_index].type != OPD_TOKEN)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if((int)ins->operand[operand_index].base != X80_B && (int)ins->operand[operand_index].base != X80_D)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_ZR16:
			if(ins->operand[operand_index].type != OPD_TOKEN || (operand_mode[ins->operand[operand_index].base] & OPDMODE_ZR16) == 0)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(prefix == 0)
				prefix = register_prefix[ins->operand[operand_index].base];
			else if(register_prefix[ins->operand[operand_index].base] != 0 && register_prefix[ins->operand[operand_index].base] != prefix)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_ZR16_NOPREF:
			if(ins->operand[operand_index].type != OPD_TOKEN || (operand_mode[ins->operand[operand_index].base] & OPDMODE_ZR16) == 0)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(register_prefix[ins->operand[operand_index].base] != 0 && register_prefix[ins->operand[operand_index].base] != NO_PREFIX)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_ZR16_NOSP:
			if(ins->operand[operand_index].type != OPD_TOKEN || (operand_mode[ins->operand[operand_index].base] & OPDMODE_ZR16) == 0 || (int)ins->operand[operand_index].base == X80_SP)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(prefix == 0)
				prefix = register_prefix[ins->operand[operand_index].base];
			else if(register_prefix[ins->operand[operand_index].base] != 0 && register_prefix[ins->operand[operand_index].base] != prefix)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_ZIDX:
			if(ins->operand[operand_index].type != OPD_TOKEN)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			switch((int)ins->operand[operand_index].base)
			{
			case X80_HL:
				if(prefix != 0 && prefix != NO_PREFIX)
				{
					result.type = MATCH_FAILED;
					result.info = ERROR_TYPE_INVALID_OPERAND;
					return result;
				}
				prefix = NO_PREFIX;
				break;
			case X80_IX:
				if(prefix != 0 && prefix != PREFIX_IX)
				{
					result.type = MATCH_FAILED;
					result.info = ERROR_TYPE_INVALID_OPERAND;
					return result;
				}
				prefix = PREFIX_IX;
				break;
			case X80_IY:
				if(prefix != 0 && prefix != PREFIX_IY)
				{
					result.type = MATCH_FAILED;
					result.info = ERROR_TYPE_INVALID_OPERAND;
					return result;
				}
				prefix = PREFIX_IY;
				break;
			default:
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_ZMIDX:
			if(ins->operand[operand_index].type != OPD_MEM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			switch((int)ins->operand[operand_index].base)
			{
			case X80_HL:
				if(prefix != 0 && prefix != NO_PREFIX)
				{
					result.type = MATCH_FAILED;
					result.info = ERROR_TYPE_INVALID_OPERAND;
					return result;
				}
				prefix = NO_PREFIX;
				break;
			case X80_IX:
				if(prefix != 0 && prefix != PREFIX_IX)
				{
					result.type = MATCH_FAILED;
					result.info = ERROR_TYPE_INVALID_OPERAND;
					return result;
				}
				prefix = PREFIX_IX;
				break;
			case X80_IY:
				if(prefix != 0 && prefix != PREFIX_IY)
				{
					result.type = MATCH_FAILED;
					result.info = ERROR_TYPE_INVALID_OPERAND;
					return result;
				}
				prefix = PREFIX_IY;
				break;
			default:
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_OUT_OF_RANGE;
				return result;
			}

			evaluate_expression(ins->operand[operand_index].parameter, ref, ins->code_offset);
			if(!is_scalar(ref))
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_OUT_OF_RANGE;
				return result;
			}
			if(int_is_zero(ref->value))
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_OUT_OF_RANGE;
				return result;
			}
			int_clear(ref->value);
			break;
		case CST_ZMHL:
			if(ins->operand[operand_index].type != OPD_MEM || (int)ins->operand[operand_index].base != X80_HL || ins->operand[operand_index].parameter != NULL)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_ZMHLD:
#if TARGET_X80
			if(ins->operand[operand_index].type != OPD_HLD)
#endif
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_ZMHLI:
#if TARGET_X80
			if(ins->operand[operand_index].type != OPD_HLI)
#endif
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_ZMDED:
#if TARGET_X80
			if(ins->operand[operand_index].type != OPD_DED)
#endif
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_ZMDEI:
#if TARGET_X80
			if(ins->operand[operand_index].type != OPD_DEI)
#endif
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_ZM:
		case CST_ZM_NOHL:
			if(ins->operand[operand_index].type != OPD_MEM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			switch((int)ins->operand[operand_index].base)
			{
			case X80_HL:
				if(pattern->constraint[operand_index] == CST_ZM_NOHL)
				{
					result.type = MATCH_FAILED;
					result.info = ERROR_TYPE_INVALID_OPERAND;
					return result;
				}
				if(prefix != 0 && prefix != NO_PREFIX)
				{
					result.type = MATCH_FAILED;
					result.info = ERROR_TYPE_INVALID_OPERAND;
					return result;
				}
				prefix = NO_PREFIX;
				break;
			case X80_IX:
				if(prefix != 0 && prefix != PREFIX_IX)
				{
					result.type = MATCH_FAILED;
					result.info = ERROR_TYPE_INVALID_OPERAND;
					return result;
				}
				prefix = PREFIX_IX;
				break;
			case X80_IY:
				if(prefix != 0 && prefix != PREFIX_IY)
				{
					result.type = MATCH_FAILED;
					result.info = ERROR_TYPE_INVALID_OPERAND;
					return result;
				}
				prefix = PREFIX_IY;
				break;
			default:
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}

			if(prefix != NO_PREFIX)
			{
				// check parameter size
				if(forgiving)
					break;
				evaluate_expression(ins->operand[operand_index].parameter, ref, ins->code_offset);
				if(is_scalar(ref) && integer_get_size(ref->value) > 1)
				{
					result.type = MATCH_FAILED;
					result.info = ERROR_TYPE_OUT_OF_RANGE;
				}
				int_clear(ref->value);
			}
			break;
		case CST_ZMR16:
			if(ins->operand[operand_index].type != OPD_MEM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if((int)ins->operand[operand_index].base != X80_BC && (int)ins->operand[operand_index].base != X80_DE)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			evaluate_expression(ins->operand[operand_index].parameter, ref, ins->code_offset);
			if(!is_scalar(ref) || !int_is_zero(ref->value))
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
			}
			int_clear(ref->value);
			break;
		case CST_ZMI3:
			if(ins->operand[operand_index].type != OPD_MEM || ins->operand[operand_index].base != REG_NONE)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(forgiving)
				break;
			evaluate_expression(ins->operand[operand_index].parameter, ref, ins->code_offset);
			if(!is_scalar(ref) || !uint_fits(ref->value) || uint_get(ref->value) > 7)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_OUT_OF_RANGE;
			}
			int_clear(ref->value);
			break;
		case CST_ZMI5_GE8:
			if(ins->operand[operand_index].type != OPD_MEM || ins->operand[operand_index].base != REG_NONE)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(forgiving)
				break;
			evaluate_expression(ins->operand[operand_index].parameter, ref, ins->code_offset);
			if(!is_scalar(ref) || !uint_fits(ref->value) || uint_get(ref->value) < 8 || uint_get(ref->value) > 31)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_OUT_OF_RANGE;
			}
			int_clear(ref->value);
			break;
		case CST_ZMI8:
			if(ins->operand[operand_index].type != OPD_MEM || ins->operand[operand_index].base != REG_NONE)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(forgiving)
				break;
			evaluate_expression(ins->operand[operand_index].parameter, ref, ins->code_offset);
			if(!is_scalar(ref) || uinteger_get_size(ref->value) > 1)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_OUT_OF_RANGE;
			}
			int_clear(ref->value);
			break;
		case CST_ZMI8_HIGH:
			if(ins->operand[operand_index].type != OPD_MEM || ins->operand[operand_index].base != REG_NONE)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(forgiving)
				break;
			evaluate_expression(ins->operand[operand_index].parameter, ref, ins->code_offset);
			if(!is_scalar(ref))
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_OUT_OF_RANGE;
			}
			else if(uint_cmp_ui(ref->value, 0xFF00) < 0 || uint_cmp_ui(ref->value, 0xFFFF) > 0)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_OUT_OF_RANGE;
			}
			int_clear(ref->value);
			break;
		case CST_ZMI16:
			if(ins->operand[operand_index].type != OPD_MEM || ins->operand[operand_index].base != REG_NONE)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(forgiving)
				break;
			evaluate_expression(ins->operand[operand_index].parameter, ref, ins->code_offset);
			if(!is_scalar(ref) || uinteger_get_size(ref->value) > 2)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_OUT_OF_RANGE;
			}
			int_clear(ref->value);
			break;
		case CST_AREG:
			if(ins->operand[operand_index].type != OPD_TOKEN || (int)ins->operand[operand_index].base != X80_A)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_HREG:
			if(ins->operand[operand_index].type != OPD_TOKEN || (int)ins->operand[operand_index].base != X80_H)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_FREG:
#if TARGET_X80
			if(ins->operand[operand_index].type != OPD_TOKEN || (int)ins->operand[operand_index].base != X80_F)
#endif
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_DEREG:
			if(ins->operand[operand_index].type != OPD_TOKEN || (int)ins->operand[operand_index].base != X80_DE)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_HLREG:
			if(ins->operand[operand_index].type != OPD_TOKEN || (int)ins->operand[operand_index].base != X80_HL)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_AFREG:
			if(ins->operand[operand_index].type != OPD_TOKEN || (int)ins->operand[operand_index].base != X80_AF)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_AF2REG:
			if(ins->operand[operand_index].type != OPD_TOKEN || (int)ins->operand[operand_index].base != X80_AF2)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_PSWREG:
			if(ins->operand[operand_index].type != OPD_TOKEN || (int)ins->operand[operand_index].base != X80_PSW)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_SPREG:
			if(ins->operand[operand_index].type != OPD_TOKEN || (int)ins->operand[operand_index].base != X80_SP)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_SPMEM:
			if(ins->operand[operand_index].type != OPD_MEM || (int)ins->operand[operand_index].base != X80_SP)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_IREG:
			if(ins->operand[operand_index].type != OPD_TOKEN || (int)ins->operand[operand_index].base != X80_I)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_RREG:
			if(ins->operand[operand_index].type != OPD_TOKEN || (int)ins->operand[operand_index].base != X80_R)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_CMEM:
			if(ins->operand[operand_index].type != OPD_MEM || ((int)ins->operand[operand_index].base != X80_C && (int)ins->operand[operand_index].base != X80_BC))
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_EQ0:
			if(ins->operand[operand_index].type != OPD_IMM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(forgiving)
				break;
			evaluate_expression(ins->operand[operand_index].parameter, ref, ins->code_offset);
			if(!is_scalar(ref))
			{
				int_clear(ref->value);
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(!int_is_zero(ref->value))
			{
				int_clear(ref->value);
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			int_clear(ref->value);
			break;
		case CST_EQ1:
			if(ins->operand[operand_index].type != OPD_IMM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(forgiving)
				break;
			evaluate_expression(ins->operand[operand_index].parameter, ref, ins->code_offset);
			if(!is_scalar(ref))
			{
				int_clear(ref->value);
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(uint_cmp_ui(ref->value, 1) != 0)
			{
				int_clear(ref->value);
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			int_clear(ref->value);
			break;
		case CST_EQ2:
			if(ins->operand[operand_index].type != OPD_IMM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(forgiving)
				break;
			evaluate_expression(ins->operand[operand_index].parameter, ref, ins->code_offset);
			if(!is_scalar(ref))
			{
				int_clear(ref->value);
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(uint_cmp_ui(ref->value, 2) != 0)
			{
				int_clear(ref->value);
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			int_clear(ref->value);
			break;
		case CST_RST:
			if(ins->operand[operand_index].type != OPD_IMM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(forgiving)
				break;
			evaluate_expression(ins->operand[operand_index].parameter, ref, ins->code_offset);
			if(!is_scalar(ref))
			{
				int_clear(ref->value);
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(!uint_fits(ref->value) || (uint_get(ref->value) & ~0x38) != 0)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			int_clear(ref->value);
			break;
		case CST_COND:
			if(ins->operand[operand_index].type != OPD_TOKEN || (operand_mode[ins->operand[operand_index].base] & OPDMODE_COND) == 0)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
#if TARGET_X80
			if(ins->cpu == CPU_GBZ80 && condition_index[ins->operand[operand_index].base] >= 4)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
#endif
			break;
		case CST_COND2:
			if(ins->operand[operand_index].type != OPD_TOKEN || (operand_mode[ins->operand[operand_index].base] & OPDMODE_COND) == 0
					|| condition_index[ins->operand[operand_index].base] >= 4)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		}
	}

	if(prefix != 0 && (ins->cpu < CPU_Z80 || ins->cpu == CPU_GBZ80))
	{
		result.type = MATCH_FAILED;
		result.info = ERROR_TYPE_INVALID_OPERAND;
		return result;
	}

	return result;
}

#if TARGET_X86
# define is_mcs8(__ins) false
#else
# define is_mcs8(__ins) ((__ins)->cpu == CPU_DP2200 || (__ins)->cpu == CPU_DP2200V2 || (__ins)->cpu == CPU_8008)
#endif

static size_t instruction_pattern_get_length(const instruction_pattern_t * pattern, instruction_t * ins)
{
	size_t length = 0;

	bool has_prefix = false;
	bool has_displacement = false;

	for(size_t operand_index = 0; operand_index < ins->operand_count; operand_index++)
	{
		switch(pattern->constraint[operand_index])
		{
		default:
			break;
		case CST_ZR8:
		case CST_ZR16:
		case CST_ZR16_NOSP:
		case CST_ZIDX:
		case CST_ZMIDX:
			if(register_prefix[ins->operand[operand_index].base] != 0 && register_prefix[ins->operand[operand_index].base] != NO_PREFIX)
				has_prefix = true;
			break;
		case CST_ZM:
		case CST_ZM_NOHL:
			if(register_prefix[ins->operand[operand_index].base] != 0 && register_prefix[ins->operand[operand_index].base] != NO_PREFIX)
				has_prefix = true;
			if(has_prefix)
			{
				has_displacement = true;
			}
			break;
		}
	}

	if(has_prefix)
		length++;
	if(has_displacement)
		length++;

	const action_t * actions;
#if TARGET_X86
	actions = pattern->action;
#else
	actions = is_mcs8(ins) ? pattern->mcs8_action : pattern->action;
#endif

	for(size_t action_index = 0; action_index < MAX_ACTION_COUNT; action_index++)
	{
		switch(actions[action_index])
		{
		case ACT_END:
			goto end;
		case ACT_PUT:
			length++;
			action_index++;
			break;
		case ACT_PUT2:
			length += 2;
			action_index += 2;
			break;
		case ACT_ADD_R0:
		case ACT_ADD_R1:
		case ACT_ADD_R0S3:
		case ACT_ADD_R1S3:
		case ACT_ADD_R0R1:
		case ACT_ADD_COND:
		case ACT_ADD_REG:
		case ACT_ADD_REGS:
		case ACT_ADD_REGS3:
		case ACT_ADD_COND0S3:
		case ACT_ADD_VALS1:
		case ACT_ADD_I0:
		case ACT_ADD_I0S1:
		case ACT_ADD_I0S3:
		case ACT_ADD_I1S1:
		case ACT_ADD_I0R1:
		case ACT_ADD_I0R2:
			length++;
			action_index++;
			break;
		case ACT_I8_0:
		case ACT_I8_1:
		case ACT_R8_0:
		case ACT_R8_1:
		case ACT_VAL:
			length++;
			break;
		case ACT_I16_0:
		case ACT_I16_1:
			length += 2;
			break;
		}
	}

end:
	return length;
}

static void instruction_pattern_generate(const instruction_pattern_t * pattern, instruction_t * ins)
{
	uint8_t prefix = 0;
	reference_t displacement[1];
	bool has_displacement = false;

	for(size_t operand_index = 0; operand_index < ins->operand_count; operand_index++)
	{
		switch(pattern->constraint[operand_index])
		{
		default:
			break;
		case CST_ZR8:
			if(register_prefix[ins->operand[operand_index].base] != 0)
				prefix = register_prefix[ins->operand[operand_index].base];
			break;
		case CST_ZR16:
		case CST_ZR16_NOSP:
			if(register_prefix[ins->operand[operand_index].base] != 0)
				prefix = register_prefix[ins->operand[operand_index].base];
			break;
		case CST_ZM:
		case CST_ZM_NOHL:
			if(register_prefix[ins->operand[operand_index].base] != 0)
				prefix = register_prefix[ins->operand[operand_index].base];
			if(prefix != 0 && prefix != NO_PREFIX)
			{
				has_displacement = true;
				evaluate_expression(ins->operand[operand_index].parameter, displacement, ins->code_offset);
			}
			break;
		case CST_ZIDX:
		case CST_ZMIDX:
			if(register_prefix[ins->operand[operand_index].base] != 0)
				prefix = register_prefix[ins->operand[operand_index].base];
			break;
		}
	}

	if(prefix == NO_PREFIX)
		prefix = 0;
	if(prefix != 0)
		output_byte(prefix);

	const action_t * actions;
#if TARGET_X86
	actions = pattern->action;
#else
	actions = ins->cpu == CPU_DP2200 || ins->cpu == CPU_DP2200V2 || ins->cpu == CPU_8008 ? pattern->mcs8_action : pattern->action;
#endif

	for(size_t action_index = 0; action_index < MAX_ACTION_COUNT; action_index++)
	{
		unsigned index, index2;
		reference_t ref[1];
		switch(actions[action_index])
		{
		case ACT_END:
			return;
		case ACT_PUT:
			output_byte(actions[++action_index]);
			if(action_index == 1 && has_displacement)
				output_word_as(displacement, DATA_LE, 1, false, R_Z80_8_DIS);
			break;
		case ACT_PUT2:
			output_byte(actions[++action_index]);
			if(action_index == 1 && has_displacement)
				output_word_as(displacement, DATA_LE, 1, false, R_Z80_8_DIS);
			output_byte(actions[++action_index]);
			break;
		case ACT_ADD_R0:
			index = register_index[ins->operand[0].base];
#if TARGET_X80
			if(is_mcs8(ins))
				index = register_i80_to_i8(index);
#endif
			output_byte(actions[++action_index] + index);
			if(action_index == 1 && has_displacement)
				output_word_as(displacement, DATA_LE, 1, false, R_Z80_8_DIS);
			break;
		case ACT_ADD_R1:
			index = register_index[ins->operand[1].base];
#if TARGET_X80
			if(is_mcs8(ins))
				index = register_i80_to_i8(index);
#endif
			output_byte(actions[++action_index] + index);
			if(action_index == 1 && has_displacement)
				output_word_as(displacement, DATA_LE, 1, false, R_Z80_8_DIS);
			break;
		case ACT_ADD_R0S3:
			index = register_index[ins->operand[0].base];
#if TARGET_X80
			if(is_mcs8(ins))
				index = register_i80_to_i8(index);
#endif
			output_byte(actions[++action_index] + (index << 3));
			if(action_index == 1 && has_displacement)
				output_word_as(displacement, DATA_LE, 1, false, R_Z80_8_DIS);
			break;
		case ACT_ADD_R1S3:
			index = register_index[ins->operand[1].base];
#if TARGET_X80
			if(is_mcs8(ins))
				index = register_i80_to_i8(index);
#endif
			output_byte(actions[++action_index] + (index << 3));
			if(action_index == 1 && has_displacement)
				output_word_as(displacement, DATA_LE, 1, false, R_Z80_8_DIS);
			break;
		case ACT_ADD_R0R1:
			index = register_index[ins->operand[0].base];
			index2 = register_index[ins->operand[1].base];
#if TARGET_X80
			if(is_mcs8(ins))
			{
				index = register_i80_to_i8(index);
				index2 = register_i80_to_i8(index2);
			}
#endif
			output_byte(actions[++action_index] + (index << 3) + index2);
			if(action_index == 1 && has_displacement)
				output_word_as(displacement, DATA_LE, 1, false, R_Z80_8_DIS);
			break;
		case ACT_ADD_COND:
			index = ins->condition;
#if TARGET_X80
			if(is_mcs8(ins))
				index = condition_i80_to_i8(index);
#endif
			output_byte(actions[++action_index] + (index << 3));
			break;
		case ACT_ADD_REG:
			index = ins->condition;
#if TARGET_X80
			if(!is_mcs8(ins))
				index = register_i8_to_i80(index);
#endif
			output_byte(actions[++action_index] + index);
			break;
		case ACT_ADD_REGS:
			index = ins->condition;
#if TARGET_X80
			if(!is_mcs8(ins))
			{
				index = (ins->condition) >> 3;
				index2 = ins->condition & 7;
				index = register_i8_to_i80(index);
				index2 = register_i8_to_i80(index2);
				index = (index << 3) + index2;
			}
#endif
			output_byte(actions[++action_index] + index);
			break;
		case ACT_ADD_REGS3:
			index = ins->condition;
#if TARGET_X80
			if(!is_mcs8(ins))
				index = register_i8_to_i80(index);
#endif
			output_byte(actions[++action_index] + (index << 3));
			break;
		case ACT_ADD_COND0S3:
			index = condition_index[ins->operand[0].base];
#if TARGET_X80
			if(is_mcs8(ins))
				index = condition_i80_to_i8(index);
#endif
			output_byte(actions[++action_index] + (index << 3));
			break;
		case ACT_ADD_VALS1:
			output_byte(actions[++action_index] + (ins->condition << 1));
			break;
		case ACT_ADD_I0:
			evaluate_expression(ins->operand[0].parameter, ref, ins->code_offset);
			output_byte(actions[++action_index] + uint_get(ref->value));
			int_clear(ref->value);
			break;
		case ACT_ADD_I0S1:
			evaluate_expression(ins->operand[0].parameter, ref, ins->code_offset);
			output_byte(actions[++action_index] + (uint_get(ref->value) << 1));
			int_clear(ref->value);
			break;
		case ACT_ADD_I0S3:
			evaluate_expression(ins->operand[0].parameter, ref, ins->code_offset);
			output_byte(actions[++action_index] + (uint_get(ref->value) << 3));
			int_clear(ref->value);
			break;
		case ACT_ADD_I1S1:
			evaluate_expression(ins->operand[1].parameter, ref, ins->code_offset);
			output_byte(actions[++action_index] + (uint_get(ref->value) << 1));
			int_clear(ref->value);
			break;
		case ACT_ADD_I0R1:
			evaluate_expression(ins->operand[0].parameter, ref, ins->code_offset);
			index = register_index[ins->operand[1].base];
#if TARGET_X80
			if(is_mcs8(ins))
				index = register_i80_to_i8(index);
#endif
			output_byte(actions[++action_index] + (uint_get(ref->value) << 3) + index);
			int_clear(ref->value);
			break;
		case ACT_ADD_I0R2:
			evaluate_expression(ins->operand[0].parameter, ref, ins->code_offset);
			index = register_index[ins->operand[2].base];
#if TARGET_X80
			if(is_mcs8(ins))
				index = register_i80_to_i8(index);
#endif
			output_byte(actions[++action_index] + (uint_get(ref->value) << 3) + index);
			int_clear(ref->value);
			break;
		case ACT_I8_0:
			evaluate_expression(ins->operand[0].parameter, ref, ins->code_offset);
			output_word(ref, DATA_LE, 1);
			int_clear(ref->value);
			break;
		case ACT_I8_1:
			evaluate_expression(ins->operand[1].parameter, ref, ins->code_offset);
			output_word(ref, DATA_LE, 1);
			int_clear(ref->value);
			break;
		case ACT_R8_0:
			evaluate_expression(ins->operand[0].parameter, ref, ins->code_offset);
			uint_sub_ui(ref->value, ins->following->code_offset);
			output_word_pcrel(ref, DATA_LE, 1);
			int_clear(ref->value);
			break;
		case ACT_R8_1:
			evaluate_expression(ins->operand[1].parameter, ref, ins->code_offset);
			uint_sub_ui(ref->value, ins->following->code_offset);
			output_word_pcrel(ref, DATA_LE, 1);
			int_clear(ref->value);
			break;
		case ACT_I16_0:
			evaluate_expression(ins->operand[0].parameter, ref, ins->code_offset);
			output_word(ref, DATA_LE, 2);
			int_clear(ref->value);
			break;
		case ACT_I16_1:
			evaluate_expression(ins->operand[1].parameter, ref, ins->code_offset);
			output_word(ref, DATA_LE, 2);
			int_clear(ref->value);
			break;
		case ACT_VAL:
			output_byte(ins->condition);
			break;
		}
	}
}

static const instruction_pattern_t * find_pattern(instruction_t * ins, bool forgiving, uint64_t * error_types_ptr)
{
	uint64_t error_types = 0;
	const struct instruction_patterns_t * patterns = &x80_patterns[ins->mnemonic].pattern[ins->operand_count];
	if(patterns->pattern == NULL)
	{
		if(error_types_ptr)
			*error_types_ptr |= 1 << ERROR_TYPE_OPCOUNT_ERROR;
		return NULL;
	}

	size_t perfect_match_length = (size_t)-1;
	size_t last_match = (size_t)-1;
	for(size_t pattern_index = 0; pattern_index < patterns->count; pattern_index++)
	{
		match_result_t match = instruction_pattern_match(&patterns->pattern[pattern_index], ins, forgiving);
		switch(match.type)
		{
		case MATCH_PERFECT:
		case MATCH_TRUNCATED: // TODO: signal warning
			{
				// find shortest match ("tightest")
				size_t new_length = instruction_pattern_get_length(&patterns->pattern[pattern_index], ins);
				if(new_length < perfect_match_length)
				{
					perfect_match_length = new_length;
					last_match = pattern_index;
				}
			}
			break;
		case MATCH_FAILED:
			error_types |= 1 << match.info;
			break;
		}
	}

	if(last_match == (size_t)-1)
	{
		if(error_types_ptr)
			*error_types_ptr |= error_types;
		return NULL;
	}
	else
	{
		return &patterns->pattern[last_match];
	}
}

static void report_errors(size_t line_number, uint64_t errors)
{
	fprintf(stderr, "Line %ld: ", line_number);
	for(int error_type = _ERROR_TYPE_SUCCESS + 1; error_type < _ERROR_TYPE_COUNT; error_type++)
	{
		if((errors & (1 << error_type)) == 0)
			continue;
		switch(error_type)
		{
		case ERROR_TYPE_INTERNAL:
			fprintf(stderr, "Internal error\n");
			break;
		case ERROR_TYPE_OPCOUNT_ERROR:
			fprintf(stderr, "Invalid number of operands\n");
			break;
		case ERROR_TYPE_INVALID_OPERAND:
			fprintf(stderr, "Invalid operand\n");
			break;
		case ERROR_TYPE_OUT_OF_RANGE:
			fprintf(stderr, "Operand value outside permitted range\n");
			break;
		case ERROR_TYPE_WRONG_CPU:
			fprintf(stderr, "Instruction not supported on specified architecture\n");
			break;
		}
	}
}

size_t x80_instruction_compute_length(instruction_t * ins, bool forgiving)
{
	uint64_t error_types = 0;
	const instruction_pattern_t * pattern = find_pattern(ins, forgiving, &error_types);
	if(pattern == NULL)
	{
		report_errors(ins->line_number, error_types);
		//fprintf(stderr, "Line %ld: Invalid instruction\n", ins->line_number);
		//return RESULT_FAILED; // TODO: should return RESULT_FAILED
		return (size_t)-1;
	}
	ssize_t result = instruction_pattern_get_length(pattern, ins);
	if(result < 0)
	{
		report_errors(ins->line_number, 1 << -result);
		return (size_t)-1;
	}
	return result;
}

void x80_generate_instruction(instruction_t * ins)
{
	const instruction_pattern_t * pattern = find_pattern(ins, false, NULL);

//if(!pattern) { fprintf(stderr, "fail\n"); return; }

	instruction_pattern_generate(pattern, ins);
}

#ifdef TARGET_X86
# define CPU_ANY CPU_8080
# define CPU_ALL CPU_Z80
# define CPU_NOT_GBZ80 CPU_Z80
# define CPU_8008 CPU_8080
#else
# define CPU_ANY CPU_DP2200
# define CPU_ALL CPU_GBZ80
# define CPU_NOT_8080 (CPU_8080 - 1)
# define CPU_NOT_GBZ80 (CPU_GBZ80 - 1)
#endif

static const instruction_pattern_t pattern_i8080_aci_1[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_I8 },
		{ ACT_PUT, 0xCE, ACT_I8_0 },
		_MCS8({ ACT_PUT, 0x0C, ACT_I8_0 })
	},
};

static const instruction_pattern_t pattern_i8080_adc_1[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_IR8 },
		{ ACT_ADD_R0, 0x88 },
		_MCS8({ ACT_ADD_R0, 0x88 })
	},
};

static const instruction_pattern_t pattern_i8080_add_1[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_IR8 },
		{ ACT_ADD_R0, 0x80 },
		_MCS8({ ACT_ADD_R0, 0x80 })
	},
};

static const instruction_pattern_t pattern_i8080_adi_1[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_I8 },
		{ ACT_PUT, 0xC6, ACT_I8_0 },
		_MCS8({ ACT_PUT, 0x04, ACT_I8_0 })
	},
};

static const instruction_pattern_t pattern_i8080_ana_1[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_IR8 },
		{ ACT_ADD_R0, 0xA0 },
		_MCS8({ ACT_ADD_R0, 0xA0 })
	},
};

static const instruction_pattern_t pattern_i8080_ani_1[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_I8 },
		{ ACT_PUT, 0xE6, ACT_I8_0 },
		_MCS8({ ACT_PUT, 0x24, ACT_I8_0 })
	},
};

#if TARGET_X80
static const instruction_pattern_t pattern_i8080_arhl_0[] =
{
	{
		CPU_8085, CPU_8085,
		{ },
		{ ACT_PUT, 0x10 },
	},
};
#endif

static const instruction_pattern_t pattern_i8080_c_cc_1[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_UI16 },
		{ ACT_ADD_COND, 0xC4, ACT_I16_0 },
		_MCS8({ ACT_ADD_COND, 0x42, ACT_I16_0 })
	},
};

static const instruction_pattern_t pattern_i8080_call_1[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_UI16 },
		{ ACT_PUT, 0xCD, ACT_I16_0 },
		_MCS8({ ACT_PUT, 0x46, ACT_I16_0 })
	},
};

static const instruction_pattern_t pattern_i8080_cma_0[] =
{
	{
		CPU_8080, CPU_ALL,
		{ },
		{ ACT_PUT, 0x2F },
	},
};

static const instruction_pattern_t pattern_i8080_cmc_0[] =
{
	{
		CPU_8080, CPU_ALL,
		{ },
		{ ACT_PUT, 0x3F },
	},
};

static const instruction_pattern_t pattern_i8080_cmp_1[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_IR8 },
		{ ACT_ADD_R0, 0xB8 },
		_MCS8({ ACT_ADD_R0, 0xB8 })
	},
};

static const instruction_pattern_t pattern_i8080_cpi_1[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_I8 },
		{ ACT_PUT, 0xFE, ACT_I8_0 },
		_MCS8({ ACT_PUT, 0x3C, ACT_I8_0 })
	},
};

static const instruction_pattern_t pattern_i8080_daa_0[] =
{
	{
		CPU_8080, CPU_ALL,
		{ },
		{ ACT_PUT, 0x27 },
	},
};

static const instruction_pattern_t pattern_i8080_dad_1[] =
{
	{
		CPU_8080, CPU_ALL,
		{ CST_IR16 },
		{ ACT_ADD_R0S3, 0x09 },
	},
};

static const instruction_pattern_t pattern_i8080_dcr_1[] =
{
#if TARGET_X80
	{
		CPU_8008, CPU_8008,
		{ CST_IR8_NOTA },
		{ },
		{ ACT_ADD_R0S3, 0x01 },
	},
#endif
	{
		CPU_8080, CPU_ALL,
		{ CST_IR8 },
		{ ACT_ADD_R0S3, 0x05 },
		_MCS8({ ACT_ADD_R0S3, 0x00 })
	},
};


static const instruction_pattern_t pattern_i8080_dcx_1[] =
{
	{
		CPU_8080, CPU_ALL,
		{ CST_IR16 },
		{ ACT_ADD_R0S3, 0x0B },
	},
};

static const instruction_pattern_t pattern_i8080_di_0[] =
{
#if TARGET_X80
	{
		CPU_DP2200V2, CPU_DP2200V2,
		{ },
		{ },
		{ ACT_PUT, 0x20 },
	},
#endif
	{
		CPU_8080, CPU_ALL,
		{ },
		{ ACT_PUT, 0xF3 },
	},
};

#if TARGET_X80
static const instruction_pattern_t pattern_i8080_dsub_0[] =
{
	{
		CPU_8085, CPU_8085,
		{ },
		{ ACT_PUT, 0x08 },
	},
};
#endif

static const instruction_pattern_t pattern_i8080_ei_0[] =
{
#if TARGET_X80
	{
		CPU_DP2200V2, CPU_DP2200V2,
		{ },
		{ },
		{ ACT_PUT, 0x28 },
	},
#endif
	{
		CPU_8080, CPU_ALL,
		{ },
		{ ACT_PUT, 0xFB },
	},
};

static const instruction_pattern_t pattern_i8080_hlt_0[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ },
		{ ACT_PUT, 0x76 },
		_MCS8({ ACT_PUT, 0xFF })
	},
};

static const instruction_pattern_t pattern_i8080_in_1[] =
{
#if TARGET_X80
	{
		CPU_8008, CPU_8008,
		{ CST_UI3 },
		{ },
		{ ACT_ADD_I0S1, 0x41 },
	},
#endif
	{
		CPU_8080, CPU_NOT_GBZ80,
		{ CST_UI8 },
		{ ACT_PUT, 0xDB, ACT_I8_0 },
	},
};

static const instruction_pattern_t pattern_i8080_inr_1[] =
{
#if TARGET_X80
	{
		CPU_8008, CPU_8008,
		{ CST_IR8_NOTA },
		{ },
		{ ACT_ADD_R0S3, 0x00 },
	},
#endif
	{
		CPU_8080, CPU_ALL,
		{ CST_IR8 },
		{ ACT_ADD_R0S3, 0x04 },
	},
};

static const instruction_pattern_t pattern_i8080_inx_1[] =
{
	{
		CPU_8080, CPU_ALL,
		{ CST_IR16 },
		{ ACT_ADD_R0S3, 0x03 },
	},
};

static const instruction_pattern_t pattern_i8080_j_cc_1[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_UI16 },
		{ ACT_ADD_COND, 0xC2, ACT_I16_0 },
		_MCS8({ ACT_ADD_COND, 0x40, ACT_I16_0 })
	},
};

static const instruction_pattern_t pattern_i8080_jmp_1[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_UI16 },
		{ ACT_PUT, 0xC3, ACT_I16_0 },
		_MCS8({ ACT_PUT, 0x44, ACT_I16_0 })
	},
};

#if TARGET_X80
static const instruction_pattern_t pattern_i8080_jnui_1[] =
{
	{
		CPU_8085, CPU_8085,
		{ CST_UI16 },
		{ ACT_PUT, 0xDD, ACT_I16_0 },
	},
};

static const instruction_pattern_t pattern_i8080_jui_1[] =
{
	{
		CPU_8085, CPU_8085,
		{ CST_UI16 },
		{ ACT_PUT, 0xFD, ACT_I16_0 },
	},
};
#endif

static const instruction_pattern_t pattern_i8080_lda_1[] =
{
	{
		CPU_8080, CPU_NOT_GBZ80,
		{ CST_UI16 },
		{ ACT_PUT, 0x3A, ACT_I16_0 },
	},
};

static const instruction_pattern_t pattern_i8080_ldax_1[] =
{
	{
		CPU_8080, CPU_ALL,
		{ CST_IMR16 },
		{ ACT_ADD_R0S3, 0x0A },
	},
};

#if TARGET_X80
static const instruction_pattern_t pattern_i8080_ldhi_1[] =
{
	{
		CPU_8085, CPU_8085,
		{ CST_UI8 },
		{ ACT_PUT, 0x28, ACT_I8_0 },
	},
};

static const instruction_pattern_t pattern_i8080_ldsi_1[] =
{
	{
		CPU_8085, CPU_8085,
		{ CST_UI8 },
		{ ACT_PUT, 0x38, ACT_I8_0 },
	},
};
#endif

static const instruction_pattern_t pattern_i8080_lhld_1[] =
{
	{
		CPU_8080, CPU_NOT_GBZ80,
		{ CST_UI16 },
		{ ACT_PUT, 0x2A, ACT_I16_0 },
	},
};

#if TARGET_X80
static const instruction_pattern_t pattern_i8080_lhlx_0[] =
{
	{
		CPU_8085, CPU_8085,
		{ },
		{ ACT_PUT, 0xED },
	},
};
#endif

static const instruction_pattern_t pattern_i8080_lxi_2[] =
{
	{
		CPU_8080, CPU_ALL,
		{ CST_IR16, CST_I16 },
		{ ACT_ADD_R0S3, 0x01, ACT_I16_1 },
	},
};

static const instruction_pattern_t pattern_i8080_mov_2[] =
{
#if TARGET_X80
	{
		CPU_ANY, CPU_NOT_8080,
		{ CST_IR8, CST_IR8_NOT_SAME },
		{ ACT_ADD_R0R1, 0x40 },
		{ ACT_ADD_R0R1, 0xC0 },
	},
#endif
	{
		CPU_8080, CPU_ALL,
		{ CST_IR8, CST_IR8_NOT_BOTH_M },
		{ ACT_ADD_R0R1, 0x40 },
	},
};


static const instruction_pattern_t pattern_i8080_mvi_2[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_IR8, CST_I8 },
		{ ACT_ADD_R0S3, 0x06, ACT_I8_1 },
		_MCS8({ ACT_ADD_R0S3, 0x06, ACT_I8_1 })
	},
};

static const instruction_pattern_t pattern_i8080_nop_0[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ },
		{ ACT_PUT, 0x0 },
		_MCS8({ ACT_PUT, 0xC0 })
	},
};

static const instruction_pattern_t pattern_i8080_ora_1[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_IR8 },
		{ ACT_ADD_R0, 0xB0 },
		_MCS8({ ACT_ADD_R0, 0xB0 })
	},
};

static const instruction_pattern_t pattern_i8080_ori_1[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_I8 },
		{ ACT_PUT, 0xF6, ACT_I8_0 },
		_MCS8({ ACT_PUT, 0x34, ACT_I8_0 })
	},
};

static const instruction_pattern_t pattern_i8080_out_1[] =
{
#if TARGET_X80
	{
		CPU_8008, CPU_8008,
		{ CST_UI5_GE8 },
		{ },
		{ ACT_ADD_I0S1, 0x41 },
	},
#endif
	{
		CPU_8080, CPU_NOT_GBZ80,
		{ CST_UI8 },
		{ ACT_PUT, 0xD3, ACT_I8_0 },
	},
};

static const instruction_pattern_t pattern_i8080_pchl_0[] =
{
	{
		CPU_8080, CPU_ALL,
		{ },
		{ ACT_PUT, 0xE9 },
	},
};

static const instruction_pattern_t pattern_i8080_pop_1[] =
{
#if TARGET_X80
	{
		CPU_DP2200V2, CPU_DP2200V2,
		{ CST_HREG },
		{ },
		{ ACT_PUT, 0x30 },
	},
#endif
	{
		CPU_8080, CPU_ALL,
		{ CST_IR16_NOSP },
		{ ACT_ADD_R0S3, 0xC1 },
	},
	{
		CPU_8080, CPU_ALL,
		{ CST_PSWREG },
		{ ACT_PUT, 0xF1 },
	},
};

static const instruction_pattern_t pattern_i8080_push_1[] =
{
#if TARGET_X80
	{
		CPU_DP2200V2, CPU_DP2200V2,
		{ CST_HREG },
		{ },
		{ ACT_PUT, 0x38 },
	},
#endif
	{
		CPU_ANY, CPU_ALL,
		{ CST_IR16_NOSP },
		{ ACT_ADD_R0S3, 0xC5 },
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_PSWREG },
		{ ACT_PUT, 0xF5 },
	},
};

static const instruction_pattern_t pattern_i8080_ral_0[] =
{
	{
		CPU_8008, CPU_ALL,
		{ },
		{ ACT_PUT, 0x17 },
		_MCS8({ ACT_PUT, 0x12 })
	},
};

static const instruction_pattern_t pattern_i8080_rar_0[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ },
		{ ACT_PUT, 0x1F },
		_MCS8({ ACT_PUT, 0x1A })
	},
};

static const instruction_pattern_t pattern_i8080_r_cc_0[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ },
		{ ACT_ADD_COND, 0xC0 },
		_MCS8({ ACT_ADD_COND, 0x03 })
	},
};

#if TARGET_X80
static const instruction_pattern_t pattern_i8080_rdel_0[] =
{
	{
		CPU_8085, CPU_8085,
		{ },
		{ ACT_PUT, 0x18 },
	},
};
#endif

static const instruction_pattern_t pattern_i8080_ret_0[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ },
		{ ACT_PUT, 0xC9 },
		_MCS8({ ACT_PUT, 0x07 })
	},
};

#if TARGET_X80
static const instruction_pattern_t pattern_i8080_rim_0[] =
{
	{
		CPU_8085, CPU_8085,
		{ },
		{ ACT_PUT, 0x20 },
	},
};
#endif

static const instruction_pattern_t pattern_i8080_rlc_0[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ },
		{ ACT_PUT, 0x07 },
		_MCS8({ ACT_PUT, 0x02 })
	},
};

static const instruction_pattern_t pattern_i8080_rrc_0[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ },
		{ ACT_PUT, 0x0F },
		_MCS8({ ACT_PUT, 0x0A })
	},
};

static const instruction_pattern_t pattern_i8080_rst_1[] =
{
	{
		CPU_8008, CPU_ALL,
		{ CST_UI3 },
		{ ACT_ADD_I0S3, 0xC7 },
		_MCS8({ ACT_ADD_I0S3, 0x05 })
	},
};

#if TARGET_X80
static const instruction_pattern_t pattern_i8080_rstv_0[] =
{
	{
		CPU_8085, CPU_8085,
		{ },
		{ ACT_PUT, 0xCB },
	},
};
#endif

static const instruction_pattern_t pattern_i8080_sbb_1[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_IR8 },
		{ ACT_ADD_R0, 0x98 },
		_MCS8({ ACT_ADD_R0, 0x98 })
	},
};

static const instruction_pattern_t pattern_i8080_sbi_1[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_I8 },
		{ ACT_PUT, 0xDE, ACT_I8_0 },
		_MCS8({ ACT_PUT, 0x2C, ACT_I8_0 })
	},
};

static const instruction_pattern_t pattern_i8080_shld_1[] =
{
	{
		CPU_8080, CPU_NOT_GBZ80,
		{ CST_UI16 },
		{ ACT_PUT, 0x22, ACT_I16_0 },
	},
};

#if TARGET_X80
static const instruction_pattern_t pattern_i8080_shlx_0[] =
{
	{
		CPU_8085, CPU_8085,
		{ },
		{ ACT_PUT, 0xD9 },
	},
};

static const instruction_pattern_t pattern_i8080_sim_0[] =
{
	{
		CPU_8085, CPU_8085,
		{ },
		{ ACT_PUT, 0x30 },
	},
};
#endif

static const instruction_pattern_t pattern_i8080_sphl_0[] =
{
	{
		CPU_8080, CPU_ALL,
		{ },
		{ ACT_PUT, 0xF9 },
	},
};

static const instruction_pattern_t pattern_i8080_sta_1[] =
{
	{
		CPU_8080, CPU_NOT_GBZ80,
		{ CST_UI16 },
		{ ACT_PUT, 0x32, ACT_I16_0 },
	},
};

static const instruction_pattern_t pattern_i8080_stax_1[] =
{
	{
		CPU_8080, CPU_ALL,
		{ CST_IMR16 },
		{ ACT_ADD_R0S3, 0x02 },
	},
};

static const instruction_pattern_t pattern_i8080_stc_0[] =
{
	{
		CPU_8080, CPU_ALL,
		{ },
		{ ACT_PUT, 0x37 },
	},
};

static const instruction_pattern_t pattern_i8080_sub_1[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_IR8 },
		{ ACT_ADD_R0, 0x90 },
		_MCS8({ ACT_ADD_R0, 0x90 })
	},
};

static const instruction_pattern_t pattern_i8080_sui_1[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_I8 },
		{ ACT_PUT, 0xD6, ACT_I8_0 },
		_MCS8({ ACT_PUT, 0x14, ACT_I8_0 })
	},
};

static const instruction_pattern_t pattern_i8080_xchg_0[] =
{
	{
		CPU_8080, CPU_NOT_GBZ80,
		{ },
		{ ACT_PUT, 0xEB },
	},
};

static const instruction_pattern_t pattern_i8080_xra_1[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_IR8 },
		{ ACT_ADD_R0, 0xA8 },
		_MCS8({ ACT_ADD_R0, 0xA8 })
	},
};

static const instruction_pattern_t pattern_i8080_xri_1[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_I8 },
		{ ACT_PUT, 0xEE, ACT_I8_0 },
		_MCS8({ ACT_PUT, 0x2C, ACT_I8_0 })
	},
};

static const instruction_pattern_t pattern_i8080_xthl_0[] =
{
	{
		CPU_8080, CPU_NOT_GBZ80,
		{ },
		{ ACT_PUT, 0xE3 },
	},
};

static const instruction_pattern_t pattern_z80_adc_2[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_AREG, CST_ZR8 },
		{ ACT_ADD_R1, 0x88 },
		_MCS8({ ACT_ADD_R0, 0x88 })
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_AREG, CST_ZM },
		{ ACT_PUT, 0x8E },
		_MCS8({ ACT_PUT, 0x8F })
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_AREG, CST_I8 },
		{ ACT_PUT, 0xCE, ACT_I8_1 },
		_MCS8({ ACT_PUT, 0x0C, ACT_I8_0 })
	},

	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_HLREG, CST_ZR16_NOPREF },
		{ ACT_PUT, 0xED, ACT_ADD_R1S3, 0x4A },
	},
};

static const instruction_pattern_t pattern_z80_add_2[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_AREG, CST_ZR8 },
		{ ACT_ADD_R1, 0x80 },
		_MCS8({ ACT_ADD_R0, 0x80 })
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_AREG, CST_ZM },
		{ ACT_PUT, 0x86 },
		_MCS8({ ACT_PUT, 0x87 })
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_AREG, CST_I8 },
		{ ACT_PUT, 0xC6, ACT_I8_1 },
		_MCS8({ ACT_PUT, 0x04, ACT_I8_0 })
	},

	{
		CPU_8080, CPU_ALL,
		{ CST_ZIDX, CST_ZR16 },
		{ ACT_ADD_R1S3, 0x09 },
	},

#if TARGET_X80
	{
		CPU_GBZ80, CPU_GBZ80,
		{ CST_SPREG, CST_SI8 },
		{ ACT_PUT, 0xE8, ACT_I8_1 },
	},
#endif
};

static const instruction_pattern_t pattern_z80_and_1[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_ZR8 },
		{ ACT_ADD_R0, 0xA0 },
		_MCS8({ ACT_ADD_R0, 0xA0 })
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_ZM },
		{ ACT_PUT, 0xA6 },
		_MCS8({ ACT_PUT, 0xA7 })
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_I8 },
		{ ACT_PUT, 0xE6, ACT_I8_0 },
		_MCS8({ ACT_PUT, 0x24, ACT_I8_0 })
	},
};

// extension
static const instruction_pattern_t pattern_z80_and_2[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_AREG, CST_ZR8 },
		{ ACT_ADD_R1, 0xA0 },
		_MCS8({ ACT_ADD_R1, 0xA0 })
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_AREG, CST_ZM },
		{ ACT_PUT, 0xA6 },
		_MCS8({ ACT_PUT, 0xA7 })
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_AREG, CST_I8 },
		{ ACT_PUT, 0xE6, ACT_I8_1 },
		_MCS8({ ACT_PUT, 0x24, ACT_I8_1 })
	},
};

static const instruction_pattern_t pattern_z80_bit_2[] =
{
	{
		CPU_Z80, CPU_ALL,
		{ CST_UI3, CST_ZR8_NOPREF },
		{ ACT_PUT, 0xCB, ACT_ADD_I0R1, 0x40 },
	},
	{
		CPU_Z80, CPU_ALL,
		{ CST_UI3, CST_ZM },
		{ ACT_PUT, 0xCB, ACT_ADD_I0S3, 0x46 },
	},
};

static const instruction_pattern_t pattern_z80_call_1[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_UI16 },
		{ ACT_PUT, 0xCD, ACT_I16_0 },
		_MCS8({ ACT_PUT, 0x46, ACT_I16_0 })
	},
};

static const instruction_pattern_t pattern_z80_call_2[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_COND, CST_UI16 },
		{ ACT_ADD_COND0S3, 0xC4, ACT_I16_1 },
		_MCS8({ ACT_ADD_COND0S3, 0x42, ACT_I16_0 })
	},
};

static const instruction_pattern_t pattern_z80_cp_1[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_ZR8 },
		{ ACT_ADD_R0, 0xB8 },
		_MCS8({ ACT_ADD_R0, 0xB8 })
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_ZM },
		{ ACT_PUT, 0xBE },
		_MCS8({ ACT_PUT, 0xBF })
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_I8 },
		{ ACT_PUT, 0xFE, ACT_I8_0 },
		_MCS8({ ACT_PUT, 0x3C, ACT_I8_0 })
	},
};

// extension
static const instruction_pattern_t pattern_z80_cp_2[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_AREG, CST_ZR8 },
		{ ACT_ADD_R1, 0xB8 },
		_MCS8({ ACT_ADD_R1, 0xB8 })
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_AREG, CST_ZM },
		{ ACT_PUT, 0xBE },
		_MCS8({ ACT_PUT, 0xBF })
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_AREG, CST_I8 },
		{ ACT_PUT, 0xFE, ACT_I8_1 },
		_MCS8({ ACT_PUT, 0x3C, ACT_I8_0 })
	},
};

static const instruction_pattern_t pattern_z80_cpd_0[] =
{
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ },
		{ ACT_PUT2, 0xED, 0xA9 },
	},
};

static const instruction_pattern_t pattern_z80_cpdr_0[] =
{
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ },
		{ ACT_PUT2, 0xED, 0xB9 },
	},
};

static const instruction_pattern_t pattern_z80_cpi_0[] =
{
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ },
		{ ACT_PUT2, 0xED, 0xA1 },
	},
};

static const instruction_pattern_t pattern_z80_cpir_0[] =
{
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ },
		{ ACT_PUT2, 0xED, 0xB1 },
	},
};

static const instruction_pattern_t pattern_z80_dec_1[] =
{
#if TARGET_X80
	{
		CPU_8008, CPU_8008,
		{ CST_ZR8_NOTA },
		{ },
		{ ACT_ADD_R0S3, 0x01 },
	},
#endif
	{
		CPU_8080, CPU_ALL,
		{ CST_ZR8 },
		{ ACT_ADD_R0S3, 0x05 },
	},
	{
		CPU_8008, CPU_ALL,
		{ CST_ZM },
		{ ACT_PUT, 0x35 },
		_MCS8({ ACT_ADD_R0S3, 0x39 })
	},

	{
		CPU_8080, CPU_ALL,
		{ CST_ZR16 },
		{ ACT_ADD_R0S3, 0x0B },
	},
};

static const instruction_pattern_t pattern_z80_djnz_1[] =
{
	{
		CPU_Z80, CPU_ALL,
		{ CST_REL8 },
		{ ACT_PUT, 0x10, ACT_R8_0 },
	},
};

static const instruction_pattern_t pattern_z80_ex_2[] =
{
	{
		CPU_ANY, CPU_NOT_GBZ80,
		{ CST_DEREG, CST_HLREG },
		{ ACT_PUT, 0xEB },
	},
	{
		CPU_ANY, CPU_NOT_GBZ80,
		{ CST_HLREG, CST_DEREG },
		{ ACT_PUT, 0xEB },
	},
	{
		CPU_ANY, CPU_NOT_GBZ80,
		{ CST_SPMEM, CST_ZIDX },
		{ ACT_PUT, 0xE3 },
	},
	{
		CPU_ANY, CPU_NOT_GBZ80,
		{ CST_ZIDX, CST_SPMEM },
		{ ACT_PUT, 0xE3 },
	},
	{
		CPU_Z80, CPU_ALL,
		{ CST_AFREG, CST_AF2REG },
		{ ACT_PUT, 0x08 },
	},
	{
		CPU_Z80, CPU_ALL,
		{ CST_AF2REG, CST_AFREG },
		{ ACT_PUT, 0x08 },
	},
};

static const instruction_pattern_t pattern_z80_exx_0[] =
{
	{
		CPU_Z80, CPU_ALL,
		{ },
		{ ACT_PUT, 0xD9 },
	},
};

static const instruction_pattern_t pattern_z80_im_1[] =
{
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_EQ0 },
		{ ACT_PUT2, 0xED, 0x46 },
	},
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_EQ1 },
		{ ACT_PUT2, 0xED, 0x56 },
	},
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_EQ2 },
		{ ACT_PUT2, 0xED, 0x5E },
	},
};

static const instruction_pattern_t pattern_z80_in_1[] =
{
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_CMEM },
		{ ACT_PUT2, 0xED, 0x70 },
	},
};

static const instruction_pattern_t pattern_z80_in_2[] =
{
#if TARGET_X80
	{
		CPU_8008, CPU_8008,
		{ CST_AREG, CST_ZMI3 },
		{ },
		{ ACT_ADD_I1S1, 0x41 },
	},
#endif
	{
		CPU_8080, CPU_NOT_GBZ80,
		{ CST_AREG, CST_ZMI8 },
		{ ACT_PUT, 0xDB, ACT_I8_1 },
	},
	{
		CPU_8080, CPU_NOT_GBZ80,
		{ CST_ZR8_NOPREF, CST_CMEM },
		{ ACT_PUT, 0xED, ACT_ADD_R0S3, 0x40 },
	},
};

static const instruction_pattern_t pattern_z80_inc_1[] =
{
#if TARGET_X80
	{
		CPU_8008, CPU_8008,
		{ CST_ZR8_NOTA },
		{ },
		{ ACT_ADD_R0S3, 0x00 },
	},
#endif
	{
		CPU_8080, CPU_ALL,
		{ CST_ZR8 },
		{ ACT_ADD_R0S3, 0x04 },
	},
	{
		CPU_8008, CPU_ALL,
		{ CST_ZM },
		{ ACT_PUT, 0x34 },
		_MCS8({ ACT_PUT, 0x38 })
	},

	{
		CPU_8080, CPU_ALL,
		{ CST_ZR16 },
		{ ACT_ADD_R0S3, 0x03 },
	},
};

static const instruction_pattern_t pattern_z80_ind_0[] =
{
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ },
		{ ACT_PUT2, 0xED, 0xAA },
	},
};

static const instruction_pattern_t pattern_z80_indr_0[] =
{
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ },
		{ ACT_PUT2, 0xED, 0xBA },
	},
};

static const instruction_pattern_t pattern_z80_ini_0[] =
{
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ },
		{ ACT_PUT2, 0xED, 0xA2 },
	},
};

static const instruction_pattern_t pattern_z80_inir_0[] =
{
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ },
		{ ACT_PUT2, 0xED, 0xB2 },
	},
};

static const instruction_pattern_t pattern_z80_jp_1[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_UI16 },
		{ ACT_PUT, 0xC3, ACT_I16_0 },
		_MCS8({ ACT_PUT, 0x44, ACT_I16_0 })
	},

	{
		CPU_8080, CPU_ALL,
		{ CST_ZMIDX },
		{ ACT_PUT, 0xE9 },
	},
};

static const instruction_pattern_t pattern_z80_jp_2[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_COND, CST_UI16 },
		{ ACT_ADD_COND0S3, 0xC2, ACT_I16_1 },
		_MCS8({ ACT_ADD_COND0S3, 0x40, ACT_I16_1 })
	},
};

static const instruction_pattern_t pattern_z80_jr_1[] =
{
	{
		CPU_Z80, CPU_ALL,
		{ CST_REL8 },
		{ ACT_PUT, 0x18, ACT_R8_0 },
	},
};

static const instruction_pattern_t pattern_z80_jr_2[] =
{
	{
		CPU_Z80, CPU_ALL,
		{ CST_COND2, CST_REL8 },
		{ ACT_ADD_COND0S3, 0x20, ACT_R8_1 },
	},
};

static const instruction_pattern_t pattern_z80_ld_2[] =
{
#if TARGET_X80
	{
		CPU_ANY, CPU_NOT_8080,
		{ CST_ZR8, CST_ZR8_NOT_SAME },
		{ ACT_ADD_R0R1, 0x40 },
		{ ACT_ADD_R0R1, 0xC0 },
	},
#endif
	{
		CPU_8080, CPU_ALL,
		{ CST_ZR8, CST_ZR8 },
		{ ACT_ADD_R0R1, 0x40 },
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_ZR8_NOPREF, CST_ZM },
		{ ACT_ADD_R0S3, 0x46 },
		_MCS8({ ACT_ADD_R0S3, 0xC7 })
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_ZM, CST_ZR8_NOPREF },
		{ ACT_ADD_R1, 0x70, },
		_MCS8({ ACT_ADD_R0S3, 0xF8 })
	},

	{
		CPU_8080, CPU_ALL,
		{ CST_AREG, CST_ZMR16 },
		{ ACT_ADD_R1S3, 0x0A },
	},
	{
		CPU_8080, CPU_ALL,
		{ CST_ZMR16, CST_AREG },
		{ ACT_ADD_R0S3, 0x02 },
	},
#if TARGET_X80
	{
		CPU_GBZ80, CPU_GBZ80,
		{ CST_ZMI8_HIGH, CST_AREG },
		{ ACT_PUT, 0xE0, ACT_I8_0 },
	},
	{
		CPU_GBZ80, CPU_GBZ80,
		{ CST_AREG, CST_ZMI8_HIGH },
		{ ACT_PUT, 0xF0, ACT_I8_1 },
	},
	{
		CPU_GBZ80, CPU_GBZ80,
		{ CST_ZMI16, CST_AREG },
		{ ACT_PUT, 0xEA, ACT_I16_0 },
	},
	{
		CPU_GBZ80, CPU_GBZ80,
		{ CST_AREG, CST_ZMI16 },
		{ ACT_PUT, 0xFA, ACT_I16_1 },
	},
#endif
	{
		CPU_8080, CPU_NOT_GBZ80,
		{ CST_AREG, CST_ZMI16 },
		{ ACT_PUT, 0x3A, ACT_I16_1 },
	},
	{
		CPU_8080, CPU_NOT_GBZ80,
		{ CST_ZMI16, CST_AREG },
		{ ACT_PUT, 0x32, ACT_I16_0 },
	},

	{
		CPU_ANY, CPU_ALL,
		{ CST_ZR8, CST_I8 },
		{ ACT_ADD_R0S3, 0x06, ACT_I8_1 },
		_MCS8({ ACT_ADD_R0S3, 0x06, ACT_I8_1 })
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_ZM, CST_I8 },
		{ ACT_PUT, 0x36, ACT_I8_1 },
		_MCS8({ ACT_PUT, 0x7E, ACT_I8_1 })
	},

	{
		CPU_8080, CPU_ALL,
		{ CST_ZR16, CST_I16 },
		{ ACT_ADD_R0S3, 0x01, ACT_I16_1 },
	},

	{
		CPU_8080, CPU_NOT_GBZ80,
		{ CST_ZIDX, CST_ZMI16 },
		{ ACT_PUT, 0x2A, ACT_I16_1 },
	},
	{
		CPU_8080, CPU_NOT_GBZ80,
		{ CST_ZR16_NOPREF, CST_ZMI16 },
		{ ACT_PUT, 0xED, ACT_ADD_R0S3, 0x4B, ACT_I16_1 },
	},

	{
		CPU_8080, CPU_NOT_GBZ80,
		{ CST_ZMI16, CST_ZIDX },
		{ ACT_PUT, 0x22, ACT_I16_0 },
	},
	{
		CPU_8080, CPU_NOT_GBZ80,
		{ CST_ZMI16, CST_ZR16_NOPREF },
		{ ACT_PUT, 0xED, ACT_ADD_R1S3, 0x43, ACT_I16_0 },
	},

	{
		CPU_8080, CPU_ALL,
		{ CST_SPREG, CST_ZIDX },
		{ ACT_PUT, 0xF9 },
	},

	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_IREG, CST_AREG },
		{ ACT_PUT2, 0xED, 0x47 },
	},
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_RREG, CST_AREG },
		{ ACT_PUT2, 0xED, 0x4F },
	},
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_AREG, CST_IREG },
		{ ACT_PUT2, 0xED, 0x57 },
	},
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_AREG, CST_RREG },
		{ ACT_PUT2, 0xED, 0x5F },
	},

#if TARGET_X80
	{
		CPU_GBZ80, CPU_GBZ80,
		{ CST_CMEM, CST_AREG },
		{ ACT_PUT, 0xE2 },
	},
	{
		CPU_GBZ80, CPU_GBZ80,
		{ CST_AREG, CST_CMEM },
		{ ACT_PUT, 0xF2 },
	},
	{
		CPU_GBZ80, CPU_GBZ80,
		{ CST_AREG, CST_ZMHLI },
		{ ACT_PUT, 0x2A },
	},
	{
		CPU_GBZ80, CPU_GBZ80,
		{ CST_AREG, CST_ZMHLD },
		{ ACT_PUT, 0x3A },
	},
	{
		CPU_GBZ80, CPU_GBZ80,
		{ CST_ZMHLI, CST_AREG },
		{ ACT_PUT, 0x22 },
	},
	{
		CPU_GBZ80, CPU_GBZ80,
		{ CST_ZMHLD, CST_AREG },
		{ ACT_PUT, 0x32 },
	},
	{
		CPU_GBZ80, CPU_GBZ80,
		{ CST_ZMI16, CST_SPREG },
		{ ACT_PUT, 0x08, ACT_I16_0 },
	},
#endif
};

static const instruction_pattern_t pattern_z80_ldd_0[] =
{
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ },
		{ ACT_PUT2, 0xED, 0xA8 },
	},
};

static const instruction_pattern_t pattern_z80_lddr_0[] =
{
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ },
		{ ACT_PUT2, 0xED, 0xB8 },
	},
};

static const instruction_pattern_t pattern_z80_ldi_0[] =
{
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ },
		{ ACT_PUT2, 0xED, 0xA0 },
	},
};

static const instruction_pattern_t pattern_z80_ldir_0[] =
{
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ },
		{ ACT_PUT2, 0xED, 0xB0 },
	},
};

#if TARGET_X80
static const instruction_pattern_t pattern_z80_ldhl_2[] =
{
	{
		CPU_GBZ80, CPU_GBZ80,
		{ CST_SPREG, CST_SI8 },
		{ ACT_PUT, 0xF8, ACT_I8_1 },
	},
};
#endif

static const instruction_pattern_t pattern_z80_neg_0[] =
{
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ },
		{ ACT_PUT2, 0xED, 0x44 },
	},
};

static const instruction_pattern_t pattern_z80_or_1[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_ZR8 },
		{ ACT_ADD_R0, 0xB0 },
		_MCS8({ ACT_ADD_R0, 0xB0 })
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_ZM },
		{ ACT_PUT, 0xB6 },
		_MCS8({ ACT_PUT, 0xB7 })
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_I8 },
		{ ACT_PUT, 0xF6, ACT_I8_0 },
		_MCS8({ ACT_PUT, 0x34, ACT_I8_0 })
	},
};

// extension
static const instruction_pattern_t pattern_z80_or_2[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_AREG, CST_ZR8 },
		{ ACT_ADD_R1, 0xB0 },
		_MCS8({ ACT_ADD_R1, 0xB0 })
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_AREG, CST_ZM },
		{ ACT_PUT, 0xB6 },
		_MCS8({ ACT_PUT, 0xB7 })
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_AREG, CST_I8 },
		{ ACT_PUT, 0xF6, ACT_I8_1 },
		_MCS8({ ACT_PUT, 0x34, ACT_I8_0 })
	},
};

static const instruction_pattern_t pattern_z80_out_2[] =
{
#if TARGET_X80
	{
		CPU_8008, CPU_8008,
		{ CST_ZMI5_GE8, CST_AREG },
		{ },
		{ ACT_ADD_I0S1, 0x41 },
	},
#endif
	{
		CPU_8080, CPU_NOT_GBZ80,
		{ CST_ZMI8, CST_AREG },
		{ ACT_PUT, 0xD3, ACT_I8_0 },
	},
	{
		CPU_Z80, CPU_ALL,
		{ CST_CMEM, CST_ZR8_NOPREF },
		{ ACT_PUT, 0xED, ACT_ADD_R1S3, 0x41 },
	},
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_CMEM, CST_EQ0 },
		{ ACT_PUT2, 0xED, 0x71 },
	},
};

static const instruction_pattern_t pattern_z80_outd_0[] =
{
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ },
		{ ACT_PUT2, 0xED, 0xAB },
	},
};

static const instruction_pattern_t pattern_z80_otdr_0[] =
{
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ },
		{ ACT_PUT2, 0xED, 0xBB },
	},
};

static const instruction_pattern_t pattern_z80_outi_0[] =
{
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ },
		{ ACT_PUT2, 0xED, 0xA3 },
	},
};

static const instruction_pattern_t pattern_z80_otir_0[] =
{
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ },
		{ ACT_PUT2, 0xED, 0xB3 },
	},
};

static const instruction_pattern_t pattern_z80_pop_1[] =
{
#if TARGET_X80
	{
		CPU_DP2200V2, CPU_DP2200V2,
		{ CST_HLREG },
		{ },
		{ ACT_PUT, 0x30 },
	},
#endif
	{
		CPU_8080, CPU_ALL,
		{ CST_ZR16_NOSP },
		{ ACT_ADD_R0S3, 0xC1 },
	},
	{
		CPU_8080, CPU_ALL,
		{ CST_AFREG },
		{ ACT_PUT, 0xF1 },
	},
};

static const instruction_pattern_t pattern_z80_push_1[] =
{
#if TARGET_X80
	{
		CPU_DP2200V2, CPU_DP2200V2,
		{ CST_HREG },
		{ },
		{ ACT_PUT, 0x38 },
	},
#endif
	{
		CPU_8080, CPU_ALL,
		{ CST_ZR16_NOSP },
		{ ACT_ADD_R0S3, 0xC5 },
	},
	{
		CPU_8080, CPU_ALL,
		{ CST_AFREG },
		{ ACT_PUT, 0xF5 },
	},
};

static const instruction_pattern_t pattern_z80_res_2[] =
{
	{
		CPU_Z80, CPU_ALL,
		{ CST_UI3, CST_ZR8_NOPREF },
		{ ACT_PUT, 0xCB, ACT_ADD_I0R1, 0x80 },
	},
	{
		CPU_Z80, CPU_ALL,
		{ CST_UI3, CST_ZM },
		{ ACT_PUT, 0xCB, ACT_ADD_I0S3, 0x86 },
	},
};

static const instruction_pattern_t pattern_z80_res_3[] =
{
	{
		CPU_Z80, CPU_ALL,
		{ CST_UI3, CST_ZM_NOHL, CST_ZR8_NOPREF },
		{ ACT_PUT, 0xCB, ACT_ADD_I0R2, 0x80 },
	},
};

static const instruction_pattern_t pattern_z80_ret_0[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ },
		{ ACT_PUT, 0xC9 },
		_MCS8({ ACT_PUT, 0x07 })
	},
};

static const instruction_pattern_t pattern_z80_ret_1[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_COND },
		{ ACT_ADD_COND0S3, 0xC0 },
		_MCS8({ ACT_ADD_COND0S3, 0x03 })
	},
};

static const instruction_pattern_t pattern_z80_reti_0[] =
{
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ },
		{ ACT_PUT2, 0xED, 0x4D },
	},
#if TARGET_X80
	{
		CPU_GBZ80, CPU_GBZ80,
		{ },
		{ ACT_PUT, 0xD9 },
	},
#endif
};

static const instruction_pattern_t pattern_z80_retn_0[] =
{
	{
		CPU_ANY, CPU_NOT_GBZ80,
		{ },
		{ ACT_PUT2, 0xED, 0x45 },
	},
};

static const instruction_pattern_t pattern_z80_rl_1[] =
{
	{
		CPU_Z80, CPU_ALL,
		{ CST_ZR8_NOPREF },
		{ ACT_PUT, 0xCB, ACT_ADD_R0, 0x10 },
	},
	{
		CPU_Z80, CPU_ALL,
		{ CST_ZM },
		{ ACT_PUT2, 0xCB, 0x16 },
	},
};

static const instruction_pattern_t pattern_z80_rl_2[] =
{
	{
		CPU_Z80, CPU_ALL,
		{ CST_ZM_NOHL, CST_ZR8_NOPREF },
		{ ACT_PUT, 0xCB, ACT_ADD_R1, 0x10 },
	},
};

static const instruction_pattern_t pattern_z80_rlc_1[] =
{
	{
		CPU_Z80, CPU_ALL,
		{ CST_ZR8_NOPREF },
		{ ACT_PUT, 0xCB, ACT_ADD_R0, 0x00 },
	},
	{
		CPU_Z80, CPU_ALL,
		{ CST_ZM },
		{ ACT_PUT2, 0xCB, 0x06 },
	},
};

static const instruction_pattern_t pattern_z80_rlc_2[] =
{
	{
		CPU_Z80, CPU_ALL,
		{ CST_ZM_NOHL, CST_ZR8_NOPREF },
		{ ACT_PUT, 0xCB, ACT_ADD_R1, 0x00 },
	},
};

static const instruction_pattern_t pattern_z80_rld_0[] =
{
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ },
		{ ACT_PUT2, 0xED, 0x6F },
	},
};

static const instruction_pattern_t pattern_z80_rr_1[] =
{
	{
		CPU_Z80, CPU_ALL,
		{ CST_ZR8_NOPREF },
		{ ACT_PUT, 0xCB, ACT_ADD_R0, 0x18 },
	},
	{
		CPU_Z80, CPU_ALL,
		{ CST_ZM },
		{ ACT_PUT2, 0xCB, 0x1E },
	},
};

static const instruction_pattern_t pattern_z80_rr_2[] =
{
	{
		CPU_Z80, CPU_ALL,
		{ CST_ZM_NOHL, CST_ZR8_NOPREF },
		{ ACT_PUT, 0xCB, ACT_ADD_R1, 0x18 },
	},
};

static const instruction_pattern_t pattern_z80_rrc_1[] =
{
	{
		CPU_Z80, CPU_ALL,
		{ CST_ZR8_NOPREF },
		{ ACT_PUT, 0xCB, ACT_ADD_R0, 0x08 },
	},
	{
		CPU_Z80, CPU_ALL,
		{ CST_ZM },
		{ ACT_PUT2, 0xCB, 0x0E },
	},
};

static const instruction_pattern_t pattern_z80_rrc_2[] =
{
	{
		CPU_Z80, CPU_ALL,
		{ CST_ZM_NOHL, CST_ZR8_NOPREF },
		{ ACT_PUT, 0xCB, ACT_ADD_R1, 0x08 },
	},
};

static const instruction_pattern_t pattern_z80_rrd_0[] =
{
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ },
		{ ACT_PUT2, 0xED, 0x67 },
	},
};

static const instruction_pattern_t pattern_z80_rst_1[] =
{
	{
		CPU_8008, CPU_ALL,
		{ CST_RST },
		{ ACT_ADD_I0, 0xC7 },
		_MCS8({ ACT_ADD_I0, 0x05 })
	},
};

static const instruction_pattern_t pattern_z80_sbc_2[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_AREG, CST_ZR8 },
		{ ACT_ADD_R1, 0x98 },
		_MCS8({ ACT_ADD_R1, 0x98 })
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_AREG, CST_ZM },
		{ ACT_PUT, 0x9E },
		_MCS8({ ACT_PUT, 0x9F })
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_AREG, CST_I8 },
		{ ACT_PUT, 0xDE, ACT_I8_1 },
		_MCS8({ ACT_PUT, 0x2C, ACT_I8_0 })
	},

	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_HLREG, CST_ZR16_NOPREF },
		{ ACT_PUT, 0xED, ACT_ADD_R1S3, 0x42 },
	},
};

static const instruction_pattern_t pattern_z80_set_2[] =
{
	{
		CPU_Z80, CPU_ALL,
		{ CST_UI3, CST_ZR8_NOPREF },
		{ ACT_PUT, 0xCB, ACT_ADD_I0R1, 0xC0 },
	},
	{
		CPU_Z80, CPU_ALL,
		{ CST_UI3, CST_ZM },
		{ ACT_PUT, 0xCB, ACT_ADD_I0S3, 0xC6 },
	},
};

static const instruction_pattern_t pattern_z80_set_3[] =
{
	{
		CPU_Z80, CPU_ALL,
		{ CST_UI3, CST_ZM_NOHL, CST_ZR8_NOPREF },
		{ ACT_PUT, 0xCB, ACT_ADD_I0R2, 0xC0 },
	},
};

static const instruction_pattern_t pattern_z80_sla_1[] =
{
	{
		CPU_Z80, CPU_ALL,
		{ CST_ZR8_NOPREF },
		{ ACT_PUT, 0xCB, ACT_ADD_R0, 0x20 },
	},
	{
		CPU_Z80, CPU_ALL,
		{ CST_ZM },
		{ ACT_PUT2, 0xCB, 0x26 },
	},
};

static const instruction_pattern_t pattern_z80_sla_2[] =
{
	{
		CPU_Z80, CPU_ALL,
		{ CST_ZM_NOHL, CST_ZR8_NOPREF },
		{ ACT_PUT, 0xCB, ACT_ADD_R1, 0x20 },
	},
};

static const instruction_pattern_t pattern_z80_sll_1[] =
{
	{
		CPU_Z80, CPU_ALL,
		{ CST_ZR8_NOPREF },
		{ ACT_PUT, 0xCB, ACT_ADD_R0, 0x30 },
	},
	{
		CPU_Z80, CPU_ALL,
		{ CST_ZM },
		{ ACT_PUT2, 0xCB, 0x36 },
	},
};

static const instruction_pattern_t pattern_z80_sll_2[] =
{
	{
		CPU_Z80, CPU_ALL,
		{ CST_ZM_NOHL, CST_ZR8_NOPREF },
		{ ACT_PUT, 0xCB, ACT_ADD_R1, 0x30 },
	},
};

static const instruction_pattern_t pattern_z80_sra_1[] =
{
	{
		CPU_Z80, CPU_ALL,
		{ CST_ZR8_NOPREF },
		{ ACT_PUT, 0xCB, ACT_ADD_R0, 0x28 },
	},
	{
		CPU_Z80, CPU_ALL,
		{ CST_ZM },
		{ ACT_PUT2, 0xCB, 0x2E },
	},
};

static const instruction_pattern_t pattern_z80_sra_2[] =
{
	{
		CPU_Z80, CPU_ALL,
		{ CST_ZM_NOHL, CST_ZR8_NOPREF },
		{ ACT_PUT, 0xCB, ACT_ADD_R1, 0x28 },
	},
};

static const instruction_pattern_t pattern_z80_srl_1[] =
{
	{
		CPU_Z80, CPU_ALL,
		{ CST_ZR8_NOPREF },
		{ ACT_PUT, 0xCB, ACT_ADD_R0, 0x38 },
	},
	{
		CPU_Z80, CPU_ALL,
		{ CST_ZM },
		{ ACT_PUT2, 0xCB, 0x3E },
	},
};

static const instruction_pattern_t pattern_z80_srl_2[] =
{
	{
		CPU_Z80, CPU_ALL,
		{ CST_ZM_NOHL, CST_ZR8_NOPREF },
		{ ACT_PUT, 0xCB, ACT_ADD_R1, 0x38 },
	},
};

#if TARGET_X80
static const instruction_pattern_t pattern_z80_stop_1[] =
{
	{
		CPU_GBZ80, CPU_GBZ80,
		{ CST_UI8},
		{ ACT_PUT, 0x10, ACT_I8_0 },
	},
};
#endif

static const instruction_pattern_t pattern_z80_sub_1[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_ZR8 },
		{ ACT_ADD_R0, 0x90 },
		_MCS8({ ACT_ADD_R0, 0x90 })
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_ZM },
		{ ACT_PUT, 0x96 },
		_MCS8({ ACT_ADD_R0, 0x97 })
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_I8 },
		{ ACT_PUT, 0xD6, ACT_I8_0 },
		_MCS8({ ACT_PUT, 0x14, ACT_I8_0 })
	},
};

// extension
static const instruction_pattern_t pattern_z80_sub_2[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_AREG, CST_ZR8 },
		{ ACT_ADD_R1, 0x90 },
		_MCS8({ ACT_ADD_R1, 0x91 })
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_AREG, CST_ZM },
		{ ACT_PUT, 0x96 },
		_MCS8({ ACT_ADD_R0, 0x97 })
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_AREG, CST_I8 },
		{ ACT_PUT, 0xD6, ACT_I8_1 },
		_MCS8({ ACT_PUT, 0x14, ACT_I8_1 })
	},
};

#if TARGET_X80
static const instruction_pattern_t pattern_z80_swap_1[] =
{
	{
		CPU_Z80, CPU_ALL,
		{ CST_ZR8_NOPREF },
		{ ACT_PUT, 0xCB, ACT_ADD_R0, 0x30 },
	},
	{
		CPU_Z80, CPU_ALL,
		{ CST_ZM },
		{ ACT_PUT2, 0xCB, 0x36 },
	},
};
#endif

static const instruction_pattern_t pattern_z80_xor_1[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_ZR8 },
		{ ACT_ADD_R0, 0xA8 },
		_MCS8({ ACT_ADD_R0, 0xA8 })
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_ZM },
		{ ACT_PUT, 0xAE },
		_MCS8({ ACT_PUT, 0xAF })
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_I8 },
		{ ACT_PUT, 0xEE, ACT_I8_0 },
		_MCS8({ ACT_PUT, 0x2C, ACT_I8_0 })
	},
};

// extension
static const instruction_pattern_t pattern_z80_xor_2[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_AREG, CST_ZR8 },
		{ ACT_ADD_R1, 0xA8 },
		_MCS8({ ACT_ADD_R1, 0xA8 })
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_AREG, CST_ZM },
		{ ACT_PUT, 0xAE },
		_MCS8({ ACT_PUT, 0xAF })
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_AREG, CST_I8 },
		{ ACT_PUT, 0xEE, ACT_I8_1 },
		_MCS8({ ACT_PUT, 0x2C, ACT_I8_1 })
	},
};

#if TARGET_X80
static const instruction_pattern_t pattern_r800_adj_1[] =
{
	{
		CPU_8080, CPU_ALL,
		{ CST_AREG },
		{ ACT_PUT, 0x27 },
	},
};

static const instruction_pattern_t pattern_r800_cmp_2[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_AREG, CST_ZR8 },
		{ ACT_ADD_R1, 0xB8 },
		_MCS8({ ACT_ADD_R1, 0xB8 })
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_AREG, CST_ZM },
		{ ACT_PUT, 0xBE },
		_MCS8({ ACT_PUT, 0xBF })
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_AREG, CST_I8 },
		{ ACT_PUT, 0xFE, ACT_I8_1 },
		_MCS8({ ACT_PUT, 0x3C, ACT_I8_0 })
	},

	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_AREG, CST_ZMHLD },
		{ ACT_PUT2, 0xED, 0xA9 },
	},
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_AREG, CST_ZMHLI },
		{ ACT_PUT2, 0xED, 0xA1 },
	},
};

static const instruction_pattern_t pattern_r800_cmpm_2[] =
{
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_AREG, CST_ZMHLD },
		{ ACT_PUT2, 0xED, 0xB9 },
	},
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_AREG, CST_ZMHLI },
		{ ACT_PUT2, 0xED, 0xB1 },
	},
};

static const instruction_pattern_t pattern_r800_in_2[] =
{
#if TARGET_X80
	{
		CPU_8008, CPU_8008,
		{ CST_AREG, CST_ZMI3 },
		{ },
		{ ACT_ADD_I1S1, 0x41 },
	},
#endif
	{
		CPU_8080, CPU_NOT_GBZ80,
		{ CST_AREG, CST_ZMI8 },
		{ ACT_PUT, 0xDB, ACT_I8_1 },
	},
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_ZR8_NOPREF, CST_CMEM },
		{ ACT_PUT, 0xED, ACT_ADD_R0S3, 0x40 },
	},
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_FREG, CST_CMEM },
		{ ACT_PUT2, 0xED, 0x70 },
	},

	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_ZMHLD, CST_CMEM },
		{ ACT_PUT2, 0xED, 0xAA },
	},
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_ZMHLI, CST_CMEM },
		{ ACT_PUT2, 0xED, 0xA2 },
	},
};

static const instruction_pattern_t pattern_r800_inm_2[] =
{
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_ZMHLD, CST_CMEM },
		{ ACT_PUT2, 0xED, 0xBA },
	},
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_ZMHLI, CST_CMEM },
		{ ACT_PUT2, 0xED, 0xB2 },
	},
};

static const instruction_pattern_t pattern_r800_move_2[] =
{
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_ZMHLD, CST_ZMDED },
		{ ACT_PUT2, 0xED, 0xA8 },
	},
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_ZMHLI, CST_ZMDEI },
		{ ACT_PUT2, 0xED, 0xA0 },
	},
};

static const instruction_pattern_t pattern_r800_movem_2[] =
{
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_ZMHLD, CST_ZMDED },
		{ ACT_PUT2, 0xED, 0xB8 },
	},
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_ZMHLI, CST_ZMDEI },
		{ ACT_PUT2, 0xED, 0xB0 },
	},
};

static const instruction_pattern_t pattern_r800_mulub_2[] =
{
	{
		CPU_R800, CPU_R800,
		{ CST_AREG, CST_ZR8 },
		{ ACT_PUT, 0xED, ACT_ADD_R1S3, 0xC1 },
	},
};

static const instruction_pattern_t pattern_r800_muluw_2[] =
{
	{
		CPU_R800, CPU_NOT_GBZ80,
		{ CST_HLREG, CST_ZR16_NOPREF },
		{ ACT_PUT, 0xED, ACT_ADD_R1S3, 0xC3 },
	},
};

static const instruction_pattern_t pattern_r800_neg_1[] =
{
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_AREG },
		{ ACT_PUT2, 0xED, 0x44 },
	},
};

static const instruction_pattern_t pattern_r800_not_1[] =
{
	{
		CPU_8080, CPU_ALL,
		{ CST_AREG },
		{ ACT_PUT, 0x2F },
	},
};

static const instruction_pattern_t pattern_r800_or_2[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_AREG, CST_ZR8 },
		{ ACT_ADD_R1, 0xB0 },
		_MCS8({ ACT_ADD_R1, 0xB0 })
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_AREG, CST_ZM },
		{ ACT_PUT, 0xB6 },
		_MCS8({ ACT_PUT, 0xB7 })
	},
	{
		CPU_ANY, CPU_ALL,
		{ CST_AREG, CST_I8 },
		{ ACT_PUT, 0xF6, ACT_I8_1 },
		_MCS8({ ACT_PUT, 0x34, ACT_I8_0 })
	},
};

static const instruction_pattern_t pattern_r800_out_2[] =
{
#if TARGET_X80
	{
		CPU_8008, CPU_8008,
		{ CST_ZMI5_GE8, CST_AREG },
		{ },
		{ ACT_ADD_I0S1, 0x41 },
	},
#endif
	{
		CPU_8080, CPU_NOT_GBZ80,
		{ CST_ZMI8, CST_AREG },
		{ ACT_PUT, 0xD3, ACT_I8_0 },
	},
	{
		CPU_Z80, CPU_ALL,
		{ CST_CMEM, CST_ZR8_NOPREF },
		{ ACT_PUT, 0xED, ACT_ADD_R1S3, 0x41 },
	},
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_CMEM, CST_EQ0 },
		{ ACT_PUT2, 0xED, 0x71 },
	},
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_CMEM, CST_ZMHLD },
		{ ACT_PUT2, 0xED, 0xAB },
	},
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_CMEM, CST_ZMHLI },
		{ ACT_PUT2, 0xED, 0xA3 },
	},
};

static const instruction_pattern_t pattern_r800_outm_2[] =
{
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_CMEM, CST_ZMHLD },
		{ ACT_PUT2, 0xED, 0xBB },
	},
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_CMEM, CST_ZMHLI },
		{ ACT_PUT2, 0xED, 0xB3 },
	},
};

static const instruction_pattern_t pattern_r800_rol4_1[] =
{
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_ZMHL },
		{ ACT_PUT2, 0xED, 0x6F },
	},
};

static const instruction_pattern_t pattern_r800_ror4_1[] =
{
	{
		CPU_Z80, CPU_NOT_GBZ80,
		{ CST_ZMHL },
		{ ACT_PUT2, 0xED, 0x67 },
	},
};

static const instruction_pattern_t pattern_r800_short_b_cc_1[] =
{
	{
		CPU_Z80, CPU_ALL,
		{ CST_REL8 },
		{ ACT_ADD_COND, 0x20, ACT_R8_0 },
	},
};

static const instruction_pattern_t pattern_i8008_ac_r_0[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ },
		{ ACT_ADD_REG, 0x88 },
		_MCS8({ ACT_ADD_REG, 0x88 })
	},
};

static const instruction_pattern_t pattern_i8008_ad_r_0[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ },
		{ ACT_ADD_REG, 0x80 },
		_MCS8({ ACT_ADD_REG, 0x80 })
	},
};

static const instruction_pattern_t pattern_i8008_cp_r_0[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ },
		{ ACT_ADD_REG, 0xA0 },
		_MCS8({ ACT_ADD_REG, 0xA0 })
	},
};

static const instruction_pattern_t pattern_i8008_dc_r_0[] =
{
	{
		CPU_8008, CPU_ALL,
		{ CST_IR8_NOTA },
		{ ACT_ADD_REGS3, 0x05 },
		_MCS8({ ACT_ADD_REGS3, 0x01 })
	},
};

static const instruction_pattern_t pattern_i8008_in_r_0[] =
{
	{
		CPU_8008, CPU_ALL,
		{ CST_IR8_NOTA },
		{ ACT_ADD_REGS3, 0x04 },
		_MCS8({ ACT_ADD_REGS3, 0x00 })
	},
};

static const instruction_pattern_t pattern_i8008_l_r_r_0[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ },
		{ ACT_ADD_REGS, 0x40 },
		_MCS8({ ACT_ADD_REGS, 0xC0 })
	},
};

static const instruction_pattern_t pattern_i8008_l_r_i_1[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_I8 },
		{ ACT_ADD_REGS3, 0x06, ACT_I8_0 },
		_MCS8({ ACT_ADD_REGS3, 0x06, ACT_I8_0 })
	},
};

static const instruction_pattern_t pattern_i8008_nd_r_0[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ },
		{ ACT_ADD_REG, 0xA0 },
		_MCS8({ ACT_ADD_REG, 0xA0 })
	},
};

static const instruction_pattern_t pattern_i8008_or_r_0[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ },
		{ ACT_ADD_REG, 0xB0 },
		_MCS8({ ACT_ADD_REG, 0xB0 })
	},
};

static const instruction_pattern_t pattern_i8008_sb_r_0[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ CST_IR8 },
		{ ACT_ADD_REG, 0x98 },
		_MCS8({ ACT_ADD_REG, 0x98 })
	},
};

static const instruction_pattern_t pattern_i8008_su_r_0[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ },
		{ ACT_ADD_REG, 0x90 },
		_MCS8({ ACT_ADD_REG, 0x90 })
	},
};

static const instruction_pattern_t pattern_i8008_xr_r_0[] =
{
	{
		CPU_ANY, CPU_ALL,
		{ },
		{ ACT_ADD_REG, 0xA8 },
		_MCS8({ ACT_ADD_REG, 0xA8 })
	},
};

static const instruction_pattern_t pattern_dp2200_alpha_0[] =
{
	{
		CPU_DP2200V2, CPU_DP2200V2,
		{ },
		{ },
		{ ACT_PUT, 0x18 },
	},
};

static const instruction_pattern_t pattern_dp2200_beta_0[] =
{
	{
		CPU_DP2200V2, CPU_DP2200V2,
		{ },
		{ },
		{ ACT_PUT, 0x10 },
	},
};

static const instruction_pattern_t pattern_dp2200_input_0[] =
{
	{
		CPU_ANY, CPU_NOT_GBZ80,
		{ },
		{ ACT_PUT2, 0xDB, 0x00 },
		{ ACT_PUT, 0x41 },
	},
};

static const instruction_pattern_t pattern_dp2200_ex_0[] =
{
	{
		CPU_ANY, CPU_NOT_GBZ80,
		{ },
		{ ACT_PUT, 0xD3, ACT_VAL },
		{ ACT_ADD_VALS1, 0x41 },
	},
};

static const instruction_pattern_t pattern_dp2200_pop_0[] =
{
	{
		CPU_DP2200V2, CPU_DP2200V2,
		{ },
		{ },
		{ ACT_PUT, 0x30 },
	},
	{
		CPU_8080, CPU_ALL,
		{ },
		{ ACT_PUT, 0xE1 },
	},
};

static const instruction_pattern_t pattern_dp2200_push_0[] =
{
	{
		CPU_DP2200V2, CPU_DP2200V2,
		{ },
		{ },
		{ ACT_PUT, 0x38 },
	},
	{
		CPU_8080, CPU_ALL,
		{ },
		{ ACT_PUT, 0xE5 },
	},
};
#endif // TARGET_X80

// TODO: only if TARGET_X86?
static const instruction_pattern_t pattern_z80_calln_1[] =
{
	{
		CPU_ANY, CPU_Z80, // V30+
		{ CST_UI8 },
		{ ACT_PUT2, 0xED, 0xED, ACT_I8_0 },
	},
};

static const instruction_pattern_t pattern_z80_retem_0[] =
{
	{
		CPU_ANY, CPU_Z80, // V30+
		{ },
		{ ACT_PUT2, 0xED, 0xFD },
	},
};

#define PATTERN(__pattern) { (__pattern), sizeof(__pattern) / sizeof((__pattern)[0]) }
#define NOPATTERN { NULL, 0 }

static const pattern_t x80_patterns[_MNEM_TOTAL] =
{
	[MNEM_I8080_ACI] = { { NOPATTERN, PATTERN(pattern_i8080_aci_1) } },
	[MNEM_I8080_ADC] = { { NOPATTERN, PATTERN(pattern_i8080_adc_1) } },
	[MNEM_I8080_ADD] = { { NOPATTERN, PATTERN(pattern_i8080_add_1) } },
	[MNEM_I8080_ADI] = { { NOPATTERN, PATTERN(pattern_i8080_adi_1) } },
	[MNEM_I8080_ANA] = { { NOPATTERN, PATTERN(pattern_i8080_ana_1) } },
	[MNEM_I8080_ANI] = { { NOPATTERN, PATTERN(pattern_i8080_ani_1) } },
	[MNEM_I8080_C_CC] = { { NOPATTERN, PATTERN(pattern_i8080_c_cc_1) } },
	[MNEM_I8080_CALL] = { { NOPATTERN, PATTERN(pattern_i8080_call_1) } },
	[MNEM_I8080_CMA] = { { PATTERN(pattern_i8080_cma_0) } },
	[MNEM_I8080_CMC] = { { PATTERN(pattern_i8080_cmc_0) } },
	[MNEM_I8080_CMP] = { { NOPATTERN, PATTERN(pattern_i8080_cmp_1) } },
	[MNEM_I8080_CPI] = { { NOPATTERN, PATTERN(pattern_i8080_cpi_1) } },
	[MNEM_I8080_DAA] = { { PATTERN(pattern_i8080_daa_0) } },
	[MNEM_I8080_DAD] = { { NOPATTERN, PATTERN(pattern_i8080_dad_1) } },
	[MNEM_I8080_DCX] = { { NOPATTERN, PATTERN(pattern_i8080_dcx_1) } },
	[MNEM_I8080_DCR] = { { NOPATTERN, PATTERN(pattern_i8080_dcr_1) } },
	[MNEM_I8080_DI] = { { PATTERN(pattern_i8080_di_0) } },
	[MNEM_I8080_EI] = { { PATTERN(pattern_i8080_ei_0) } },
	[MNEM_I8080_HLT] = { { PATTERN(pattern_i8080_hlt_0) } },
	[MNEM_I8080_IN] = { { NOPATTERN, PATTERN(pattern_i8080_in_1) } },
	[MNEM_I8080_INX] = { { NOPATTERN, PATTERN(pattern_i8080_inx_1) } },
	[MNEM_I8080_INR] = { { NOPATTERN, PATTERN(pattern_i8080_inr_1) } },
	[MNEM_I8080_J_CC] = { { NOPATTERN, PATTERN(pattern_i8080_j_cc_1) } },
	[MNEM_I8080_JMP] = { { NOPATTERN, PATTERN(pattern_i8080_jmp_1) } },
	[MNEM_I8080_LDA] = { { NOPATTERN, PATTERN(pattern_i8080_lda_1) } },
	[MNEM_I8080_LDAX] = { { NOPATTERN, PATTERN(pattern_i8080_ldax_1) } },
	[MNEM_I8080_LHLD] = { { NOPATTERN, PATTERN(pattern_i8080_lhld_1) } },
	[MNEM_I8080_LXI] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_i8080_lxi_2) } },
	[MNEM_I8080_MOV] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_i8080_mov_2) } },
	[MNEM_I8080_MVI] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_i8080_mvi_2) } },
	[MNEM_I8080_NOP] = { { PATTERN(pattern_i8080_nop_0) } },
	[MNEM_I8080_ORA] = { { NOPATTERN, PATTERN(pattern_i8080_ora_1) } },
	[MNEM_I8080_ORI] = { { NOPATTERN, PATTERN(pattern_i8080_ori_1) } },
	[MNEM_I8080_OUT] = { { NOPATTERN, PATTERN(pattern_i8080_out_1) } },
	[MNEM_I8080_PCHL] = { { PATTERN(pattern_i8080_pchl_0) } },
	[MNEM_I8080_POP] = { { NOPATTERN, PATTERN(pattern_i8080_pop_1) } },
	[MNEM_I8080_PUSH] = { { NOPATTERN, PATTERN(pattern_i8080_push_1) } },
	[MNEM_I8080_RAL] = { { PATTERN(pattern_i8080_ral_0) } },
	[MNEM_I8080_RAR] = { { PATTERN(pattern_i8080_rar_0) } },
	[MNEM_I8080_R_CC] = { { PATTERN(pattern_i8080_r_cc_0) } },
	[MNEM_I8080_RET] = { { PATTERN(pattern_i8080_ret_0) } },
	[MNEM_I8080_RLC] = { { PATTERN(pattern_i8080_rlc_0) } },
	[MNEM_I8080_RRC] = { { PATTERN(pattern_i8080_rrc_0) } },
	[MNEM_I8080_RST] = { { NOPATTERN, PATTERN(pattern_i8080_rst_1) } },
	[MNEM_I8080_SBB] = { { NOPATTERN, PATTERN(pattern_i8080_sbb_1) } },
	[MNEM_I8080_SBI] = { { NOPATTERN, PATTERN(pattern_i8080_sbi_1) } },
	[MNEM_I8080_SHLD] = { { NOPATTERN, PATTERN(pattern_i8080_shld_1) } },
	[MNEM_I8080_SPHL] = { { PATTERN(pattern_i8080_sphl_0) } },
	[MNEM_I8080_STA] = { { NOPATTERN, PATTERN(pattern_i8080_sta_1) } },
	[MNEM_I8080_STC] = { { PATTERN(pattern_i8080_stc_0) } },
	[MNEM_I8080_STAX] = { { NOPATTERN, PATTERN(pattern_i8080_stax_1) } },
	[MNEM_I8080_SUB] = { { NOPATTERN, PATTERN(pattern_i8080_sub_1) } },
	[MNEM_I8080_SUI] = { { NOPATTERN, PATTERN(pattern_i8080_sui_1) } },
	[MNEM_I8080_XCHG] = { { PATTERN(pattern_i8080_xchg_0) } },
	[MNEM_I8080_XRA] = { { NOPATTERN, PATTERN(pattern_i8080_xra_1) } },
	[MNEM_I8080_XRI] = { { NOPATTERN, PATTERN(pattern_i8080_xri_1) } },
	[MNEM_I8080_XTHL] = { { PATTERN(pattern_i8080_xthl_0) } },

#if TARGET_X80
	[MNEM_I8085_ARHL] = { { PATTERN(pattern_i8080_arhl_0) } },
	[MNEM_I8085_DSUB] = { { PATTERN(pattern_i8080_dsub_0) } },
	[MNEM_I8085_JNUI] = { { NOPATTERN, PATTERN(pattern_i8080_jnui_1) } },
	[MNEM_I8085_JUI] = { { NOPATTERN, PATTERN(pattern_i8080_jui_1) } },
	[MNEM_I8085_LDHI] = { { NOPATTERN, PATTERN(pattern_i8080_ldhi_1) } },
	[MNEM_I8085_LDSI] = { { NOPATTERN, PATTERN(pattern_i8080_ldsi_1) } },
	[MNEM_I8085_LHLX] = { { PATTERN(pattern_i8080_lhlx_0) } },
	[MNEM_I8085_RDEL] = { { PATTERN(pattern_i8080_rdel_0) } },
	[MNEM_I8085_RIM] = { { PATTERN(pattern_i8080_rim_0) } },
	[MNEM_I8085_RSTV] = { { PATTERN(pattern_i8080_rstv_0) } },
	[MNEM_I8085_SHLX] = { { PATTERN(pattern_i8080_shlx_0) } },
	[MNEM_I8085_SIM] = { { PATTERN(pattern_i8080_sim_0) } },
#endif

	[MNEM_Z80_ADC] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_z80_adc_2) } },
	[MNEM_Z80_ADD] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_z80_add_2) } },
	[MNEM_Z80_AND] = { { NOPATTERN, PATTERN(pattern_z80_and_1), PATTERN(pattern_z80_and_2) } },
	[MNEM_Z80_BIT] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_z80_bit_2) } },
	[MNEM_Z80_CALL] = { { NOPATTERN, PATTERN(pattern_z80_call_1), PATTERN(pattern_z80_call_2) } },
	[MNEM_Z80_CP] = { { NOPATTERN, PATTERN(pattern_z80_cp_1), PATTERN(pattern_z80_cp_2) } },
	[MNEM_Z80_CPD] = { { PATTERN(pattern_z80_cpd_0) } },
	[MNEM_Z80_CPDR] = { { PATTERN(pattern_z80_cpdr_0) } },
	[MNEM_Z80_CPI] = { { PATTERN(pattern_z80_cpi_0) } },
	[MNEM_Z80_CPIR] = { { PATTERN(pattern_z80_cpir_0) } },
	[MNEM_Z80_DEC] = { { NOPATTERN, PATTERN(pattern_z80_dec_1) } },
	[MNEM_Z80_DJNZ] = { { NOPATTERN, PATTERN(pattern_z80_djnz_1) } },
	[MNEM_Z80_EX] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_z80_ex_2) } },
	[MNEM_Z80_EXX] = { { PATTERN(pattern_z80_exx_0) } },
	[MNEM_Z80_IM] = { { NOPATTERN, PATTERN(pattern_z80_im_1) } },
	[MNEM_Z80_IN] = { { NOPATTERN, PATTERN(pattern_z80_in_1), PATTERN(pattern_z80_in_2) } },
	[MNEM_Z80_INC] = { { NOPATTERN, PATTERN(pattern_z80_inc_1) } },
	[MNEM_Z80_IND] = { { PATTERN(pattern_z80_ind_0) } },
	[MNEM_Z80_INDR] = { { PATTERN(pattern_z80_indr_0) } },
	[MNEM_Z80_INI] = { { PATTERN(pattern_z80_ini_0) } },
	[MNEM_Z80_INIR] = { { PATTERN(pattern_z80_inir_0) } },
	[MNEM_Z80_JP] = { { NOPATTERN, PATTERN(pattern_z80_jp_1), PATTERN(pattern_z80_jp_2) } },
	[MNEM_Z80_JR] = { { NOPATTERN, PATTERN(pattern_z80_jr_1), PATTERN(pattern_z80_jr_2) } },
	[MNEM_Z80_LD] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_z80_ld_2) } },
	[MNEM_Z80_LDD] = { { PATTERN(pattern_z80_ldd_0) } },
	[MNEM_Z80_LDDR] = { { PATTERN(pattern_z80_lddr_0) } },
	[MNEM_Z80_LDI] = { { PATTERN(pattern_z80_ldi_0) } },
	[MNEM_Z80_LDIR] = { { PATTERN(pattern_z80_ldir_0) } },
	[MNEM_Z80_NEG] = { { PATTERN(pattern_z80_neg_0) } },
	[MNEM_Z80_OR] = { { NOPATTERN, PATTERN(pattern_z80_or_1), PATTERN(pattern_z80_or_2) } },
	[MNEM_Z80_OUT] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_z80_out_2) } },
	[MNEM_Z80_OUTD] = { { PATTERN(pattern_z80_outd_0) } },
	[MNEM_Z80_OTDR] = { { PATTERN(pattern_z80_otdr_0) } },
	[MNEM_Z80_OUTI] = { { PATTERN(pattern_z80_outi_0) } },
	[MNEM_Z80_OTIR] = { { PATTERN(pattern_z80_otir_0) } },
	[MNEM_Z80_PUSH] = { { NOPATTERN, PATTERN(pattern_z80_push_1) } },
	[MNEM_Z80_POP] = { { NOPATTERN, PATTERN(pattern_z80_pop_1) } },
	[MNEM_Z80_RES] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_z80_res_2), PATTERN(pattern_z80_res_3) } },
	[MNEM_Z80_RET] = { { PATTERN(pattern_z80_ret_0), PATTERN(pattern_z80_ret_1) } },
	[MNEM_Z80_RETI] = { { PATTERN(pattern_z80_reti_0) } },
	[MNEM_Z80_RETN] = { { PATTERN(pattern_z80_retn_0) } },
	[MNEM_Z80_RL] = { { NOPATTERN, PATTERN(pattern_z80_rl_1), PATTERN(pattern_z80_rl_2) } },
	[MNEM_Z80_RLC] = { { NOPATTERN, PATTERN(pattern_z80_rlc_1), PATTERN(pattern_z80_rlc_2) } },
	[MNEM_Z80_RLD] = { { PATTERN(pattern_z80_rld_0) } },
	[MNEM_Z80_RR] = { { NOPATTERN, PATTERN(pattern_z80_rr_1), PATTERN(pattern_z80_rr_2) } },
	[MNEM_Z80_RRC] = { { NOPATTERN, PATTERN(pattern_z80_rrc_1), PATTERN(pattern_z80_rrc_2) } },
	[MNEM_Z80_RRD] = { { PATTERN(pattern_z80_rrd_0) } },
	[MNEM_Z80_RST] = { { NOPATTERN, PATTERN(pattern_z80_rst_1) } },
	[MNEM_Z80_SBC] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_z80_sbc_2) } },
	[MNEM_Z80_SET] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_z80_set_2), PATTERN(pattern_z80_set_3) } },
	[MNEM_Z80_SLA] = { { NOPATTERN, PATTERN(pattern_z80_sla_1), PATTERN(pattern_z80_sla_2) } },
	[MNEM_Z80_SLL] = { { NOPATTERN, PATTERN(pattern_z80_sll_1), PATTERN(pattern_z80_sll_2) } },
	[MNEM_Z80_SRA] = { { NOPATTERN, PATTERN(pattern_z80_sra_1), PATTERN(pattern_z80_sra_2) } },
	[MNEM_Z80_SRL] = { { NOPATTERN, PATTERN(pattern_z80_srl_1), PATTERN(pattern_z80_srl_2) } },
	[MNEM_Z80_SUB] = { { NOPATTERN, PATTERN(pattern_z80_sub_1), PATTERN(pattern_z80_sub_2) } },
	[MNEM_Z80_XOR] = { { NOPATTERN, PATTERN(pattern_z80_xor_1), PATTERN(pattern_z80_xor_2) } },

#if TARGET_X80
	[MNEM_GBZ80_LDHL] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_z80_ldhl_2) } },
	[MNEM_GBZ80_SWAP] = { { NOPATTERN, PATTERN(pattern_z80_swap_1) } },
	[MNEM_GBZ80_STOP] = { { NOPATTERN, PATTERN(pattern_z80_stop_1) } },
#endif

#if TARGET_X80
	[MNEM_R800_ADJ] = { { NOPATTERN, PATTERN(pattern_r800_adj_1) } },
	[MNEM_R800_AND] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_z80_and_2) } },
	[MNEM_R800_BR] = { { NOPATTERN, PATTERN(pattern_z80_jp_1) } },
	[MNEM_R800_SHORT_BR] = { { NOPATTERN, PATTERN(pattern_z80_jr_1) } },
	[MNEM_R800_SHORT_B_CC] = { { NOPATTERN, PATTERN(pattern_r800_short_b_cc_1) } },
	[MNEM_R800_CMP] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_r800_cmp_2) } },
	[MNEM_R800_CMPM] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_r800_cmpm_2) } },
	[MNEM_R800_IN] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_r800_in_2) } },
	[MNEM_R800_INM] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_r800_inm_2) } },
	[MNEM_R800_MOVE] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_r800_move_2) } },
	[MNEM_R800_MOVEM] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_r800_movem_2) } },
	[MNEM_R800_MULUB] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_r800_mulub_2) } },
	[MNEM_R800_MULUW] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_r800_muluw_2) } },
	[MNEM_R800_NEG] = { { NOPATTERN, PATTERN(pattern_r800_neg_1) } },
	[MNEM_R800_NOT] = { { NOPATTERN, PATTERN(pattern_r800_not_1) } },
	[MNEM_R800_OR] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_r800_or_2) } },
	[MNEM_R800_OUT] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_r800_out_2) } },
	[MNEM_R800_OUTM] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_r800_outm_2) } },
	[MNEM_R800_ROL4] = { { NOPATTERN, PATTERN(pattern_r800_rol4_1) } },
	[MNEM_R800_ROR4] = { { NOPATTERN, PATTERN(pattern_r800_ror4_1) } },
	[MNEM_R800_SUB] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_z80_sub_2) } },
	[MNEM_R800_XOR] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_z80_xor_2) } },
#endif

	[MNEM_Z80_CALLN] = { { NOPATTERN, PATTERN(pattern_z80_calln_1) } },
	[MNEM_Z80_RETEM] = { { PATTERN(pattern_z80_retem_0) } },

#if TARGET_X80
	[MNEM_I8008_AC_R] = { { PATTERN(pattern_i8008_ac_r_0) } },
	[MNEM_I8008_AD_R] = { { PATTERN(pattern_i8008_ad_r_0) } },
	[MNEM_I8008_CP_R] = { { PATTERN(pattern_i8008_cp_r_0) } },
	[MNEM_I8008_DC_R] = { { PATTERN(pattern_i8008_dc_r_0) } },
	[MNEM_I8008_IN_R] = { { PATTERN(pattern_i8008_in_r_0) } },
	[MNEM_I8008_L_R_I] = { { NOPATTERN, PATTERN(pattern_i8008_l_r_i_1) } },
	[MNEM_I8008_L_R_R] = { { PATTERN(pattern_i8008_l_r_r_0) } },
	[MNEM_I8008_ND_R] = { { PATTERN(pattern_i8008_nd_r_0) } },
	[MNEM_I8008_OR_R] = { { PATTERN(pattern_i8008_or_r_0) } },
	[MNEM_I8008_SB_R] = { { PATTERN(pattern_i8008_sb_r_0) } },
	[MNEM_I8008_SU_R] = { { PATTERN(pattern_i8008_su_r_0) } },
	[MNEM_I8008_XR_R] = { { PATTERN(pattern_i8008_xr_r_0) } },
	[MNEM_DP2200_ALPHA] = { { PATTERN(pattern_dp2200_alpha_0) } },
	[MNEM_DP2200_BETA] = { { PATTERN(pattern_dp2200_beta_0) } },
	[MNEM_DP2200_EX] = { { PATTERN(pattern_dp2200_ex_0), PATTERN(pattern_i8080_out_1) } }, // extension
	[MNEM_DP2200_INPUT] = { { PATTERN(pattern_dp2200_input_0), PATTERN(pattern_i8080_in_1) } }, // extension
	[MNEM_DP2200_POP] = { { PATTERN(pattern_dp2200_pop_0) } },
	[MNEM_DP2200_PUSH] = { { PATTERN(pattern_dp2200_push_0) } },
#endif
};

