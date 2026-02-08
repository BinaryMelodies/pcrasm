
#include <assert.h>
#include <stdio.h>
#include "../asm.h"
#include "isa.h"

enum ins_flag_t
{
	INS_O8 = 0x0001, // operation is 8 bits
	INS_O16 = 0x0002, // operation is 16 bits
	INS_O32 = 0x0004, // operation is 32 bits
	_INS_O64 = 0x0008, // operation is 64 bits
	_INS_A16 = 0x0010, // address size is 16 bits
	INS_A32 = 0x0020, // address size is 32 bits
	_INS_A64 = 0x0040, // address size is 64 bits
	INS_OB = 0x0080, // default operand size is the current bit-width (jumps)
	INS_AB = 0x00100, // default address size is the current bit-width (loop)
	INS_OD64 = 0x0200, // operand size is 64-bit in 64-bit by default (jumps), also suppresses REX.W generation
	INS_NEEDSIZE = 0x0400, // cannot be matched unless size is explicit
	INS_F3 = 0x0800, // obligatory F3 prefix
	INS_NO64 = 0x1000, // only works in 16/32-bit mode
	INS_LM64 = 0x2000, // only works in 64-bit mode
	INS_OB32 = 0x4000, // default operand size is the current bit-width or 32 (to avoid generating prefixes)
	INS_X87 = 0x8000,
	INS_O64 = _INS_O64 | INS_LM64,
	INS_A16 = _INS_A16 | INS_NO64,
	INS_A64 = _INS_A64 | INS_LM64,
};
typedef enum ins_flag_t ins_flag_t;

enum constraint_t
{
	CST_ANY,
	CST_UI8, // unsigned 8-bit (may or may not truncate)
	CST_SI8, // signed 8-bit (may or may not truncate)
	CST_I8, // both signed and unsigned 8-bit permitted
	CST_UI16, // unsigned 16-bit (may or may not truncate)
//	CST_SI16, // signed 16-bit (may or may not truncate)
	CST_I16, // both signed and unsigned 16-bit permitted
	CST_UI32, // unsigned 32-bit (may or may not truncate)
	CST_SI32, // signed 32-bit (may or may not truncate)
	CST_I32, // both signed and unsigned 32-bit
	CST_I64, // signed 64-bit
	CST_REL8, // signed 8-bit relative to next instruction
	CST_REL16, // signed 16-bit relative to next instruction
	CST_REL32, // signed 32-bit relative to next instruction
	CST_AL,
	CST_AH,
	CST_AX,
	CST_EAX,
	CST_RAX,
	CST_CL,
	CST_DX,
	CST_ES,
	CST_CS,
	CST_SS,
	CST_DS,
	CST_FS,
	CST_GS,
	CST_DS3,
	CST_DS2,
	CST_PSW,
	CST_RSYMBOL,
	CST_R8, // 8-bit GPR (al, ah, bpl all allowed)
	CST_R16, // 16-bit GPR
	CST_R32, // 32-bit GPR
	CST_R64, // 64-bit GPR
	CST_RM8, // 8-bit GPR or memory operand
	CST_RM16, // 16-bit GPR or memory operand
	CST_RM32, // 32-bit GPR or memory operand
	CST_RM64, // 64-bit GPR or memory operand
	CST_RM16PLUS, // 16/32/64-bit GPR or memory operand
	CST_M, // memory operand
	CST_M8, // 8-bit memory operand
	CST_M16, // 16-bit memory operand
	CST_M32, // 32-bit memory operand
	CST_M64, // 64-bit memory operand
	CST_M80, // 80-bit memory operand
	CST_MOFF8, // 8-bit offset memory operand
	CST_MOFF16, // 16-bit offset memory operand
	CST_MOFF32, // 32-bit offset memory operand
	CST_MOFF64, // 64-bit offset memory operand
	CST_FAR16, // far 16:16-bit operand
	CST_FAR32, // far 16:32-bit operand
	CST_FARM16, // 16:16-bit far memory operand
	CST_FARM32, // 16:32-bit far memory operand
	CST_FARM64, // 16:64-bit far memory operand
	CST_SEG, // segment register
	CST_ST0,
	CST_ST,
	CST_CREG, // CRx
	CST_DREG, // DRx
	CST_TREG, // TRx
	CST_DSSI, // a string target location
	CST_ESDI, // a string target location
	CST_BXAL, // XLAT operand
	CST_EQ1, // 1
	CST_EQ3, // 3
	CST_CY, // cy (NEC)
	CST_DIR, // dir (NEC)
#define CST_MASK 0xFF
#define CST_NOSIZE 0x100 // operand must not have size (int 4)
#define CST_NEEDSIZE 0x200 // must have explicit size (movsx eax, [0])
#define CST_IMPLSIZE 0x400 // operand has implicit size (in eax, dx)
#define CST_NOTRUNC 0x800 // immediate must not be truncated (add cx, 0x80 should use the 2-byte form)
};
typedef enum constraint_t constraint_t;

enum action_t
{
	ACT_END,
	ACT_PUT, // puts a single byte
	ACT_PUT2, // puts 2 bytes
	ACT_PUT3, // puts 3 bytes
	ACT_I8_0, // puts an immediate byte from operand 0
	ACT_I8_1, // puts an immediate byte from operand 1
	ACT_I8_2, // puts an immediate byte from operand 2
	ACT_I16_0, // puts a 16-bit immediate from operand 0
	ACT_I16_1, // puts a 16-bit immediate from operand 1
	ACT_I16_2, // puts a 16-bit immediate from operand 2
	ACT_I32_0, // puts a 32-bit immediate from operand 0
	ACT_I32_1, // puts a 32-bit immediate from operand 1
	ACT_I32_2, // puts a 32-bit immediate from operand 2
	ACT_I64_1, // puts a 64-bit immediate from operand 1
	ACT_IA_1, // puts an immediate from operand 1 according to address size
	ACT_R8_0, // puts an immediate byte from operand 0 relative to the start of the next instruction
	ACT_R8_2, // puts an immediate byte from operand 2 relative to the start of the next instruction
	ACT_R16_0, // puts an immediate 16-bit word from operand 0 relative to the start of the next instruction
	ACT_R32_0, // puts an immediate 32-bit word from operand 0 relative to the start of the next instruction
	ACT_MEM, // puts a ModRM byte
	ACT_MEM_I, // puts a ModRM byte using a provided REG field value
	ACT_ADD_COND, // puts a single byte with the condition field added to it
	ACT_ADD_REG, // puts a single byte with the register field added to it
	ACT_ADD_FCMOV, // puts two bytes with the condition and register fields added to them, for FCMOV
	ACT_F16_0, // puts 16:16 value
	ACT_F32_0, // puts 16:32 value
};
typedef enum action_t action_t;
#define MAX_ACTION_COUNT 6

struct instruction_pattern_t
{
	cpu_type_t min_cpu, max_cpu;
	ins_flag_t flags;
	int modrm_operand, regfield_operand, register_operand;
	constraint_t constraint[MAX_OPD_COUNT + 1];
	action_t action[MAX_ACTION_COUNT];
	fpu_type_t min_fpu;
};

#define NO_MODRM -1, -1, -1
#define MODRM1(__x) (__x), -1, -1
#define MODRM2(__x, __y) (__x), (__y), -1
#define REGOPD(__x) -1, -1, (__x)

// TODO: prioritize errors (for example, invalid operand will appear a lot of times due to multiple potential patterns)
enum error_type
{
	_ERROR_TYPE_SUCCESS = 0, // not an error
	ERROR_TYPE_INTERNAL, // internal error
	ERROR_TYPE_OPCOUNT_ERROR, // invalid number of operands
	ERROR_TYPE_INVALID_OPERAND, // invalid operand
	ERROR_TYPE_OUT_OF_RANGE, // operand value outside permitted range
	ERROR_TYPE_WRONG_CPU, // not supported on specified CPU
	ERROR_TYPE_NO64, // not supported in 64-bit mode
	ERROR_TYPE_LM64, // only supported in 64-bit mode
	ERROR_TYPE_WRONG_FPU, // not supported on specified FPU
	ERROR_TYPE_OPSIZE_MISMATCH, // operand size mismatch
	ERROR_TYPE_OPSIZE_MISSSING, // operation size missing
	ERROR_TYPE_OPSIZE_INVALID, // invalid operation size
	ERROR_TYPE_ADSIZE_INVALID, // invalid address size
	ERROR_TYPE_SEGMENT_MISMATCH, // segment prefix mismatch
	ERROR_TYPE_INVALID_ADDRESS, // invalid addressing mode
	ERROR_TYPE_INVALID_ADDRESS_NON64, // invalid addressing mode in non-64-bit mode
	ERROR_TYPE_INVALID_REX_OPERAND, // operand not allowed with REX prefix
	_ERROR_TYPE_COUNT,
};

ssize_t compile_modrm_operand(instruction_t * ins, operand_t * opd, bool forgiving, uint8_t * bytes, uint8_t * rex, size_t * displacement_size)
{
	switch(opd->type)
	{
	case OPD_GPRB:
		if((opd->base & 8) != 0)
			*rex |= 0x41;
		else if(opd->base >= 4)
			*rex |= 0x40;
		*displacement_size = 0;
		bytes[0] = 0xC0 | (opd->base & 7);
		return 1;

	case OPD_GPRW:
	case OPD_GPRD:
	case OPD_GPRQ:
	case OPD_XMMREG:
		if((opd->base & 8) != 0)
			*rex |= 0x41;
		*displacement_size = 0;
		bytes[0] = 0xC0 | (opd->base & 7);
		return 1;

	case OPD_GPRH:
		if(*rex != 0)
			return -ERROR_TYPE_INVALID_REX_OPERAND;
		*displacement_size = 0;
		bytes[0] = 0xC0 | (opd->base & 7);
		return 1;

	case OPD_SEG:
	case OPD_STREG:
	case OPD_MMREG:
	case OPD_TREG:
		*displacement_size = 0;
		bytes[0] = 0xC0 | (opd->base & 7);
		return 1;

	case OPD_CREG:
	case OPD_DREG:
		// TODO: lock prefix
		if((opd->base & 8) != 0)
			*rex |= 0x41;
		*displacement_size = 0;
		bytes[0] = 0xC0 | (opd->base & 7);
		return 1;

	case OPD_MEM:
	case OPD_FARMEM:
		break;

	default:
		return -ERROR_TYPE_INVALID_OPERAND;
	}

	if(ins->address_size != BITSIZE_NONE && ins->address_size != opd->address_size)
	{
		//fprintf(stderr, "Address size mismatch\n");
		return -ERROR_TYPE_ADSIZE_INVALID;
	}
	if(ins->segment_prefix != 0 && opd->segment != REG_NOSEG && opd->segment != ins->segment_prefix)
	{
		//fprintf(stderr, "Segment prefix mismatch\n");
		return -ERROR_TYPE_SEGMENT_MISMATCH;
	}

	if(opd->address_size == BITSIZE16)
	{
		regnumber_t base;
		regnumber_t index;
		uint8_t modrm = 0;
		if(opd->base == REG_SI || opd->base == REG_DI || opd->index == REG_BX || opd->index == REG_BP)
		{
			base = opd->index;
			index = opd->base;
		}
		else
		{
			base = opd->base;
			index = opd->index;
		}
		if(base != REG_NONE && base != REG_BX && base != REG_BP)
		{
			//fprintf(stderr, "Invalid addressing mode\n");
			return -ERROR_TYPE_INVALID_ADDRESS;
		}
		if(index != REG_NONE && index != REG_BX && index != REG_BP)
		{
			//fprintf(stderr, "Invalid addressing mode\n");
			return -ERROR_TYPE_INVALID_ADDRESS;
		}
		if(forgiving)
		{
			modrm = 0x00;
			*displacement_size = 0;
		}
		else
		{
			reference_t displacement[1];
			evaluate_expression(opd->parameter, displacement, ins->code_offset);
			switch(is_scalar(displacement) ? integer_get_size(displacement->value) : 2)
			{
			case 0:
				modrm = 0x00;
				*displacement_size = 0;
				break;
			case 1:
				modrm = 0x40;
				*displacement_size = 1;
				break;
			default:
				modrm = 0x80;
				*displacement_size = 2;
				break;
			}
			int_clear(displacement->value);
		}
		switch(base)
		{
		case REG_NONE:
			switch(index)
			{
			case REG_NONE:
				modrm = 0x06;
				*displacement_size = 2;
				break;
			case REG_SI:
				modrm |= 0x04;
				break;
			case REG_DI:
				modrm |= 0x05;
				break;
			default:
				return -ERROR_TYPE_INTERNAL;
			}
			break;
		case REG_BX:
			switch(index)
			{
			case REG_NONE:
				modrm |= 0x07;
				break;
			case REG_SI:
				modrm |= 0x00;
				break;
			case REG_DI:
				modrm |= 0x01;
				break;
			default:
				return -ERROR_TYPE_INTERNAL;
			}
			break;
		case REG_BP:
			switch(index)
			{
			case REG_NONE:
				modrm |= 0x06;
				if(modrm == 0x06)
				{
					modrm = 0x46;
					*displacement_size = 1;
				}
				break;
			case REG_SI:
				modrm |= 0x02;
				break;
			case REG_DI:
				modrm |= 0x03;
				break;
			default:
				return -ERROR_TYPE_INTERNAL;
			}
			break;
		default:
			return -ERROR_TYPE_INTERNAL;
		}
		bytes[0] = modrm;
		return 1;
	}
	else
	{
		regnumber_t base;
		regnumber_t index;
		uint8_t modrm = 0;
		if(opd->base != REG_IPREL && (opd->index == REG_SP || (opd->base == REG_NONE && opd->scale == 1)))
		{
			base = opd->index;
			index = opd->base;
		}
		else
		{
			base = opd->base;
			index = opd->index;
		}
		if(index == REG_SP)
		{
			//fprintf(stderr, "Invalid addressing mode\n");
			return -ERROR_TYPE_INVALID_ADDRESS;
		}
		if(ins->bits != BITSIZE64 && (base == REG_IP || base == REG_IPREL))
		{
			//fprintf(stderr, "Invalid addressing mode in non-64-bit mode\n");
			return -ERROR_TYPE_INVALID_ADDRESS_NON64;
		}
		if((base == REG_IP || base == REG_IPREL) && index != REG_NONE)
		{
			//fprintf(stderr, "Invalid addressing mode\n");
			return -ERROR_TYPE_INVALID_ADDRESS;
		}
		if(forgiving)
		{
			modrm = 0x00;
			*displacement_size = 0;
		}
		else
		{
			reference_t displacement[1];
			evaluate_expression(opd->parameter, displacement, ins->code_offset);
			if(base == REG_IPREL)
			{
				uint_sub_ui(displacement->value, ins->following->code_offset);
			}
			// For EIP/RIP relative addressing, it always uses a 32-bit displacement
			switch(is_scalar(displacement) ? integer_get_size(displacement->value) : 2)
			{
			case 0:
				modrm = 0x00;
				*displacement_size = 0;
				break;
			case 1:
				modrm = 0x40;
				*displacement_size = 1;
				break;
			default:
				modrm = 0x80;
				*displacement_size = 4;
				break;
			}
			int_clear(displacement->value);
		}

		if(base == REG_IP || base == REG_IPREL)
		{
			modrm = 0x05;
			*displacement_size = 4;
			bytes[0] = modrm;
			return 1;
		}
		else if(opd->index == REG_NONE && base == REG_NONE && ins->bits != BITSIZE64)
		{
			modrm = 0x05;
			bytes[0] = modrm;
			*displacement_size = 4;
			return 1;
		}
		else if(opd->index == REG_NONE && (opd->base & 7) != 4 && !(ins->bits == BITSIZE64 && opd->base == REG_NONE))
		{
			if((base & 7) == 5 && modrm == 0x00)
			{
				modrm = 0x45;
				*displacement_size = 1;
			}
			else
			{
				modrm |= base & 7;
			}
			if(base != REG_NONE && (base & 8) != 0)
				*rex |= 0x41;
			bytes[0] = modrm;
			return 1;
		}
		else
		{
			// SIB
			uint8_t sib;
			switch(opd->scale)
			{
			case 1:
				sib = 0x00;
				break;
			case 2:
				sib = 0x40;
				break;
			case 4:
				sib = 0x80;
				break;
			case 8:
				sib = 0xC0;
				break;
			default:
//fprintf(stderr, "%ld\n", opd->scale);
				return -ERROR_TYPE_INTERNAL;
			}
			if(base == REG_NONE)
			{
				modrm = 0x00;
				sib |= 0x05;
				*displacement_size = 4;
			}
			else if((base & 7) == 5 && modrm == 0x00)
			{
				modrm = 0x40;
				sib |= 0x05;
				*displacement_size = 1;
			}
			else
			{
				sib |= base & 7;
			}
			if(index == REG_NONE)
			{
				sib |= 0x04;
			}
			else
			{
				sib |= (index & 7) << 3;
			}
			modrm |= 0x04;
			if(base != REG_NONE && (base & 8) != 0)
				*rex |= 0x41;
			if(index != REG_NONE && (index & 8) != 0)
				*rex |= 0x42;
			bytes[0] = modrm;
			bytes[1] = sib;
			return 2;
		}
	}
}

bitsize_t instruction_get_operation_size(const instruction_pattern_t * pattern, instruction_t * ins)
{
	bool need_size = false;
	bitsize_t size = ins->operation_size;
	for(size_t operand_index = 0; operand_index < ins->operand_count; operand_index++)
	{
		// some instructions must not have a size here (int 4)
		// some instructions provide an implicit size (in eax, dx)
		// sometimes it does not matter (mov es, qword [0])
		if((pattern->constraint[operand_index] & CST_NOSIZE) != 0 )
		{
			if(ins->operand[operand_index].size != BITSIZE_NONE)
				return BITSIZE_ERROR;
		}
		else if((pattern->constraint[operand_index] & (CST_NEEDSIZE | CST_IMPLSIZE)) != 0)
		{
			continue;
		}
		else
		{
			need_size = true;
			if(size == BITSIZE_NONE)
				size = ins->operand[operand_index].size;
			else if(ins->operand[operand_index].size != BITSIZE_NONE && ins->operand[operand_index].size != size)
				return BITSIZE_ERROR;
		}
	}
	if(size == BITSIZE_NONE && (pattern->flags & INS_OB) != 0)
		return ins->bits;
	else if(size == BITSIZE_NONE && (pattern->flags & INS_OB32) != 0)
		return ins->bits < BITSIZE64 ? ins->bits : BITSIZE32;
	else if(need_size || size != BITSIZE_NONE)
		return size;
	else if((pattern->flags & INS_O8) != 0)
		return BITSIZE8;
	else if((pattern->flags & INS_O16) != 0)
		return BITSIZE16;
	else if((pattern->flags & INS_O32) != 0)
		return BITSIZE32;
	else if((pattern->flags & _INS_O64) != 0)
		return BITSIZE64;
	else
		return BITSIZE_NONE;
}

bitsize_t instruction_get_address_size(const instruction_pattern_t * pattern, instruction_t * ins)
{
	bitsize_t size = ins->address_size;
	for(size_t operand_index = 0; operand_index < ins->operand_count; operand_index++)
	{
		if(ins->operand[operand_index].type == OPD_MEM || ins->operand[operand_index].type == OPD_FARMEM)
		{
			if(size == BITSIZE_NONE)
				size = ins->operand[operand_index].address_size;
			else if(ins->operand[operand_index].address_size != size)
				return BITSIZE_ERROR;
		}
	}
	if(size != BITSIZE_NONE)
		return size;
	else if((pattern->flags & INS_AB) != 0)
		return ins->bits;
	else if((pattern->flags & _INS_A16) != 0)
		return BITSIZE16;
	else if((pattern->flags & INS_A32) != 0)
		return BITSIZE32;
	else if((pattern->flags & _INS_A64) != 0)
		return BITSIZE64;
	else
		return BITSIZE_NONE;
}

static ssize_t instruction_pattern_get_length(const instruction_pattern_t * pattern, instruction_t * ins, bool forgiving);

static match_result_t instruction_pattern_match(const instruction_pattern_t * pattern, instruction_t * ins, bool forgiving)
{
	match_result_t result = { .type = MATCH_PERFECT };

	if(ins->cpu < pattern->min_cpu || ins->cpu > pattern->max_cpu)
	{
		result.type = MATCH_FAILED;
		result.info = ERROR_TYPE_WRONG_CPU;
		return result;
	}
	if((pattern->flags & INS_NO64) != 0 && ins->bits == BITSIZE64)
	{
		result.type = MATCH_FAILED;
		result.info = ERROR_TYPE_NO64;
		return result;
	}
	if((pattern->flags & INS_LM64) != 0 && ins->bits != BITSIZE64)
	{
		result.type = MATCH_FAILED;
		result.info = ERROR_TYPE_LM64;
		return result;
	}

	if((pattern->flags & INS_X87) != 0)
	{
		if(ins->fpu < pattern->min_fpu)
		{
			result.type = MATCH_FAILED;
			result.info = ERROR_TYPE_WRONG_FPU;
			return result;
		}
	}

	bitsize_t operation_size = instruction_get_operation_size(pattern, ins);
	if(operation_size == BITSIZE_ERROR)
	{
//		fprintf(stderr, "Operand size mismatch\n");
		result.type = MATCH_FAILED;
		result.info = ERROR_TYPE_OPSIZE_MISMATCH;
		return result;
	}

	if((pattern->flags & INS_NEEDSIZE) != 0 && operation_size == BITSIZE_NONE)
	{
		// operation needs size
		result.type = MATCH_FAILED;
		result.info = ERROR_TYPE_OPSIZE_MISSSING;
		return result;
	}

	bitsize_t expected_operation_size = { 0 };
	if((pattern->flags & INS_O8) != 0)
	{
		expected_operation_size = BITSIZE8;
	}
	if((pattern->flags & INS_O16) != 0)
	{
		expected_operation_size = BITSIZE16;
	}
	if((pattern->flags & INS_O32) != 0)
	{
		expected_operation_size = BITSIZE32;
	}
	if((pattern->flags & _INS_O64) != 0)
	{
		expected_operation_size = BITSIZE64;
	}

	if(expected_operation_size != BITSIZE_NONE)
	{
		if(operation_size == BITSIZE_NONE)
		{
//			fprintf(stderr, "Operation size not specified\n");
			// TODO: this should not occur
			result.type = MATCH_FAILED;
			result.info = ERROR_TYPE_INTERNAL;
			return result;
		}
		else if(operation_size != expected_operation_size)
		{
			result.type = MATCH_FAILED;
			result.info = ERROR_TYPE_OPSIZE_INVALID;
			return result;
		}
	}

	if(ins->bits == BITSIZE64 && (pattern->flags & INS_OD64) != 0 && operation_size == BITSIZE32)
	{
		result.type = MATCH_FAILED;
		result.info = ERROR_TYPE_OPSIZE_INVALID;
		return result;
	}

	bitsize_t address_size = instruction_get_address_size(pattern, ins);
	bitsize_t expected_address_size = { 0 };
	if((pattern->flags & _INS_A16) != 0)
	{
		expected_address_size = BITSIZE16;
	}
	if((pattern->flags & INS_A32) != 0)
	{
		expected_address_size = BITSIZE32;
	}
	if((pattern->flags & _INS_A64) != 0)
	{
		expected_address_size = BITSIZE64;
	}

	if(expected_address_size != BITSIZE_NONE)
	{
		if(address_size == BITSIZE_NONE)
		{
			// TODO: this should not occur
			result.type = MATCH_FAILED;
			result.info = ERROR_TYPE_INTERNAL;
			return result;
		}
		else if(address_size != expected_address_size)
		{
			result.type = MATCH_FAILED;
			result.info = ERROR_TYPE_ADSIZE_INVALID;
			return result;
		}
	}

	uint8_t rex = 0;

	if(pattern->modrm_operand != -1)
	{
		uint8_t tmp_bytes[2];
		size_t tmp_size;
		ssize_t result = compile_modrm_operand(ins, &ins->operand[pattern->modrm_operand], forgiving, tmp_bytes, &rex, &tmp_size);
		if(result < 0)
		{
			return (match_result_t) { .type = MATCH_FAILED, .info = -result };
		}
	}

	if(pattern->regfield_operand != -1)
	{
		if(ins->operand[pattern->regfield_operand].type == OPD_GPRH)
		{
			if(rex != 0)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_REX_OPERAND;
				return result;
			}
		}
		else if(ins->operand[pattern->regfield_operand].type == OPD_GPRB && ins->operand[pattern->regfield_operand].base >= 4)
		{
			rex |= 0x40;
		}

		if(rex != 0)
		{
			if(pattern->modrm_operand != -1 && ins->operand[pattern->modrm_operand].type == OPD_GPRH)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_REX_OPERAND;
				return result;
			}
		}
	}

	int iram_usage = -1;
	for(size_t operand_index = 0; operand_index < ins->operand_count; operand_index++)
	{
		reference_t ref[1];
		switch((constraint_t)(pattern->constraint[operand_index] & CST_MASK))
		{
		case CST_ANY:
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
				if((pattern->constraint[operand_index] & CST_NOTRUNC) != 0)
				{
					result.type = MATCH_FAILED;
					result.info = ERROR_TYPE_OUT_OF_RANGE;
					return result;
				}
				result.type = MATCH_TRUNCATED;
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
				if((pattern->constraint[operand_index] & CST_NOTRUNC) != 0)
				{
					result.type = MATCH_FAILED;
					result.info = ERROR_TYPE_OUT_OF_RANGE;
					return result;
				}
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
				if((pattern->constraint[operand_index] & CST_NOTRUNC) != 0)
				{
					result.type = MATCH_FAILED;
					result.info = ERROR_TYPE_OUT_OF_RANGE;
					return result;
				}
				result.type = MATCH_TRUNCATED;
			}
			int_clear(ref->value);
			break;
#if 0
		case CST_SI16:
			if(ins->operand[operand_index].type != OPD_IMM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(forgiving)
				break;
			evaluate_expression(ins->operand[operand_index].parameter, ref, ins->code_offset);
			if(ins->bits > BITSIZE16 && (!is_scalar(ref) || integer_get_size(ref->value) > 2))
			{
				if((pattern->constraint[operand_index] & CST_NOTRUNC) != 0)
				{
					result.type = MATCH_FAILED;
					result.info = ERROR_TYPE_OUT_OF_RANGE;
					return result;
				}
				result.type = MATCH_TRUNCATED;
			}
			int_clear(ref->value);
			break;
#endif
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
		case CST_UI32:
			if(ins->operand[operand_index].type != OPD_IMM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(forgiving)
				break;
			evaluate_expression(ins->operand[operand_index].parameter, ref, ins->code_offset);
			if(ins->bits > BITSIZE32 && (!is_scalar(ref) || uinteger_get_size(ref->value) > 4))
			{
				if((pattern->constraint[operand_index] & CST_NOTRUNC) != 0)
				{
					result.type = MATCH_FAILED;
					result.info = ERROR_TYPE_OUT_OF_RANGE;
					return result;
				}
				result.type = MATCH_TRUNCATED;
			}
			int_clear(ref->value);
			break;
		case CST_SI32:
			if(ins->operand[operand_index].type != OPD_IMM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(forgiving)
				break;
			evaluate_expression(ins->operand[operand_index].parameter, ref, ins->code_offset);
			if(ins->bits > BITSIZE32 && (!is_scalar(ref) || integer_get_size(ref->value) > 4))
			{
				if((pattern->constraint[operand_index] & CST_NOTRUNC) != 0)
				{
					result.type = MATCH_FAILED;
					result.info = ERROR_TYPE_OUT_OF_RANGE;
					return result;
				}
				result.type = MATCH_TRUNCATED;
			}
			int_clear(ref->value);
			break;
		case CST_I32:
			if(ins->operand[operand_index].type != OPD_IMM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(forgiving)
				break;
			evaluate_expression(ins->operand[operand_index].parameter, ref, ins->code_offset);
			if(!is_scalar(ref) || (integer_get_size(ref->value) > 4 && uinteger_get_size(ref->value) > 4))
			{
				result.type = MATCH_TRUNCATED;
			}
			int_clear(ref->value);
			break;
		case CST_I64:
			if(ins->operand[operand_index].type != OPD_IMM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
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
		case CST_REL16:
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

			if(ins->bits > BITSIZE16)
			{
				current_section = ins->containing_section;
				if(!is_self_relative(ref))
				{
					result.type = MATCH_TRUNCATED;
				}
				else if(integer_get_size(ref->value) > 2)
				{
					result.type = MATCH_FAILED;
					result.info = ERROR_TYPE_OUT_OF_RANGE;
				}
			}

			int_clear(ref->value);
			break;
		case CST_REL32:
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

			if(ins->bits > BITSIZE32)
			{
				current_section = ins->containing_section;
				// since there are no REL64 encodings, we have to assume the relocation will fit
				/*if(!is_self_relative(ref))
				{
					result.type = MATCH_TRUNCATED;
				}
				else*/ if(integer_get_size(ref->value) > 4)
				{
					result.type = MATCH_FAILED;
					result.info = ERROR_TYPE_OUT_OF_RANGE;
				}
			}

			int_clear(ref->value);
			break;
		case CST_AL:
			if(ins->operand[operand_index].type != OPD_GPRB || ins->operand[operand_index].base != REG_AL)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_AH:
			if(ins->operand[operand_index].type != OPD_GPRH || ins->operand[operand_index].base != REG_AH)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_AX:
			if(ins->operand[operand_index].type != OPD_GPRW || ins->operand[operand_index].base != REG_AX)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_EAX:
			if(ins->operand[operand_index].type != OPD_GPRD || ins->operand[operand_index].base != REG_AX)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_RAX:
			if(ins->operand[operand_index].type != OPD_GPRQ || ins->operand[operand_index].base != REG_AX)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_CL:
			if(ins->operand[operand_index].type != OPD_GPRB || ins->operand[operand_index].base != REG_CL)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_DX:
			if(ins->operand[operand_index].type != OPD_GPRW || ins->operand[operand_index].base != REG_DX)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_ES:
			if(ins->operand[operand_index].type != OPD_SEG || ins->operand[operand_index].base != (regnumber_t)REG_ES)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_CS:
			if(ins->operand[operand_index].type != OPD_SEG || ins->operand[operand_index].base != (regnumber_t)REG_CS)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_SS:
			if(ins->operand[operand_index].type != OPD_SEG || ins->operand[operand_index].base != (regnumber_t)REG_SS)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_DS:
			if(ins->operand[operand_index].type != OPD_SEG || ins->operand[operand_index].base != (regnumber_t)REG_DS)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_FS:
			if(ins->operand[operand_index].type != OPD_SEG || ins->operand[operand_index].base != (regnumber_t)REG_FS)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_GS:
			if(ins->operand[operand_index].type != OPD_SEG || ins->operand[operand_index].base != (regnumber_t)REG_GS)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_DS3:
			if(ins->operand[operand_index].type != OPD_SEG || ins->operand[operand_index].base != (regnumber_t)REG_DS3)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_DS2:
			if(ins->operand[operand_index].type != OPD_SEG || ins->operand[operand_index].base != (regnumber_t)REG_DS2)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_PSW:
			if(ins->operand[operand_index].type != OPD_PSW)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_RSYMBOL:
			if(ins->operand[operand_index].type != OPD_RREG)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_R8:
			if(ins->operand[operand_index].type != OPD_GPRB && ins->operand[operand_index].type != OPD_GPRH)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_R16:
			if(ins->operand[operand_index].type != OPD_GPRW)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_R32:
			if(ins->operand[operand_index].type != OPD_GPRD)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_R64:
			if(ins->operand[operand_index].type != OPD_GPRQ)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_RM8:
			switch(ins->operand[operand_index].type)
			{
			case OPD_GPRB:
			case OPD_GPRH:
				break;
			case OPD_MEM:
				if(ins->operand[operand_index].size != BITSIZE_NONE && ins->operand[operand_index].size != BITSIZE8)
				{
					result.type = MATCH_FAILED;
					result.info = ERROR_TYPE_INVALID_OPERAND;
					return result;
				}
				break;
			default:
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_RM16:
			switch(ins->operand[operand_index].type)
			{
			case OPD_GPRW:
				break;
			case OPD_MEM:
				if(ins->operand[operand_index].size != BITSIZE_NONE && ins->operand[operand_index].size != BITSIZE16)
				{
					result.type = MATCH_FAILED;
					result.info = ERROR_TYPE_INVALID_OPERAND;
					return result;
				}
				break;
			default:
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_RM32:
			switch(ins->operand[operand_index].type)
			{
			case OPD_GPRD:
				break;
			case OPD_MEM:
				if(ins->operand[operand_index].size != BITSIZE_NONE && ins->operand[operand_index].size != BITSIZE32)
				{
					result.type = MATCH_FAILED;
					result.info = ERROR_TYPE_INVALID_OPERAND;
					return result;
				}
				break;
			default:
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_RM64:
			switch(ins->operand[operand_index].type)
			{
			case OPD_GPRQ:
				break;
			case OPD_MEM:
				if(ins->operand[operand_index].size != BITSIZE_NONE && ins->operand[operand_index].size != BITSIZE64)
				{
					result.type = MATCH_FAILED;
					result.info = ERROR_TYPE_INVALID_OPERAND;
					return result;
				}
				break;
			default:
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_RM16PLUS:
			switch(ins->operand[operand_index].type)
			{
			case OPD_GPRW:
			case OPD_GPRD:
			case OPD_GPRQ:
				break;
			case OPD_MEM:
				if(ins->operand[operand_index].size != BITSIZE_NONE
				&& ins->operand[operand_index].size != BITSIZE16
				&& ins->operand[operand_index].size != BITSIZE32
				&& ins->operand[operand_index].size != BITSIZE64)
				{
					result.type = MATCH_FAILED;
					result.info = ERROR_TYPE_INVALID_OPERAND;
					return result;
				}
				break;
			default:
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_M:
			if(ins->operand[operand_index].type != OPD_MEM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_M8:
			if(ins->operand[operand_index].type != OPD_MEM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			else if(ins->operand[operand_index].size != BITSIZE_NONE && ins->operand[operand_index].size != BITSIZE8)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_M16:
			if(ins->operand[operand_index].type != OPD_MEM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			else if(ins->operand[operand_index].size != BITSIZE_NONE && ins->operand[operand_index].size != BITSIZE16)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_M32:
			if(ins->operand[operand_index].type != OPD_MEM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			else if(ins->operand[operand_index].size != BITSIZE_NONE && ins->operand[operand_index].size != BITSIZE32)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_M64:
			if(ins->operand[operand_index].type != OPD_MEM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			else if(ins->operand[operand_index].size != BITSIZE_NONE && ins->operand[operand_index].size != BITSIZE64)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_M80:
			if(ins->operand[operand_index].type != OPD_MEM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			else if(ins->operand[operand_index].size != BITSIZE_NONE && ins->operand[operand_index].size != BITSIZE80)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_MOFF8:
			if(ins->operand[operand_index].type != OPD_MEM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			else if(ins->operand[operand_index].size != BITSIZE_NONE && ins->operand[operand_index].size != BITSIZE8)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			else if(ins->operand[operand_index].base != REG_NONE || ins->operand[operand_index].index != REG_NONE)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_MOFF16:
			if(ins->operand[operand_index].type != OPD_MEM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			else if(ins->operand[operand_index].size != BITSIZE_NONE && ins->operand[operand_index].size != BITSIZE16)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			else if(ins->operand[operand_index].base != REG_NONE || ins->operand[operand_index].index != REG_NONE)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_MOFF32:
			if(ins->operand[operand_index].type != OPD_MEM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			else if(ins->operand[operand_index].size != BITSIZE_NONE && ins->operand[operand_index].size != BITSIZE32)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			else if(ins->operand[operand_index].base != REG_NONE || ins->operand[operand_index].index != REG_NONE)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_MOFF64:
			if(ins->operand[operand_index].type != OPD_MEM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			else if(ins->operand[operand_index].size != BITSIZE_NONE && ins->operand[operand_index].size != BITSIZE64)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			else if(ins->operand[operand_index].base != REG_NONE || ins->operand[operand_index].index != REG_NONE)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_FAR16:
			if(ins->operand[operand_index].type != OPD_FARIMM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			else if(ins->operand[operand_index].size != BITSIZE_NONE && ins->operand[operand_index].size != BITSIZE16)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_FAR32:
			if(ins->operand[operand_index].type != OPD_FARIMM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			else if(ins->operand[operand_index].size != BITSIZE_NONE && ins->operand[operand_index].size != BITSIZE32)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_FARM16:
			if(ins->operand[operand_index].type != OPD_FARMEM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(ins->operand[operand_index].size != BITSIZE_NONE && ins->operand[operand_index].size != BITSIZE16)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_FARM32:
			if(ins->operand[operand_index].type != OPD_FARMEM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(ins->operand[operand_index].size != BITSIZE_NONE && ins->operand[operand_index].size != BITSIZE32)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_FARM64:
			if(ins->operand[operand_index].type != OPD_FARMEM)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(ins->operand[operand_index].size != BITSIZE_NONE && ins->operand[operand_index].size != BITSIZE64)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_SEG:
			if(ins->operand[operand_index].type != OPD_SEG)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_ST0:
			if(ins->operand[operand_index].type != OPD_STREG || ins->operand[operand_index].base != 0)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_ST:
			if(ins->operand[operand_index].type != OPD_STREG)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_CREG:
			if(ins->operand[operand_index].type != OPD_CREG)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_DREG:
			if(ins->operand[operand_index].type != OPD_DREG)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_TREG:
			if(ins->operand[operand_index].type != OPD_TREG)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
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
		case CST_EQ3:
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
			if(uint_cmp_ui(ref->value, 3) != 0)
			{
				int_clear(ref->value);
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			int_clear(ref->value);
			break;
		case CST_DSSI:
			if(ins->operand[operand_index].type != OPD_MEM
			|| ins->operand[operand_index].base != REG_NONE
			|| ins->operand[operand_index].index != REG_NONE)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(ins->operand[operand_index].segment == REG_IRAM)
			{
				if(iram_usage == -1)
					iram_usage = CST_DSSI;
				else
				{
					result.type = MATCH_FAILED;
					result.info = ERROR_TYPE_INVALID_OPERAND;
					return result;
				}
			}
			else if(ins->operand[operand_index].segment == REG_DS3)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_ESDI:
			if(ins->operand[operand_index].type != OPD_MEM
			|| ins->operand[operand_index].base != REG_NONE
			|| ins->operand[operand_index].index != REG_NONE)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			if(ins->operand[operand_index].segment == REG_IRAM)
			{
				if(iram_usage == -1)
					iram_usage = CST_ESDI;
				else
				{
					result.type = MATCH_FAILED;
					result.info = ERROR_TYPE_INVALID_OPERAND;
					return result;
				}
			}
			else if(ins->operand[operand_index].segment != REG_NOSEG && ins->operand[operand_index].segment != REG_ES && ins->operand[operand_index].segment != REG_DS3)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_BXAL:
			if(ins->operand[operand_index].type != OPD_MEM
			|| ins->operand[operand_index].base != REG_NONE
			|| ins->operand[operand_index].index != REG_NONE
			|| (ins->operand[operand_index].size != BITSIZE_NONE && ins->operand[operand_index].size != BITSIZE8))
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_CY:
			if(ins->operand[operand_index].type != OPD_CY)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		case CST_DIR:
			if(ins->operand[operand_index].type != OPD_DIR)
			{
				result.type = MATCH_FAILED;
				result.info = ERROR_TYPE_INVALID_OPERAND;
				return result;
			}
			break;
		}

		if((pattern->constraint[operand_index] & CST_NEEDSIZE) != 0 && ins->operand[operand_index].size == BITSIZE_NONE)
		{
			result.type = MATCH_FAILED;
			result.info = ERROR_TYPE_INVALID_OPERAND;
			return result;
		}
	}

	if(instruction_pattern_get_length(pattern, ins, forgiving) < ins->code_size)
	{
		result.type = MATCH_FAILED;
		result.info = ERROR_TYPE_INTERNAL; // TODO: make more specific
		return result;
	}

	return result;
}

static const pattern_t x86_patterns[_MNEM_TOTAL];

const instruction_pattern_t * find_pattern(instruction_t * ins, bool forgiving, uint64_t * error_types_ptr)
{
	uint64_t error_types = 0;
	const struct instruction_patterns_t * patterns = &x86_patterns[ins->mnemonic].pattern[ins->operand_count];
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
		case MATCH_TRUNCATED: // TODO: signal truncation
			{
				// find shortest match ("tightest")
				size_t new_length = instruction_pattern_get_length(&patterns->pattern[pattern_index], ins, forgiving);
//				printf("%ld -> length: %d previous: %d\n", ins->line_number, new_length, perfect_match_length);
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

static ssize_t instruction_pattern_get_length(const instruction_pattern_t * pattern, instruction_t * ins, bool forgiving)
{
	size_t length = 0;

	uint8_t rex = 0;

	if(ins->lock_prefix)
	{
		length ++;
	}

	if(ins->umov_prefix)
	{
		length ++;
	}

	if(ins->repeat_prefix != PREF_NOREP)
	{
		length ++;
	}

	segment_t source_segment_prefix = ins->source_segment_prefix;
	segment_t target_segment_prefix = ins->target_segment_prefix;

	for(size_t operand_index = 0; operand_index < ins->operand_count; operand_index++)
	{
		if(ins->operand[operand_index].type != OPD_MEM && ins->operand[operand_index].type != OPD_FARMEM)
			continue;
		if(ins->operand[operand_index].segment == REG_NOSEG)
			continue;
		switch(pattern->constraint[operand_index] & CST_MASK)
		{
		case CST_DSSI:
			source_segment_prefix = ins->operand[operand_index].segment;
			break;
		case CST_ESDI:
			if(ins->operand[operand_index].segment != REG_ES)
				target_segment_prefix = ins->operand[operand_index].segment;
			break;
		default:
			source_segment_prefix = ins->operand[pattern->modrm_operand].segment;
			target_segment_prefix = REG_NOSEG;
		}
	}

	if(source_segment_prefix != REG_NOSEG)
		length ++;
	if(target_segment_prefix != REG_NOSEG && source_segment_prefix != target_segment_prefix)
		length ++;

	bitsize_t operation_size = instruction_get_operation_size(pattern, ins);

	if((pattern->flags & INS_X87) == 0 && operation_size == BITSIZE16)
	{
		if(ins->bits != BITSIZE16)
			length ++;
	}
	if((pattern->flags & INS_X87) == 0 && operation_size == BITSIZE32)
	{
		if(ins->bits == BITSIZE16)
			length ++;
	}
	if((pattern->flags & INS_X87) == 0 && operation_size == BITSIZE64 && (pattern->flags & INS_OD64) == 0)
	{
		rex |= 0x48;
	}

	bitsize_t address_size = instruction_get_address_size(pattern, ins);

	if(address_size == BITSIZE16)
	{
		if(ins->bits == BITSIZE32)
			length ++;
		// cannot be 64-bit
	}
	if(address_size == BITSIZE32)
	{
		if(ins->bits != BITSIZE32)
			length ++;
	}

	if(pattern->modrm_operand != -1)
	{
		uint8_t modrm_bytes[2];
		size_t displacement_size;
		ssize_t modrm_count = compile_modrm_operand(ins, &ins->operand[pattern->modrm_operand], forgiving, modrm_bytes, &rex, &displacement_size);
		if(modrm_count < 0)
		{
			//fprintf(stderr, "Internal error\n");
			//assert(false);
			return -ERROR_TYPE_INTERNAL;
		}
		length += modrm_count + displacement_size;
	}

	if(pattern->regfield_operand != -1)
	{
		if(ins->operand[pattern->regfield_operand].base != REG_NONE && (ins->operand[pattern->regfield_operand].base & 8) != 0)
		{
			rex |= 0x44;
		}
		else if(ins->operand[pattern->regfield_operand].type == OPD_GPRB && ins->operand[pattern->regfield_operand].base >= 4)
		{
			rex |= 0x40;
		}
	}

	if(pattern->register_operand != -1)
	{
		if(ins->operand[pattern->register_operand].base != REG_NONE && (ins->operand[pattern->register_operand].base & 8) != 0)
		{
			rex |= 0x41;
		}
		else if(ins->operand[pattern->register_operand].type == OPD_GPRB && ins->operand[pattern->register_operand].base >= 4)
		{
			rex |= 0x40;
		}
	}

	if(rex != 0)
	{
		length ++;
	}

	for(size_t action_index = 0; action_index < MAX_ACTION_COUNT; action_index++)
	{
		switch(pattern->action[action_index])
		{
		case ACT_END:
			goto end;
		case ACT_PUT:
			length += 1;
			action_index ++;
			break;
		case ACT_PUT2:
			length += 2;
			action_index += 2;
			break;
		case ACT_PUT3:
			length += 3;
			action_index += 3;
			break;
		case ACT_I8_0:
		case ACT_I8_1:
		case ACT_I8_2:
		case ACT_R8_0:
		case ACT_R8_2:
			length += 1;
			break;
		case ACT_I16_0:
		case ACT_I16_1:
		case ACT_I16_2:
		case ACT_R16_0:
			length += 2;
			break;
		case ACT_I32_0:
		case ACT_I32_1:
		case ACT_I32_2:
		case ACT_R32_0:
		case ACT_F16_0:
			length += 4;
			break;
		case ACT_F32_0:
			length += 6;
			break;
		case ACT_I64_1:
			length += 8;
			break;
		case ACT_IA_1:
			if(address_size != BITSIZE16 && address_size != BITSIZE32 && address_size != BITSIZE64)
			{
				return -ERROR_TYPE_INTERNAL;
			}
			length += OCTETSIN(address_size);
			break;
		case ACT_MEM:
			// already added
			break;
		case ACT_MEM_I:
			action_index ++;
			// already added
			break;
		case ACT_ADD_COND:
		case ACT_ADD_REG:
			action_index ++;
			length += 1;
			break;
		case ACT_ADD_FCMOV:
			action_index += 2;
			length += 2;
			break;
		}
	}

end:
	return length;
}

static const uint8_t segment_prefix[] =
{
	/* ES (DS1) */
	0x26,
	/* CS (PS) */
	0x2E,
	/* SS */
	0x36,
	/* DS (DS0) */
	0x3E,
	/* FS */
	0x64,
	/* GS */
	0x65,
	/* DS3 */
	0xD6,
	/* DS2 */
	0x63,
	/* IRAM */
	0xF1,
};

static void instruction_pattern_generate(const instruction_pattern_t * pattern, instruction_t * ins)
{
	uint8_t rex = 0;

	current_section = ins->containing_section;

	if(ins->lock_prefix)
	{
		output_byte(0xF0);
	}

	if(ins->umov_prefix)
	{
		output_byte(0xF1);
	}

	if((pattern->flags & INS_F3) != 0)
	{
		output_byte(0xF3);
	}
	else switch(ins->repeat_prefix)
	{
	case PREF_NOREP:
		break;
	case PREF_REP:
		output_byte(0xF3);
		break;
	case PREF_REPE:
		output_byte(0xF3);
		break;
	case PREF_REPNE:
		output_byte(0xF2);
		break;
	case PREF_REPC:
		output_byte(0x65);
		break;
	case PREF_REPNC:
		output_byte(0x64);
		break;
	}

	segment_t source_segment_prefix = ins->source_segment_prefix;
	segment_t target_segment_prefix = ins->target_segment_prefix;

	for(size_t operand_index = 0; operand_index < ins->operand_count; operand_index++)
	{
		if(ins->operand[operand_index].type != OPD_MEM && ins->operand[operand_index].type != OPD_FARMEM)
			continue;
		if(ins->operand[operand_index].segment == REG_NOSEG)
			continue;
		switch(pattern->constraint[operand_index] & CST_MASK)
		{
		case CST_DSSI:
			source_segment_prefix = ins->operand[operand_index].segment;
			break;
		case CST_ESDI:
			if(ins->operand[operand_index].segment != REG_ES)
				target_segment_prefix = ins->operand[operand_index].segment;
			break;
		default:
			source_segment_prefix = ins->operand[pattern->modrm_operand].segment;
			target_segment_prefix = REG_NOSEG;
		}
	}

	if(source_segment_prefix != REG_NOSEG)
		output_byte(segment_prefix[source_segment_prefix]);
	if(target_segment_prefix != REG_NOSEG && source_segment_prefix != target_segment_prefix)
		output_byte(segment_prefix[target_segment_prefix]);

	bitsize_t operation_size = instruction_get_operation_size(pattern, ins);

	if((pattern->flags & INS_X87) == 0 && operation_size == BITSIZE16)
	{
		if(ins->bits != BITSIZE16)
			output_byte(0x66);
	}
	if((pattern->flags & INS_X87) == 0 && operation_size == BITSIZE32)
	{
		if(ins->bits == BITSIZE16)
			output_byte(0x66);
	}
	if((pattern->flags & INS_X87) == 0 && operation_size == BITSIZE64 && (pattern->flags & INS_OD64) == 0)
	{
		rex |= 0x48;
	}

	bitsize_t address_size = instruction_get_address_size(pattern, ins);

	if(address_size == BITSIZE16)
	{
		if(ins->bits == BITSIZE32)
			output_byte(0x67);
		// cannot be 64-bit
	}
	if(address_size == BITSIZE32)
	{
		if(ins->bits != BITSIZE32)
			output_byte(0x67);
	}

	uint8_t modrm_bytes[2];
	ssize_t modrm_count = 0;
	size_t displacement_size = (size_t)-1;
	if(pattern->modrm_operand != -1)
	{
		modrm_count = compile_modrm_operand(ins, &ins->operand[pattern->modrm_operand], false, modrm_bytes, &rex, &displacement_size);
		assert(displacement_size != (size_t)-1);
		if(modrm_count < 0)
		{
			fprintf(stderr, "Internal error\n");
			assert(false);
		}
	}

	if(pattern->regfield_operand != -1)
	{
		if(ins->operand[pattern->regfield_operand].base != REG_NONE && (ins->operand[pattern->regfield_operand].base & 8) != 0)
		{
			rex |= 0x44;
		}
		else if(ins->operand[pattern->regfield_operand].type == OPD_GPRB && ins->operand[pattern->regfield_operand].base >= 4)
		{
			rex |= 0x40;
		}
	}

	if(pattern->register_operand != -1)
	{
		if(ins->operand[pattern->register_operand].base != REG_NONE && (ins->operand[pattern->register_operand].base & 8) != 0)
		{
			rex |= 0x41;
		}
		else if(ins->operand[pattern->register_operand].type == OPD_GPRB && ins->operand[pattern->register_operand].base >= 4)
		{
			rex |= 0x40;
		}
	}

	if(rex != 0)
	{
		output_byte(rex);
	}

	for(size_t action_index = 0; action_index < MAX_ACTION_COUNT; action_index++)
	{
		reference_t ref[1];
		switch(pattern->action[action_index])
		{
		case ACT_END:
			return;
		case ACT_PUT:
			output_byte(pattern->action[++action_index]);
			break;
		case ACT_PUT2:
			output_byte(pattern->action[++action_index]);
			output_byte(pattern->action[++action_index]);
			break;
		case ACT_PUT3:
			output_byte(pattern->action[++action_index]);
			output_byte(pattern->action[++action_index]);
			output_byte(pattern->action[++action_index]);
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
		case ACT_I8_2:
			evaluate_expression(ins->operand[2].parameter, ref, ins->code_offset);
			output_word(ref, DATA_LE, 1);
			int_clear(ref->value);
			break;
		case ACT_R8_0:
			evaluate_expression(ins->operand[0].parameter, ref, ins->code_offset);
			uint_sub_ui(ref->value, ins->following->code_offset);
			output_word_pcrel(ref, DATA_LE, 1);
			int_clear(ref->value);
			break;
		case ACT_R8_2:
			evaluate_expression(ins->operand[2].parameter, ref, ins->code_offset);
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
		case ACT_I16_2:
			evaluate_expression(ins->operand[2].parameter, ref, ins->code_offset);
			output_word(ref, DATA_LE, 2);
			int_clear(ref->value);
			break;
		case ACT_R16_0:
			evaluate_expression(ins->operand[0].parameter, ref, ins->code_offset);
			uint_sub_ui(ref->value, ins->following->code_offset);
			output_word_pcrel(ref, DATA_LE, 2);
			int_clear(ref->value);
			break;
		case ACT_I32_0:
			evaluate_expression(ins->operand[0].parameter, ref, ins->code_offset);
			output_word(ref, DATA_LE, 4);
			int_clear(ref->value);
			break;
		case ACT_I32_1:
			evaluate_expression(ins->operand[1].parameter, ref, ins->code_offset);
			output_word(ref, DATA_LE, 4);
			int_clear(ref->value);
			break;
		case ACT_I32_2:
			evaluate_expression(ins->operand[2].parameter, ref, ins->code_offset);
			output_word(ref, DATA_LE, 4);
			int_clear(ref->value);
			break;
		case ACT_R32_0:
			evaluate_expression(ins->operand[0].parameter, ref, ins->code_offset);
			uint_sub_ui(ref->value, ins->following->code_offset);
//printf(":%d %d\n", ref->var.type, ref->var.section_index);
			output_word_pcrel(ref, DATA_LE, 4);
			int_clear(ref->value);
			break;
		case ACT_I64_1:
			evaluate_expression(ins->operand[1].parameter, ref, ins->code_offset);
			output_word(ref, DATA_LE, 8);
			int_clear(ref->value);
			break;
		case ACT_IA_1:
			evaluate_expression(ins->operand[1].parameter, ref, ins->code_offset);
			output_word(ref, DATA_LE, OCTETSIN(address_size));
			int_clear(ref->value);
			break;
		case ACT_MEM:
			modrm_bytes[0] |= (ins->operand[pattern->regfield_operand].base & 7) << 3;
			goto output_modrm;
		case ACT_MEM_I:
			modrm_bytes[0] |= ((pattern->action[++action_index] & 7) << 3);
		output_modrm:
			output_byte(modrm_bytes[0]);
			if(modrm_count > 1)
				output_byte(modrm_bytes[1]);
			if(displacement_size != 0)
			{
				evaluate_expression(ins->operand[pattern->modrm_operand].parameter, ref, ins->code_offset);
				if(ins->operand[pattern->modrm_operand].base == REG_IPREL)
				{
					uint_sub_ui(ref->value, ins->following->code_offset);
					output_word_pcrel(ref, DATA_LE, displacement_size);
				}
				else
				{
					output_word(ref, DATA_LE, displacement_size);
				}
				int_clear(ref->value);
			}
			break;
		case ACT_ADD_COND:
			output_byte(ins->condition + pattern->action[++action_index]);
			break;
		case ACT_ADD_REG:
			output_byte((ins->operand[pattern->register_operand].base & 7) + pattern->action[++action_index]);
			break;
		case ACT_ADD_FCMOV:
			output_byte((ins->condition >> 2) + pattern->action[++action_index]);
			output_byte((ins->operand[pattern->register_operand].base & 7) + ((ins->condition & 3) << 3) + pattern->action[++action_index]);
			break;
		case ACT_F16_0:
			evaluate_expression(ins->operand[0].parameter, ref, ins->code_offset);
			output_word(ref, DATA_LE, 2);
			int_clear(ref->value);
			evaluate_expression(ins->operand[0].segment_value, ref, ins->code_offset);
			output_word(ref, DATA_LE, 2);
			int_clear(ref->value);
			break;
		case ACT_F32_0:
			evaluate_expression(ins->operand[0].parameter, ref, ins->code_offset);
			output_word(ref, DATA_LE, 4);
			int_clear(ref->value);
			evaluate_expression(ins->operand[0].segment_value, ref, ins->code_offset);
			output_word(ref, DATA_LE, 2);
			int_clear(ref->value);
			break;
		}
	}
}

extern size_t x80_instruction_compute_length(instruction_t * ins, bool forgiving);
extern size_t x89_instruction_compute_length(instruction_t * ins, bool forgiving);

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
		case ERROR_TYPE_NO64:
			fprintf(stderr, "Instruction not supported in 64-bit mode\n");
			break;
		case ERROR_TYPE_LM64:
			fprintf(stderr, "Instruction only supported in 64-bit mode\n");
			break;
		case ERROR_TYPE_WRONG_FPU:
			fprintf(stderr, "Instruction not supported on specified floating point coprocesor\n");
			break;
		case ERROR_TYPE_OPSIZE_MISMATCH:
			fprintf(stderr, "Operand size mismatch\n");
			break;
		case ERROR_TYPE_OPSIZE_MISSSING:
			fprintf(stderr, "Operation size missing\n");
			break;
		case ERROR_TYPE_OPSIZE_INVALID:
			fprintf(stderr, "Invalid operation size\n");
			break;
		case ERROR_TYPE_ADSIZE_INVALID:
			fprintf(stderr, "Invalid address size\n");
			break;
		case ERROR_TYPE_SEGMENT_MISMATCH:
			fprintf(stderr, "Segment prefix mismatch\n");
			break;
		case ERROR_TYPE_INVALID_ADDRESS:
			fprintf(stderr, "Invalid addressing mode\n");
			break;
		case ERROR_TYPE_INVALID_ADDRESS_NON64:
			fprintf(stderr, "Invalid addressing mode in non-64-bit mode\n");
			break;
		case ERROR_TYPE_INVALID_REX_OPERAND:
			fprintf(stderr, "Operand not allowed with REX prefix\n");
			break;
		}
	}
}

size_t x86_instruction_compute_length(instruction_t * ins, bool forgiving)
{
	ssize_t result = -ERROR_TYPE_INTERNAL;
	uint64_t error_types = 0;
	switch(ins->isa)
	{
	case ISA_X86:
		{
			const instruction_pattern_t * pattern = find_pattern(ins, forgiving, &error_types);
			if(pattern == NULL)
			{
				report_errors(ins->line_number, error_types);
				//fprintf(stderr, "Line %ld: Invalid instruction\n", ins->line_number);
				//return RESULT_FAILED; // TODO: should return RESULT_FAILED
				return (size_t)-1;
			}
			result = instruction_pattern_get_length(pattern, ins, forgiving);
		}
		break;
	case ISA_X80:
		return x80_instruction_compute_length(ins, forgiving);
	case ISA_8089:
		return x89_instruction_compute_length(ins, forgiving);
	}

	if(result < 0)
	{
		report_errors(ins->line_number, 1 << -result);
		return (size_t)-1;
	}
	return result;
}

extern void x80_generate_instruction(instruction_t * ins);
extern void x89_generate_instruction(instruction_t * ins);

void x86_generate_instruction(instruction_t * ins)
{
	switch(ins->isa)
	{
	case ISA_X86:
		{
			const instruction_pattern_t * pattern = find_pattern(ins, false, NULL);

			instruction_pattern_generate(pattern, ins);
		}
		break;
	case ISA_X80:
		return x80_generate_instruction(ins);
	case ISA_8089:
		return x89_generate_instruction(ins);
	}
}

static const instruction_pattern_t pattern_aaa_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_NO64, NO_MODRM,
		{ },
		{ ACT_PUT, 0x37 },
	},
};

static const instruction_pattern_t pattern_aad_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_NO64, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD5, 0x0A },
	},
};

static const instruction_pattern_t pattern_aad_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_NO64, NO_MODRM,
		{ CST_UI8 | CST_NOSIZE },
		{ ACT_PUT, 0xD5, ACT_I8_0 },
	},
};

static const instruction_pattern_t pattern_aam_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_NO64, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD4, 0x0A },
	},
};

static const instruction_pattern_t pattern_aam_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_NO64, NO_MODRM,
		{ CST_UI8 | CST_NOSIZE },
		{ ACT_PUT, 0xD4, ACT_I8_0 },
	},
};

static const instruction_pattern_t pattern_aas_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_NO64, NO_MODRM,
		{ },
		{ ACT_PUT, 0x3F },
	},
};

#define _GENERATE_ADC_LIKE(__name, __value) \
static const instruction_pattern_t pattern_##__name##_2[] = \
{ \
	{ \
		CPU_8086, CPU_ALL, \
		0, NO_MODRM, \
		{ CST_AL, CST_I8 }, \
		{ ACT_PUT, 0x04 | ((__value) << 3), ACT_I8_1 }, \
	}, \
	{ \
		CPU_8086, CPU_ALL, \
		INS_O16, NO_MODRM, \
		{ CST_AX, CST_I16 }, \
		{ ACT_PUT, 0x05 | ((__value) << 3), ACT_I16_1 }, \
	}, \
	{ \
		CPU_386, CPU_ALL, \
		INS_O32, NO_MODRM, \
		{ CST_EAX, CST_I32 }, \
		{ ACT_PUT, 0x05 | ((__value) << 3), ACT_I32_1 }, \
	}, \
	{ \
		CPU_X64, CPU_ALL, \
		INS_O64, NO_MODRM, \
		{ CST_RAX, CST_SI32 }, \
		{ ACT_PUT, 0x05 | ((__value) << 3), ACT_I32_1 }, \
	}, \
	{ \
		CPU_8086, CPU_ALL, \
		INS_O8 | INS_NEEDSIZE, MODRM1(0), \
		{ CST_RM8, CST_I8 }, \
		{ ACT_PUT, 0x80, ACT_MEM_I, 2, ACT_I8_1 }, \
	}, \
	{ \
		CPU_8086, CPU_ALL, \
		INS_O16 | INS_NEEDSIZE, MODRM1(0), \
		{ CST_RM16, CST_SI8 | CST_NOTRUNC }, \
		{ ACT_PUT, 0x83, ACT_MEM_I, 2, ACT_I8_1 }, \
	}, \
	{ \
		CPU_8086, CPU_ALL, \
		INS_O16 | INS_NEEDSIZE, MODRM1(0), \
		{ CST_RM16, CST_I16 }, \
		{ ACT_PUT, 0x81, ACT_MEM_I, 2, ACT_I16_1 }, \
	}, \
	{ \
		CPU_386, CPU_ALL, \
		INS_O32 | INS_NEEDSIZE, MODRM1(0), \
		{ CST_RM32, CST_SI8 | CST_NOTRUNC }, \
		{ ACT_PUT, 0x83, ACT_MEM_I, 2, ACT_I8_1 }, \
	}, \
	{ \
		CPU_386, CPU_ALL, \
		INS_O32 | INS_NEEDSIZE, MODRM1(0), \
		{ CST_RM32, CST_I32 }, \
		{ ACT_PUT, 0x81, ACT_MEM_I, 2, ACT_I32_1 }, \
	}, \
	{ \
		CPU_X64, CPU_ALL, \
		INS_O64 | INS_NEEDSIZE, MODRM1(0), \
		{ CST_RM64, CST_SI8 | CST_NOTRUNC }, \
		{ ACT_PUT, 0x83, ACT_MEM_I, 2, ACT_I8_1 }, \
	}, \
	{ \
		CPU_X64, CPU_ALL, \
		INS_O64 | INS_NEEDSIZE, MODRM1(0), \
		{ CST_RM64, CST_SI32 }, \
		{ ACT_PUT, 0x81, ACT_MEM_I, 2, ACT_I32_1 }, \
	}, \
	{ \
		CPU_8086, CPU_ALL, \
		0, MODRM2(0, 1), \
		{ CST_RM8, CST_R8 }, \
		{ ACT_PUT, 0x00 | ((__value) << 3), ACT_MEM }, \
	}, \
	{ \
		CPU_8086, CPU_ALL, \
		INS_O16, MODRM2(0, 1), \
		{ CST_RM16, CST_R16 }, \
		{ ACT_PUT, 0x01 | ((__value) << 3), ACT_MEM }, \
	}, \
	{ \
		CPU_386, CPU_ALL, \
		INS_O32, MODRM2(0, 1), \
		{ CST_RM32, CST_R32 }, \
		{ ACT_PUT, 0x01 | ((__value) << 3), ACT_MEM }, \
	}, \
	{ \
		CPU_X64, CPU_ALL, \
		INS_O64, MODRM2(0, 1), \
		{ CST_RM64, CST_R64 }, \
		{ ACT_PUT, 0x01 | ((__value) << 3), ACT_MEM }, \
	}, \
	{ \
		CPU_8086, CPU_ALL, \
		0, MODRM2(1, 0), \
		{ CST_R8, CST_RM8 }, \
		{ ACT_PUT, 0x02 | ((__value) << 3), ACT_MEM }, \
	}, \
	{ \
		CPU_8086, CPU_ALL, \
		INS_O16, MODRM2(1, 0), \
		{ CST_R16, CST_RM16 }, \
		{ ACT_PUT, 0x03 | ((__value) << 3), ACT_MEM }, \
	}, \
	{ \
		CPU_386, CPU_ALL, \
		INS_O32, MODRM2(1, 0), \
		{ CST_R32, CST_RM32 }, \
		{ ACT_PUT, 0x03 | ((__value) << 3), ACT_MEM }, \
	}, \
	{ \
		CPU_X64, CPU_ALL, \
		INS_O64, MODRM2(1, 0), \
		{ CST_R64, CST_RM64 }, \
		{ ACT_PUT, 0x03 | ((__value) << 3), ACT_MEM }, \
	}, \
}

_GENERATE_ADC_LIKE(add, 0);
_GENERATE_ADC_LIKE(adc, 2);
_GENERATE_ADC_LIKE(and, 4);
_GENERATE_ADC_LIKE(cmp, 7);
_GENERATE_ADC_LIKE(or, 1);
_GENERATE_ADC_LIKE(sbb, 3);
_GENERATE_ADC_LIKE(sub, 5);
_GENERATE_ADC_LIKE(xor, 6);

static const instruction_pattern_t pattern_add4s_0[] =
{
	{
		CPU_V30, CPU_V55,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x20 },
	},
};

static const instruction_pattern_t pattern_add4s_2[] =
{
	{
		CPU_V30, CPU_V55,
		0, NO_MODRM,
		{ CST_ESDI, CST_DSSI },
		{ ACT_PUT2, 0x0F, 0x20 },
	},
};

static const instruction_pattern_t pattern_albit_0[] =
{
	{
		CPU_V55, CPU_V55,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x9A },
	},
};

static const instruction_pattern_t pattern_arpl_2[] =
{
	{
		CPU_286, CPU_ALL,
		INS_O16, MODRM2(0, 1),
		{ CST_RM16, CST_R16 },
		{ ACT_PUT, 0x63, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_bb0_reset_0[] =
{
	{
		CPU_GX, CPU_GX,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x3A },
	},
};

static const instruction_pattern_t pattern_bb1_reset_0[] =
{
	{
		CPU_GX, CPU_GX,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x3B },
	},
};

static const instruction_pattern_t pattern_bound_2[] =
{
	{
		CPU_186, CPU_ALL,
		INS_O16 | INS_NO64, MODRM2(1, 0),
		{ CST_R16, CST_M16 },
		{ ACT_PUT, 0x62, ACT_MEM },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NO64, MODRM2(1, 0),
		{ CST_R32, CST_M32 },
		{ ACT_PUT, 0x62, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_brk_1[] =
{
	{
		CPU_8086, CPU_ALL,
		0, NO_MODRM,
		{ CST_EQ3 | CST_NOSIZE },
		{ ACT_PUT, 0xCC },
	},
	{
		CPU_8086, CPU_ALL,
		0, NO_MODRM,
		{ CST_UI8 | CST_NOSIZE },
		{ ACT_PUT, 0xCD, ACT_I8_0 },
	},
};

static const instruction_pattern_t pattern_brkcs_1[] =
{
	{
		CPU_V25, CPU_V55,
		0, MODRM1(0),
		{ CST_R16 },
		{ ACT_PUT2, 0x0F, 0x2D, ACT_MEM_I, 0 },
	},
};

static const instruction_pattern_t pattern_brkem_1[] =
{
	{
		CPU_V30, CPU_9002,
		0, NO_MODRM,
		{ CST_UI8 | CST_NOSIZE },
		{ ACT_PUT2, 0x0F, 0xFF, ACT_I8_0 },
	},
};

static const instruction_pattern_t pattern_brkem2_1[] =
{
	{
		CPU_V30, CPU_9002,
		0, NO_MODRM,
		{ CST_UI8 | CST_NOSIZE },
		{ ACT_PUT2, 0x0F, 0xFE, ACT_I8_0 },
	},
};

static const instruction_pattern_t pattern_brkn_1[] =
{
	{
		CPU_V25, CPU_V25,
		0, NO_MODRM,
		{ CST_UI8 | CST_NOSIZE },
		{ ACT_PUT, 0x63, ACT_I8_0 },
	},
};

static const instruction_pattern_t pattern_brks_1[] =
{
	{
		CPU_V25, CPU_V25,
		0, NO_MODRM,
		{ CST_UI8 | CST_NOSIZE },
		{ ACT_PUT, 0xF1, ACT_I8_0 },
	},
};

static const instruction_pattern_t pattern_brkxa_1[] =
{
	{
		CPU_V33, CPU_V33,
		0, NO_MODRM,
		{ CST_UI8 | CST_NOSIZE },
		{ ACT_PUT2, 0x0F, 0xE0, ACT_I8_0 },
	},
};

static const instruction_pattern_t pattern_bsch_1[] =
{
	{
		CPU_V55, CPU_V55,
		INS_O8 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM8 },
		{ ACT_PUT2, 0x0F, 0x3C, ACT_MEM_I, 0 },
	},
	{
		CPU_V55, CPU_V55,
		INS_O16 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM16 },
		{ ACT_PUT2, 0x0F, 0x3D, ACT_MEM_I, 0 },
	},
};

static const instruction_pattern_t pattern_bsf_2[] =
{
	{
		CPU_386, CPU_ALL,
		INS_O16, MODRM2(1, 0),
		{ CST_R16, CST_RM16 },
		{ ACT_PUT2, 0x0F, 0xBC, ACT_MEM },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, MODRM2(1, 0),
		{ CST_R32, CST_RM32 },
		{ ACT_PUT2, 0x0F, 0xBC, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(1, 0),
		{ CST_R64, CST_RM64 },
		{ ACT_PUT2, 0x0F, 0xBC, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_bsr_2[] =
{
	{
		CPU_386, CPU_ALL,
		INS_O16, MODRM2(1, 0),
		{ CST_R16, CST_RM16 },
		{ ACT_PUT2, 0x0F, 0xBD, ACT_MEM },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, MODRM2(1, 0),
		{ CST_R32, CST_RM32 },
		{ ACT_PUT2, 0x0F, 0xBD, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(1, 0),
		{ CST_R64, CST_RM64 },
		{ ACT_PUT2, 0x0F, 0xBD, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_bswap_1[] =
{
	{
		CPU_486, CPU_ALL,
		INS_O32, REGOPD(0),
		{ CST_R32 },
		{ ACT_PUT, 0x0F, ACT_ADD_REG, 0xC8 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, REGOPD(0),
		{ CST_R64 },
		{ ACT_PUT, 0x0F, ACT_ADD_REG, 0xC8 },
	},
};

#define _GENERATE_BT_LIKE(__name, __value) \
static const instruction_pattern_t pattern_##__name##_2[] = \
{ \
	{ \
		CPU_386, CPU_ALL, \
		INS_O16, MODRM2(0, 1), \
		{ CST_RM16, CST_R16 }, \
		{ ACT_PUT2, 0x0F, 0x83 + ((__value) << 3), ACT_MEM }, \
	}, \
	{ \
		CPU_386, CPU_ALL, \
		INS_O32, MODRM2(0, 1), \
		{ CST_RM32, CST_R32 }, \
		{ ACT_PUT2, 0x0F, 0x83 + ((__value) << 3), ACT_MEM }, \
	}, \
	{ \
		CPU_X64, CPU_ALL, \
		INS_O64, MODRM2(0, 1), \
		{ CST_RM64, CST_R64 }, \
		{ ACT_PUT2, 0x0F, 0x83 + ((__value) << 3), ACT_MEM }, \
	}, \
	{ \
		CPU_386, CPU_ALL, \
		INS_O16 | INS_NEEDSIZE, MODRM1(0), \
		{ CST_RM16, CST_UI8 | CST_NOSIZE }, \
		{ ACT_PUT2, 0x0F, 0xBA, ACT_MEM_I, (__value), ACT_I8_1 }, \
	}, \
	{ \
		CPU_386, CPU_ALL, \
		INS_O32 | INS_NEEDSIZE, MODRM1(0), \
		{ CST_RM32, CST_UI8 | CST_NOSIZE }, \
		{ ACT_PUT2, 0x0F, 0xBA, ACT_MEM_I, (__value), ACT_I8_1 }, \
	}, \
	{ \
		CPU_X64, CPU_ALL, \
		INS_O64 | INS_NEEDSIZE, MODRM1(0), \
		{ CST_RM64, CST_UI8 | CST_NOSIZE }, \
		{ ACT_PUT2, 0x0F, 0xBA, ACT_MEM_I, (__value), ACT_I8_1 }, \
	}, \
}
_GENERATE_BT_LIKE(bt, 4);
_GENERATE_BT_LIKE(btc, 7);
_GENERATE_BT_LIKE(btr, 6);
_GENERATE_BT_LIKE(bts, 5);

static const instruction_pattern_t pattern_btclr_3[] =
{
	{
		CPU_V25, CPU_V55,
		0, NO_MODRM,
		{ CST_UI8 | CST_NOSIZE, CST_UI8 | CST_NOSIZE, CST_REL8 },
		{ ACT_PUT2, 0x0F, 0x9C, ACT_I8_0, ACT_I8_1, ACT_R8_2 },
	},
};

static const instruction_pattern_t pattern_btclrl_3[] =
{
	{
		CPU_V55, CPU_V55,
		0, NO_MODRM,
		{ CST_UI8 | CST_NOSIZE, CST_UI8 | CST_NOSIZE, CST_REL8 },
		{ ACT_PUT2, 0x0F, 0x9D, ACT_I8_0, ACT_I8_1, ACT_R8_2 },
	},
};

static const instruction_pattern_t pattern_call_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_OB | INS_OD64, NO_MODRM,
		{ CST_REL16 },
		{ ACT_PUT, 0xE8, ACT_R16_0 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_OB | INS_NO64, NO_MODRM,
		{ CST_REL32 },
		{ ACT_PUT, 0xE8, ACT_R32_0 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_OB | INS_OD64, NO_MODRM,
		{ CST_REL32 },
		{ ACT_PUT, 0xE8, ACT_R32_0 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_OB | INS_OD64 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM16 },
		{ ACT_PUT, 0xFF, ACT_MEM_I, 2 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_OB | INS_NO64 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM32 },
		{ ACT_PUT, 0xFF, ACT_MEM_I, 2 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_OB | INS_OD64 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM64 },
		{ ACT_PUT, 0xFF, ACT_MEM_I, 2 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_OB | INS_NO64 | INS_NEEDSIZE, NO_MODRM,
		{ CST_FAR16 },
		{ ACT_PUT, 0x9A, ACT_F16_0 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_OB | INS_NO64 | INS_NEEDSIZE, NO_MODRM,
		{ CST_FAR32 },
		{ ACT_PUT, 0x9A, ACT_F32_0 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_OB | INS_OD64 | INS_NEEDSIZE, MODRM1(0),
		{ CST_FARM16 },
		{ ACT_PUT, 0xFF, ACT_MEM_I, 3 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_OB | INS_NO64 | INS_NEEDSIZE, MODRM1(0),
		{ CST_FARM32 },
		{ ACT_PUT, 0xFF, ACT_MEM_I, 3 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_OB | INS_OD64 | INS_NEEDSIZE, MODRM1(0),
		{ CST_FARM64 },
		{ ACT_PUT, 0xFF, ACT_MEM_I, 3 },
	},
};

static const instruction_pattern_t pattern_cbw_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16, NO_MODRM,
		{ },
		{ ACT_PUT, 0x98 },
	},
};

static const instruction_pattern_t pattern_cwde_0[] =
{
	{
		CPU_386, CPU_ALL,
		INS_O32, NO_MODRM,
		{ },
		{ ACT_PUT, 0x98 },
	},
};

static const instruction_pattern_t pattern_cdqe_0[] =
{
	{
		CPU_X64, CPU_ALL,
		INS_O64, NO_MODRM,
		{ },
		{ ACT_PUT, 0x98 },
	},
};

static const instruction_pattern_t pattern_clc_0[] =
{
	{
		CPU_8086, CPU_ALL,
		0, NO_MODRM,
		{ },
		{ ACT_PUT, 0xF8 },
	},
};

static const instruction_pattern_t pattern_cld_0[] =
{
	{
		CPU_8086, CPU_ALL,
		0, NO_MODRM,
		{ },
		{ ACT_PUT, 0xFC },
	},
};

static const instruction_pattern_t pattern_cli_0[] =
{
	{
		CPU_8086, CPU_ALL,
		0, NO_MODRM,
		{ },
		{ ACT_PUT, 0xFA },
	},
};

static const instruction_pattern_t pattern_clr1_1[] =
{
	{
		CPU_V30, CPU_V55,
		0, NO_MODRM,
		{ CST_CY },
		{ ACT_PUT, 0xF8 },
	},
	{
		CPU_V30, CPU_V55,
		0, NO_MODRM,
		{ CST_DIR },
		{ ACT_PUT, 0xFC },
	},
};

#define _GENERATE_CLR1_LIKE(__name, __value) \
static const instruction_pattern_t pattern_##__name##_2[] = \
{ \
	{ \
		CPU_V30, CPU_V55, \
		INS_O8 | INS_NEEDSIZE, MODRM1(0), \
		{ CST_RM8, CST_CL | CST_IMPLSIZE }, \
		{ ACT_PUT2, 0x0F, 0x10 + (__value), ACT_MEM_I, 0 }, \
	}, \
	{ \
		CPU_V30, CPU_V55, \
		INS_O16 | INS_NEEDSIZE, MODRM1(0), \
		{ CST_RM16, CST_CL | CST_IMPLSIZE}, \
		{ ACT_PUT2, 0x0F, 0x11 + (__value), ACT_MEM_I, 0 }, \
	}, \
	{ \
		CPU_V30, CPU_V55, \
		INS_O8 | INS_NEEDSIZE, MODRM1(0), \
		{ CST_RM8, CST_UI8 | CST_NOSIZE }, \
		{ ACT_PUT2, 0x0F, 0x18 + (__value), ACT_MEM_I, 0, ACT_I8_1 }, \
	}, \
	{ \
		CPU_V30, CPU_V55, \
		INS_O16 | INS_NEEDSIZE, MODRM1(0), \
		{ CST_RM16, CST_UI8 | CST_NOSIZE }, \
		{ ACT_PUT2, 0x0F, 0x19 + (__value), ACT_MEM_I, 0, ACT_I8_1 }, \
	}, \
}
_GENERATE_CLR1_LIKE(clr1, 0x02);
_GENERATE_CLR1_LIKE(not1, 0x06);
_GENERATE_CLR1_LIKE(set1, 0x04);
_GENERATE_CLR1_LIKE(test1, 0x00);

static const instruction_pattern_t pattern_clts_0[] =
{
	{
		CPU_286, CPU_ALL,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x06 },
	},
};

static const instruction_pattern_t pattern_cmc_0[] =
{
	{
		CPU_8086, CPU_ALL,
		0, NO_MODRM,
		{ },
		{ ACT_PUT, 0xF5 },
	},
};

static const instruction_pattern_t pattern_cmov_cc_2[] =
{
	{
		CPU_686, CPU_ALL,
		INS_O16, MODRM2(1, 0),
		{ CST_R16, CST_RM16 },
		{ ACT_PUT, 0x0F, ACT_ADD_COND, 0x40, ACT_MEM },
	},
	{
		CPU_686, CPU_ALL,
		INS_O32, MODRM2(1, 0),
		{ CST_R32, CST_RM32 },
		{ ACT_PUT, 0x0F, ACT_ADD_COND, 0x40, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(1, 0),
		{ CST_R64, CST_RM64 },
		{ ACT_PUT, 0x0F, ACT_ADD_COND, 0x40, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_cmp4s_0[] =
{
	{
		CPU_V30, CPU_V55,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x20 },
	},
};

static const instruction_pattern_t pattern_cmp4s_2[] =
{
	{
		CPU_V30, CPU_V55,
		0, NO_MODRM,
		{ CST_ESDI, CST_DSSI },
		{ ACT_PUT2, 0x0F, 0x26 },
	},
};

static const instruction_pattern_t pattern_cmps_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O8 | INS_NEEDSIZE, NO_MODRM,
		{ },
		{ ACT_PUT, 0xA6 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_NEEDSIZE, NO_MODRM,
		{ },
		{ ACT_PUT, 0xA7 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NEEDSIZE, NO_MODRM,
		{ },
		{ ACT_PUT, 0xA7 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_NEEDSIZE, NO_MODRM,
		{ },
		{ ACT_PUT, 0xA7 },
	},
};

static const instruction_pattern_t pattern_cmps_2[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O8 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI, CST_DSSI },
		{ ACT_PUT, 0xA6 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI, CST_DSSI },
		{ ACT_PUT, 0xA7 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI, CST_DSSI },
		{ ACT_PUT, 0xA7 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI, CST_DSSI },
		{ ACT_PUT, 0xA7 },
	},
};

static const instruction_pattern_t pattern_cmpsb_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O8, NO_MODRM,
		{ },
		{ ACT_PUT, 0xA6 },
	},
};

static const instruction_pattern_t pattern_cmpsb_2[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O8 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI, CST_DSSI },
		{ ACT_PUT, 0xA6 },
	},
};

static const instruction_pattern_t pattern_cmpsw_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16, NO_MODRM,
		{ },
		{ ACT_PUT, 0xA7 },
	},
};

static const instruction_pattern_t pattern_cmpsw_2[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI, CST_DSSI },
		{ ACT_PUT, 0xA7 },
	},
};

static const instruction_pattern_t pattern_cmpsd_0[] =
{
	{
		CPU_386, CPU_ALL,
		INS_O32, NO_MODRM,
		{ },
		{ ACT_PUT, 0xA7 },
	},
};

static const instruction_pattern_t pattern_cmpsd_2[] =
{
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI, CST_DSSI },
		{ ACT_PUT, 0xA7 },
	},
};

static const instruction_pattern_t pattern_cmpsq_0[] =
{
	{
		CPU_X64, CPU_ALL,
		INS_O64, NO_MODRM,
		{ },
		{ ACT_PUT, 0xA7 },
	},
};

static const instruction_pattern_t pattern_cmpsq_2[] =
{
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI, CST_DSSI },
		{ ACT_PUT, 0xA7 },
	},
};

static const instruction_pattern_t pattern_cmpxchg_2[] =
{
	{
		CPU_486, CPU_486,
		INS_O8, MODRM2(0, 1),
		{ CST_RM8, CST_R8 },
		{ ACT_PUT2, 0x0F, 0xA6, ACT_MEM },
	},
	{
		CPU_486, CPU_486,
		INS_O16, MODRM2(0, 1),
		{ CST_RM16, CST_R16 },
		{ ACT_PUT2, 0x0F, 0xA7, ACT_MEM },
	},
	{
		CPU_486, CPU_486,
		INS_O32, MODRM2(0, 1),
		{ CST_RM32, CST_R32 },
		{ ACT_PUT2, 0x0F, 0xA7, ACT_MEM },
	},
	{
		CPU_486B, CPU_ALL,
		INS_O8, MODRM2(0, 1),
		{ CST_RM8, CST_R8 },
		{ ACT_PUT2, 0x0F, 0xB0, ACT_MEM },
	},
	{
		CPU_486B, CPU_ALL,
		INS_O16, MODRM2(0, 1),
		{ CST_RM16, CST_R16 },
		{ ACT_PUT2, 0x0F, 0xB1, ACT_MEM },
	},
	{
		CPU_486B, CPU_ALL,
		INS_O32, MODRM2(0, 1),
		{ CST_RM32, CST_R32 },
		{ ACT_PUT2, 0x0F, 0xB1, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(0, 1),
		{ CST_RM64, CST_R64 },
		{ ACT_PUT2, 0x0F, 0xB1, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_cmpxchg8b_1[] =
{
	{
		CPU_586, CPU_ALL,
		0, MODRM1(0),
		{ CST_RM64 },
		{ ACT_PUT2, 0x0F, 0xC7, ACT_MEM_I, 7 },
	},
};

static const instruction_pattern_t pattern_cmpxchg16b_1[] =
{
	{
		CPU_586, CPU_ALL,
		INS_O64, MODRM1(0),
		{ CST_RM64 },
		{ ACT_PUT2, 0x0F, 0xC7, ACT_MEM_I, 7 },
	},
};

static const instruction_pattern_t pattern_cnvtrp_0[] =
{
	{
		CPU_V55, CPU_V55,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x7A },
	},
};

static const instruction_pattern_t pattern_coltrp_0[] =
{
	{
		CPU_V55, CPU_V55,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x9B },
	},
};

static const instruction_pattern_t pattern_cpu_read_0[] =
{
	{
		CPU_GX, CPU_GX,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x3D },
	},
};

static const instruction_pattern_t pattern_cpu_write_0[] =
{
	{
		CPU_GX, CPU_GX,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x3C },
	},
};

static const instruction_pattern_t pattern_cpuid_0[] =
{
	{
		CPU_586, CPU_ALL,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0xA2 },
	},
};

static const instruction_pattern_t pattern_cwd_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16, NO_MODRM,
		{ },
		{ ACT_PUT, 0x99 },
	},
};

static const instruction_pattern_t pattern_cdq_0[] =
{
	{
		CPU_386, CPU_ALL,
		INS_O32, NO_MODRM,
		{ },
		{ ACT_PUT, 0x99 },
	},
};

static const instruction_pattern_t pattern_cqo_0[] =
{
	{
		CPU_X64, CPU_ALL,
		INS_O64, NO_MODRM,
		{ },
		{ ACT_PUT, 0x99 },
	},
};

static const instruction_pattern_t pattern_daa_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_NO64, NO_MODRM,
		{ },
		{ ACT_PUT, 0x27 },
	},
};

static const instruction_pattern_t pattern_das_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_NO64, NO_MODRM,
		{ },
		{ ACT_PUT, 0x2F },
	},
};

static const instruction_pattern_t pattern_dec_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16, REGOPD(0),
		{ CST_R16 },
		{ ACT_ADD_REG, 0x48 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, REGOPD(0),
		{ CST_R32 },
		{ ACT_ADD_REG, 0x48 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, REGOPD(0),
		{ CST_R64 },
		{ ACT_ADD_REG, 0x48 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O8 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM8 },
		{ ACT_PUT, 0xFE, ACT_MEM_I, 1 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM16 },
		{ ACT_PUT, 0xFF, ACT_MEM_I, 1 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM32 },
		{ ACT_PUT, 0xFF, ACT_MEM_I, 1 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM64 },
		{ ACT_PUT, 0xFF, ACT_MEM_I, 1 },
	},
};

static const instruction_pattern_t pattern_div_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O8 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM8 },
		{ ACT_PUT, 0xF6, ACT_MEM_I, 6 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM16 },
		{ ACT_PUT, 0xF7, ACT_MEM_I, 6 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM32 },
		{ ACT_PUT, 0xF7, ACT_MEM_I, 6 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM64 },
		{ ACT_PUT, 0xF7, ACT_MEM_I, 6 },
	},
};

static const instruction_pattern_t pattern_dmint_0[] =
{
	{
		CPU_GX2, CPU_GX2,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x39 },
	},
};

static const instruction_pattern_t pattern_enter_2[] =
{
	{
		CPU_186, CPU_ALL,
		0, NO_MODRM,
		{ CST_UI16 | CST_NOSIZE, CST_UI8 | CST_NOSIZE },
		{ ACT_PUT, 0xC8, ACT_I16_0, ACT_I8_1 },
	},
};

static const instruction_pattern_t pattern_ext_2[] =
{
	{
		CPU_V30, CPU_V55,
		0, MODRM2(0, 1),
		{ CST_R8, CST_R8 },
		{ ACT_PUT2, 0x0F, 0x33, ACT_MEM },
	},
	{
		CPU_V30, CPU_V55,
		0, MODRM1(0),
		{ CST_R8, CST_UI8 | CST_NOSIZE },
		{ ACT_PUT2, 0x0F, 0x3B, ACT_MEM_I, 0 },
	},
};

static const instruction_pattern_t pattern_fint_0[] =
{
	{
		CPU_V25, CPU_V55,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x92 },
	},
};

static const instruction_pattern_t pattern_getbit_0[] =
{
	{
		CPU_V55, CPU_V55,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x79 },
	},
};

static const instruction_pattern_t pattern_hlt_0[] =
{
	{
		CPU_8086, CPU_ALL,
		0, NO_MODRM,
		{ },
		{ ACT_PUT, 0xF4 },
	},
};

static const instruction_pattern_t pattern_ibts_4[] =
{
	{
		CPU_386, CPU_386,
		INS_O16, MODRM2(0, 3),
		{ CST_RM16, CST_AX, CST_CL | CST_IMPLSIZE, CST_R16 },
		{ ACT_PUT2, 0x0F, 0xA7, ACT_MEM },
	},
	{
		CPU_386, CPU_386,
		INS_O32, MODRM2(0, 3),
		{ CST_RM32, CST_EAX, CST_CL | CST_IMPLSIZE, CST_R32 },
		{ ACT_PUT2, 0x0F, 0xA7, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_idiv_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O8 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM8 },
		{ ACT_PUT, 0xF6, ACT_MEM_I, 7 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM16 },
		{ ACT_PUT, 0xF7, ACT_MEM_I, 7 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM32 },
		{ ACT_PUT, 0xF7, ACT_MEM_I, 7 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM64 },
		{ ACT_PUT, 0xF7, ACT_MEM_I, 7 },
	},
};

static const instruction_pattern_t pattern_idle_0[] =
{
	{
		CPU_V55, CPU_V55,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x9F },
	},
};

static const instruction_pattern_t pattern_imul_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O8 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM8 },
		{ ACT_PUT, 0xF6, ACT_MEM_I, 5 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM16 },
		{ ACT_PUT, 0xF7, ACT_MEM_I, 5 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM32 },
		{ ACT_PUT, 0xF7, ACT_MEM_I, 5 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM64 },
		{ ACT_PUT, 0xF7, ACT_MEM_I, 5 },
	},
};

static const instruction_pattern_t pattern_imul_2[] =
{
	{
		CPU_386, CPU_ALL,
		INS_O16, MODRM2(1, 0),
		{ CST_R16, CST_RM16 },
		{ ACT_PUT2, 0x0F, 0xAE, ACT_MEM },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, MODRM2(1, 0),
		{ CST_R32, CST_RM32 },
		{ ACT_PUT2, 0x0F, 0xAE, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(1, 0),
		{ CST_R64, CST_RM64 },
		{ ACT_PUT2, 0x0F, 0xAE, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_mul_nec_2[] =
{
	{
		CPU_186, CPU_ALL,
		INS_O16, MODRM2(0, 0),
		{ CST_R16, CST_SI8 | CST_NOTRUNC },
		{ ACT_PUT, 0x6B, ACT_MEM, ACT_I8_1 },
	},
	{
		CPU_186, CPU_ALL,
		INS_O16, MODRM2(0, 0),
		{ CST_R16, CST_I16 },
		{ ACT_PUT, 0x69, ACT_MEM, ACT_I16_1 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, MODRM2(0, 0),
		{ CST_R32, CST_SI8 | CST_NOTRUNC },
		{ ACT_PUT, 0x6B, ACT_MEM, ACT_I8_1 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, MODRM2(0, 0),
		{ CST_R32, CST_I32 },
		{ ACT_PUT, 0x69, ACT_MEM, ACT_I32_1 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(0, 0),
		{ CST_R64, CST_SI8 | CST_NOTRUNC },
		{ ACT_PUT, 0x6B, ACT_MEM, ACT_I8_1 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(0, 0),
		{ CST_R64, CST_SI32 },
		{ ACT_PUT, 0x69, ACT_MEM, ACT_I32_1 },
	},
};

static const instruction_pattern_t pattern_imul_3[] =
{
	{
		CPU_186, CPU_ALL,
		INS_O16, MODRM2(1, 0),
		{ CST_R16, CST_RM16, CST_SI8 | CST_NOTRUNC },
		{ ACT_PUT, 0x6B, ACT_MEM, ACT_I8_2 },
	},
	{
		CPU_186, CPU_ALL,
		INS_O16, MODRM2(1, 0),
		{ CST_R16, CST_RM16, CST_I16 },
		{ ACT_PUT, 0x69, ACT_MEM, ACT_I16_2 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, MODRM2(1, 0),
		{ CST_R32, CST_RM32, CST_SI8 | CST_NOTRUNC },
		{ ACT_PUT, 0x6B, ACT_MEM, ACT_I8_2 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, MODRM2(1, 0),
		{ CST_R32, CST_RM32, CST_I32 },
		{ ACT_PUT, 0x69, ACT_MEM, ACT_I32_2 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(1, 0),
		{ CST_R64, CST_RM64, CST_SI8 | CST_NOTRUNC },
		{ ACT_PUT, 0x6B, ACT_MEM, ACT_I8_2 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(1, 0),
		{ CST_R64, CST_RM64, CST_SI32 },
		{ ACT_PUT, 0x69, ACT_MEM, ACT_I32_2 },
	},
};

static const instruction_pattern_t pattern_in_2[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O8, NO_MODRM,
		{ CST_AL, CST_UI8 | CST_NOTRUNC | CST_NOSIZE },
		{ ACT_PUT, 0xE4, ACT_I8_1 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16, NO_MODRM,
		{ CST_AX, CST_UI8 | CST_NOTRUNC | CST_NOSIZE },
		{ ACT_PUT, 0xE5, ACT_I8_1 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, NO_MODRM,
		{ CST_EAX, CST_UI8 | CST_NOTRUNC | CST_NOSIZE },
		{ ACT_PUT, 0xE5, ACT_I8_1 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O8, NO_MODRM,
		{ CST_AL, CST_DX | CST_IMPLSIZE },
		{ ACT_PUT, 0xEC },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16, NO_MODRM,
		{ CST_AX, CST_DX | CST_IMPLSIZE },
		{ ACT_PUT, 0xED },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, NO_MODRM,
		{ CST_EAX, CST_DX | CST_IMPLSIZE },
		{ ACT_PUT, 0xED },
	},
};

static const instruction_pattern_t pattern_inc_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16, REGOPD(0),
		{ CST_R16 },
		{ ACT_ADD_REG, 0x40 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, REGOPD(0),
		{ CST_R32 },
		{ ACT_ADD_REG, 0x40 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, REGOPD(0),
		{ CST_R64 },
		{ ACT_ADD_REG, 0x40 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O8 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM8 },
		{ ACT_PUT, 0xFE, ACT_MEM_I, 0 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM16 },
		{ ACT_PUT, 0xFF, ACT_MEM_I, 0 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM32 },
		{ ACT_PUT, 0xFF, ACT_MEM_I, 0 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM64 },
		{ ACT_PUT, 0xFF, ACT_MEM_I, 0 },
	},
};

static const instruction_pattern_t pattern_ins_0[] =
{
	{
		CPU_186, CPU_ALL,
		INS_O8 | INS_NEEDSIZE, NO_MODRM,
		{ },
		{ ACT_PUT, 0x6C },
	},
	{
		CPU_186, CPU_ALL,
		INS_O16 | INS_NEEDSIZE, NO_MODRM,
		{ },
		{ ACT_PUT, 0x6D },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NEEDSIZE, NO_MODRM,
		{ },
		{ ACT_PUT, 0x6D },
	},
};

static const instruction_pattern_t pattern_ins_2[] =
{
	{
		CPU_186, CPU_ALL,
		INS_O8 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI, CST_DX | CST_IMPLSIZE },
		{ ACT_PUT, 0x6C },
	},
	{
		CPU_186, CPU_ALL,
		INS_O16 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI, CST_DX | CST_IMPLSIZE },
		{ ACT_PUT, 0x6D },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI, CST_DX | CST_IMPLSIZE },
		{ ACT_PUT, 0x6D },
	},
};

static const instruction_pattern_t pattern_insb_0[] =
{
	{
		CPU_186, CPU_ALL,
		INS_O8, NO_MODRM,
		{ },
		{ ACT_PUT, 0x6C },
	},
};

static const instruction_pattern_t pattern_insb_2[] =
{
	{
		CPU_186, CPU_ALL,
		INS_O8 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI, CST_DX },
		{ ACT_PUT, 0x6C },
	},
};

static const instruction_pattern_t pattern_insw_0[] =
{
	{
		CPU_186, CPU_ALL,
		INS_O16, NO_MODRM,
		{ },
		{ ACT_PUT, 0x6D },
	},
};

static const instruction_pattern_t pattern_insw_2[] =
{
	{
		CPU_186, CPU_ALL,
		INS_O16 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI },
		{ ACT_PUT, 0x6D, CST_DX },
	},
};

static const instruction_pattern_t pattern_insd_0[] =
{
	{
		CPU_386, CPU_ALL,
		INS_O32, NO_MODRM,
		{ },
		{ ACT_PUT, 0x6D },
	},
};

static const instruction_pattern_t pattern_insd_2[] =
{
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI, CST_DX },
		{ ACT_PUT, 0x6D },
	},
};

static const instruction_pattern_t pattern_ins_nec_2[] =
{
	{
		CPU_V30, CPU_V55,
		0, MODRM2(0, 1),
		{ CST_R8, CST_R8 },
		{ ACT_PUT2, 0x0F, 0x31, ACT_MEM },
	},
	{
		CPU_V30, CPU_V55,
		0, MODRM1(0),
		{ CST_R8, CST_UI8 | CST_NOSIZE },
		{ ACT_PUT2, 0x0F, 0x39, ACT_MEM_I, 0 },
	},
};

static const instruction_pattern_t pattern_int_1[] =
{
	{
		CPU_8086, CPU_ALL,
		0, NO_MODRM,
		{ CST_UI8 | CST_NOSIZE },
		{ ACT_PUT, 0xCD, ACT_I8_0 },
	},
};

static const instruction_pattern_t pattern_int1_0[] =
{
	{
		CPU_386, CPU_ALL,
		0, NO_MODRM,
		{ },
		{ ACT_PUT, 0xF1 },
	},
};

static const instruction_pattern_t pattern_int3_0[] =
{
	{
		CPU_8086, CPU_ALL,
		0, NO_MODRM,
		{ },
		{ ACT_PUT, 0xCC },
	},
};

static const instruction_pattern_t pattern_into_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_NO64, NO_MODRM,
		{ },
		{ ACT_PUT, 0xCE },
	},
};

static const instruction_pattern_t pattern_invd_0[] =
{
	{
		CPU_486, CPU_ALL,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x08 },
	},
};

static const instruction_pattern_t pattern_invlpg_1[] =
{
	{
		CPU_486, CPU_ALL,
		0, MODRM1(0),
		{ CST_M8 },
		{ ACT_PUT2, 0x0F, 0x01, ACT_MEM_I, 7 },
	},
};

static const instruction_pattern_t pattern_iret_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16, NO_MODRM,
		{ },
		{ ACT_PUT, 0xCF },
	},
};

static const instruction_pattern_t pattern_iretd_0[] =
{
	{
		CPU_386, CPU_ALL,
		INS_O32, NO_MODRM,
		{ },
		{ ACT_PUT, 0xCF },
	},
};

static const instruction_pattern_t pattern_iretq_0[] =
{
	{
		CPU_X64, CPU_ALL,
		INS_O64, NO_MODRM,
		{ },
		{ ACT_PUT, 0xCF },
	},
};

static const instruction_pattern_t pattern_j_cc_1[] =
{
	{
		CPU_8086, CPU_ALL,
		0, NO_MODRM,
		{ CST_REL8 },
		{ ACT_ADD_COND, 0xE0, ACT_R8_0 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O16 | INS_OB | INS_OD64, NO_MODRM,
		{ CST_REL16 },
		{ ACT_PUT, 0x0F, ACT_ADD_COND, 0x80, ACT_R16_0 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_OB | INS_NO64, NO_MODRM,
		{ CST_REL32 },
		{ ACT_PUT, 0x0F, ACT_ADD_COND, 0x80, ACT_R32_0 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_OB | INS_OD64, NO_MODRM,
		{ CST_REL32 },
		{ ACT_PUT, 0x0F, ACT_ADD_COND, 0x80, ACT_R32_0 },
	},
};

static const instruction_pattern_t pattern_jcxz_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_A16, NO_MODRM,
		{ CST_REL16 },
		{ ACT_PUT, 0xE3, ACT_R8_0 },
	},
};

static const instruction_pattern_t pattern_jecxz_1[] =
{
	{
		CPU_386, CPU_ALL,
		INS_A32, NO_MODRM,
		{ CST_REL16 },
		{ ACT_PUT, 0xE3, ACT_R8_0 },
	},
};

static const instruction_pattern_t pattern_jrcxz_1[] =
{
	{
		CPU_X64, CPU_ALL,
		INS_A64, NO_MODRM,
		{ CST_REL16 },
		{ ACT_PUT, 0xE3, ACT_R8_0 },
	},
};

static const instruction_pattern_t pattern_jmp_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_OD64, NO_MODRM,
		{ CST_REL8 },
		{ ACT_PUT, 0xEB, ACT_R8_0 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_OB | INS_OD64, NO_MODRM,
		{ CST_REL16 },
		{ ACT_PUT, 0xE9, ACT_R16_0 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_OB | INS_NO64, NO_MODRM,
		{ CST_REL32 },
		{ ACT_PUT, 0xE9, ACT_R32_0 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_OB | INS_OD64, NO_MODRM,
		{ CST_REL32 },
		{ ACT_PUT, 0xE9, ACT_R32_0 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_OB | INS_OD64 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM16 },
		{ ACT_PUT, 0xFF, ACT_MEM_I, 4 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_OB | INS_NO64 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM32 },
		{ ACT_PUT, 0xFF, ACT_MEM_I, 4 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_OB | INS_OD64 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM64 },
		{ ACT_PUT, 0xFF, ACT_MEM_I, 4 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_OB | INS_NO64 | INS_NEEDSIZE, NO_MODRM,
		{ CST_FAR16 },
		{ ACT_PUT, 0xEA, ACT_F16_0 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_OB | INS_NO64 | INS_NEEDSIZE, NO_MODRM,
		{ CST_FAR32 },
		{ ACT_PUT, 0xEA, ACT_F32_0 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_OB | INS_OD64 | INS_NEEDSIZE, MODRM1(0),
		{ CST_FARM16 },
		{ ACT_PUT, 0xFF, ACT_MEM_I, 5 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_OB | INS_NO64 | INS_NEEDSIZE, MODRM1(0),
		{ CST_FARM32 },
		{ ACT_PUT, 0xFF, ACT_MEM_I, 5 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_OB | INS_OD64 | INS_NEEDSIZE, MODRM1(0),
		{ CST_FARM64 },
		{ ACT_PUT, 0xFF, ACT_MEM_I, 5 },
	},
};

static const instruction_pattern_t pattern_jmpe_1[] =
{
	{
		CPU_IA64, CPU_IA64,
		INS_O16 | INS_OB, NO_MODRM,
		{ CST_REL16 },
		{ ACT_PUT2, 0x0F, 0xB8, ACT_R16_0 },
	},
	{
		CPU_IA64, CPU_IA64,
		INS_O32 | INS_OB, NO_MODRM,
		{ CST_REL32 },
		{ ACT_PUT2, 0x0F, 0xB8, ACT_R32_0 },
	},
	{
		CPU_IA64, CPU_IA64,
		INS_O16 | INS_OB | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM16 },
		{ ACT_PUT2, 0x0F, 0x00, ACT_MEM_I, 6 },
	},
	{
		CPU_IA64, CPU_IA64,
		INS_O32 | INS_OB | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM32 },
		{ ACT_PUT2, 0x0F, 0x00, ACT_MEM_I, 6 },
	},
};

static const instruction_pattern_t pattern_lahf_0[] =
{
	{
		CPU_8086, CPU_ALL,
		0, NO_MODRM,
		{ },
		{ ACT_PUT, 0x9F },
	},
};

static const instruction_pattern_t pattern_lar_2[] =
{
	{
		CPU_286, CPU_ALL,
		INS_O16, MODRM2(1, 0),
		{ CST_R16, CST_RM16PLUS | CST_IMPLSIZE },
		{ ACT_PUT2, 0x0F, 0x02, ACT_MEM },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, MODRM2(1, 0),
		{ CST_R32, CST_RM16PLUS | CST_IMPLSIZE },
		{ ACT_PUT2, 0x0F, 0x02, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_lds_2[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_NO64, MODRM2(1, 0),
		{ CST_R16, CST_M16 },
		{ ACT_PUT, 0xC5, ACT_MEM },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NO64, MODRM2(1, 0),
		{ CST_R32, CST_M32 },
		{ ACT_PUT, 0xC5, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_les_2[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_NO64, MODRM2(1, 0),
		{ CST_R16, CST_M16 },
		{ ACT_PUT, 0xC4, ACT_MEM },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NO64, MODRM2(1, 0),
		{ CST_R32, CST_M32 },
		{ ACT_PUT, 0xC4, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_lfs_2[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16, MODRM2(1, 0),
		{ CST_R16, CST_M16 },
		{ ACT_PUT2, 0x0F, 0xB4, ACT_MEM },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, MODRM2(1, 0),
		{ CST_R32, CST_M32 },
		{ ACT_PUT2, 0x0F, 0xB4, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(1, 0),
		{ CST_R64, CST_M64 },
		{ ACT_PUT2, 0x0F, 0xB4, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_lgs_2[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16, MODRM2(1, 0),
		{ CST_R16, CST_M16 },
		{ ACT_PUT2, 0x0F, 0xB5, ACT_MEM },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, MODRM2(1, 0),
		{ CST_R32, CST_M32 },
		{ ACT_PUT2, 0x0F, 0xB5, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(1, 0),
		{ CST_R64, CST_M64 },
		{ ACT_PUT2, 0x0F, 0xB5, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_lss_2[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16, MODRM2(1, 0),
		{ CST_R16, CST_M16 },
		{ ACT_PUT2, 0x0F, 0xB2, ACT_MEM },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, MODRM2(1, 0),
		{ CST_R32, CST_M32 },
		{ ACT_PUT2, 0x0F, 0xB2, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(1, 0),
		{ CST_R64, CST_M64 },
		{ ACT_PUT2, 0x0F, 0xB2, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_lea_2[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16, MODRM2(1, 0),
		{ CST_R16, CST_M | CST_NOSIZE },
		{ ACT_PUT, 0x8D, ACT_MEM },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, MODRM2(1, 0),
		{ CST_R32, CST_M | CST_NOSIZE },
		{ ACT_PUT, 0x8D, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(1, 0),
		{ CST_R64, CST_M | CST_NOSIZE },
		{ ACT_PUT, 0x8D, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_leave_0[] =
{
	{
		CPU_186, CPU_ALL,
		0, NO_MODRM,
		{ },
		{ ACT_PUT, 0xC9 },
	},
};

static const instruction_pattern_t pattern_lgdt_1[] =
{
	{
		CPU_286, CPU_ALL,
		0, MODRM1(0),
		{ CST_M },
		{ ACT_PUT2, 0x0F, 0x01, ACT_MEM_I, 2 },
	},
};

static const instruction_pattern_t pattern_lidt_1[] =
{
	{
		CPU_286, CPU_ALL,
		0, MODRM1(0),
		{ CST_M },
		{ ACT_PUT2, 0x0F, 0x01, ACT_MEM_I, 3 },
	},
};

static const instruction_pattern_t pattern_lldt_1[] =
{
	{
		CPU_286, CPU_ALL,
		0, MODRM1(0),
		{ CST_RM16 | CST_IMPLSIZE },
		{ ACT_PUT2, 0x0F, 0x00, ACT_MEM_I, 2 },
	},
};

static const instruction_pattern_t pattern_lmsw_1[] =
{
	{
		CPU_286, CPU_ALL,
		0, MODRM1(0),
		{ CST_RM16 | CST_IMPLSIZE },
		{ ACT_PUT2, 0x0F, 0x01, ACT_MEM_I, 6 },
	},
};

static const instruction_pattern_t pattern_loadall_0[] =
{
	{
		CPU_286, CPU_286,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x05 },
	},
	{
		CPU_386, CPU_486B,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x07 },
	},
};

static const instruction_pattern_t pattern_loadall286_0[] =
{
	{
		CPU_286, CPU_286,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x05 },
	},
};

static const instruction_pattern_t pattern_loadall386_0[] =
{
	{
		CPU_386, CPU_486B,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x07 },
	},
};

static const instruction_pattern_t pattern_lods_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O8 | INS_NEEDSIZE, NO_MODRM,
		{ },
		{ ACT_PUT, 0xAC },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_NEEDSIZE, NO_MODRM,
		{ },
		{ ACT_PUT, 0xAD },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NEEDSIZE, NO_MODRM,
		{ },
		{ ACT_PUT, 0xAD },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_NEEDSIZE, NO_MODRM,
		{ },
		{ ACT_PUT, 0xAD },
	},
};

static const instruction_pattern_t pattern_lods_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O8 | INS_NEEDSIZE, NO_MODRM,
		{ CST_DSSI },
		{ ACT_PUT, 0xAC },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_NEEDSIZE, NO_MODRM,
		{ CST_DSSI },
		{ ACT_PUT, 0xAD },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NEEDSIZE, NO_MODRM,
		{ CST_DSSI },
		{ ACT_PUT, 0xAD },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_NEEDSIZE, NO_MODRM,
		{ CST_DSSI },
		{ ACT_PUT, 0xAD },
	},
};

static const instruction_pattern_t pattern_lodsb_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O8, NO_MODRM,
		{ },
		{ ACT_PUT, 0xAC },
	},
};

static const instruction_pattern_t pattern_lodsb_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O8 | INS_NEEDSIZE, NO_MODRM,
		{ CST_DSSI },
		{ ACT_PUT, 0xAC },
	},
};

static const instruction_pattern_t pattern_lodsw_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16, NO_MODRM,
		{ },
		{ ACT_PUT, 0xAD },
	},
};

static const instruction_pattern_t pattern_lodsw_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_NEEDSIZE, NO_MODRM,
		{ CST_DSSI },
		{ ACT_PUT, 0xAD },
	},
};

static const instruction_pattern_t pattern_lodsd_0[] =
{
	{
		CPU_386, CPU_ALL,
		INS_O32, NO_MODRM,
		{ },
		{ ACT_PUT, 0xAD },
	},
};

static const instruction_pattern_t pattern_lodsd_1[] =
{
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NEEDSIZE, NO_MODRM,
		{ CST_DSSI },
		{ ACT_PUT, 0xAD },
	},
};

static const instruction_pattern_t pattern_lodsq_0[] =
{
	{
		CPU_X64, CPU_ALL,
		INS_O64, NO_MODRM,
		{ },
		{ ACT_PUT, 0xAD },
	},
};

static const instruction_pattern_t pattern_lodsq_1[] =
{
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_NEEDSIZE, NO_MODRM,
		{ CST_DSSI },
		{ ACT_PUT, 0xAD },
	},
};

static const instruction_pattern_t pattern_loop_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_A16 | INS_AB, NO_MODRM,
		{ },
		{ ACT_PUT, 0xE2, ACT_R8_0 },
	},
	{
		CPU_386, CPU_ALL,
		INS_A32 | INS_AB, NO_MODRM,
		{ },
		{ ACT_PUT, 0xE2, ACT_R8_0 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_A64 | INS_AB, NO_MODRM,
		{ },
		{ ACT_PUT, 0xE2, ACT_R8_0 },
	},
};

static const instruction_pattern_t pattern_loope_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_A16 | INS_AB, NO_MODRM,
		{ },
		{ ACT_PUT, 0xE1, ACT_R8_0 },
	},
	{
		CPU_386, CPU_ALL,
		INS_A32 | INS_AB, NO_MODRM,
		{ },
		{ ACT_PUT, 0xE1, ACT_R8_0 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_A64 | INS_AB, NO_MODRM,
		{ },
		{ ACT_PUT, 0xE1, ACT_R8_0 },
	},
};

static const instruction_pattern_t pattern_loopne_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_A16 | INS_AB, NO_MODRM,
		{ },
		{ ACT_PUT, 0xE0, ACT_R8_0 },
	},
	{
		CPU_386, CPU_ALL,
		INS_A32 | INS_AB, NO_MODRM,
		{ },
		{ ACT_PUT, 0xE0, ACT_R8_0 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_A64 | INS_AB, NO_MODRM,
		{ },
		{ ACT_PUT, 0xE0, ACT_R8_0 },
	},
};

static const instruction_pattern_t pattern_lsl_2[] =
{
	{
		CPU_286, CPU_ALL,
		INS_O16, MODRM2(1, 0),
		{ CST_R16, CST_RM16PLUS | CST_IMPLSIZE },
		{ ACT_PUT2, 0x0F, 0x03, ACT_MEM },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, MODRM2(1, 0),
		{ CST_R32, CST_RM16PLUS | CST_IMPLSIZE },
		{ ACT_PUT2, 0x0F, 0x03, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(1, 0),
		{ CST_R64, CST_RM16PLUS | CST_IMPLSIZE },
		{ ACT_PUT2, 0x0F, 0x03, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_ltr_1[] =
{
	{
		CPU_286, CPU_ALL,
		0, MODRM1(0),
		{ CST_RM16 | CST_IMPLSIZE },
		{ ACT_PUT2, 0x0F, 0x00, ACT_MEM_I, 3 },
	},
};

static const instruction_pattern_t pattern_lzcnt_2[] =
{
	{
		CPU_X64, CPU_ALL,
		INS_O16 | INS_F3, MODRM2(1, 0),
		{ CST_R16, CST_RM16 },
		{ ACT_PUT2, 0x0F, 0xBD, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O32 | INS_F3, MODRM2(1, 0),
		{ CST_R32, CST_RM32 },
		{ ACT_PUT2, 0x0F, 0xBD, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_F3, MODRM2(1, 0),
		{ CST_R64, CST_RM64 },
		{ ACT_PUT2, 0x0F, 0xBD, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_mhdec_0[] =
{
	{
		CPU_V55, CPU_V55,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x7C },
	},
};

static const instruction_pattern_t pattern_mhenc_0[] =
{
	{
		CPU_V55, CPU_V55,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x93 },
	},
};

static const instruction_pattern_t pattern_mov_2[] =
{
	{
		CPU_8086, CPU_ALL,
		0, REGOPD(0),
		{ CST_R8, CST_I8 },
		{ ACT_ADD_REG, 0xB0, ACT_I8_1 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16, REGOPD(0),
		{ CST_R16, CST_I16 },
		{ ACT_ADD_REG, 0xB8, ACT_I16_1 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, REGOPD(0),
		{ CST_R32, CST_I32 },
		{ ACT_ADD_REG, 0xB8, ACT_I32_1 },
	},
	// r64, i64 after rm64, i32
	{
		CPU_8086, CPU_ALL,
		0, NO_MODRM,
		{ CST_AL, CST_MOFF8 },
		{ ACT_PUT, 0xA0, ACT_IA_1 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16, NO_MODRM,
		{ CST_AX, CST_MOFF16 },
		{ ACT_PUT, 0xA1, ACT_IA_1 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, NO_MODRM,
		{ CST_EAX, CST_MOFF32 },
		{ ACT_PUT, 0xA1, ACT_IA_1 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, NO_MODRM,
		{ CST_RAX, CST_MOFF64 },
		{ ACT_PUT, 0xA1, ACT_IA_1 },
	},
	{
		CPU_8086, CPU_ALL,
		0, NO_MODRM,
		{ CST_MOFF8, CST_AL },
		{ ACT_PUT, 0xA2, ACT_IA_1 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16, NO_MODRM,
		{ CST_MOFF16, CST_AX },
		{ ACT_PUT, 0xA3, ACT_IA_1 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, NO_MODRM,
		{ CST_MOFF32, CST_EAX },
		{ ACT_PUT, 0xA3, ACT_IA_1 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, NO_MODRM,
		{ CST_MOFF64, CST_RAX },
		{ ACT_PUT, 0xA3, ACT_IA_1 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O8 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM8, CST_I8 },
		{ ACT_ADD_REG, 0xC6, ACT_MEM_I, 0, ACT_I8_1 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM16, CST_I16 },
		{ ACT_PUT, 0xC7, ACT_MEM_I, 0, ACT_I16_1 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM32, CST_I32 },
		{ ACT_PUT, 0xC7, ACT_MEM_I, 0, ACT_I32_1 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_NEEDSIZE, MODRM1(0),
		{ CST_R64, CST_SI32 | CST_NOTRUNC }, // disallow truncation, better implementation available
		{ ACT_PUT, 0xC7, ACT_MEM_I, 0, ACT_I32_1 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M64, CST_SI32 },
		{ ACT_PUT, 0xC7, ACT_MEM_I, 0, ACT_I32_1 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, REGOPD(0),
		{ CST_R64, CST_I64 },
		{ ACT_ADD_REG, 0xB8, ACT_I64_1 },
	},
	{
		CPU_8086, CPU_ALL,
		0, MODRM2(0, 1),
		{ CST_RM8, CST_R8 },
		{ ACT_PUT, 0x88, ACT_MEM },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16, MODRM2(0, 1),
		{ CST_RM16, CST_R16 },
		{ ACT_PUT, 0x89, ACT_MEM },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, MODRM2(0, 1),
		{ CST_RM32, CST_R32 },
		{ ACT_PUT, 0x89, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(0, 1),
		{ CST_RM64, CST_R64 },
		{ ACT_PUT, 0x89, ACT_MEM },
	},
	{
		CPU_8086, CPU_ALL,
		0, MODRM2(1, 0),
		{ CST_R8, CST_RM8 },
		{ ACT_PUT, 0x8A, ACT_MEM },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16, MODRM2(1, 0),
		{ CST_R16, CST_RM16 },
		{ ACT_PUT, 0x8B, ACT_MEM },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, MODRM2(1, 0),
		{ CST_R32, CST_RM32 },
		{ ACT_PUT, 0x8B, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(1, 0),
		{ CST_R64, CST_RM64 },
		{ ACT_PUT, 0x8B, ACT_MEM },
	},
	// segment registers
	{
		CPU_8086, CPU_ALL,
		INS_O16, MODRM2(0, 1),
		{ CST_RM16, CST_SEG },
		{ ACT_PUT, 0x8C, ACT_MEM },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, MODRM2(0, 1),
		{ CST_RM32, CST_SEG },
		{ ACT_PUT, 0x8C, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(0, 1),
		{ CST_RM64, CST_SEG },
		{ ACT_PUT, 0x8C, ACT_MEM },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_OB, MODRM2(0, 1),
		{ CST_SEG, CST_RM16 },
		{ ACT_PUT, 0x8E, ACT_MEM },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_OB32, MODRM2(0, 1),
		{ CST_SEG, CST_RM32 },
		{ ACT_PUT, 0x8E, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(0, 1),
		{ CST_SEG, CST_RM64 },
		{ ACT_PUT, 0x8E, ACT_MEM },
	},
	// CRx
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NO64, MODRM2(0, 1),
		{ CST_R32, CST_CREG },
		{ ACT_PUT2, 0x0F, 0x20, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_LM64 | INS_OD64, MODRM2(0, 1),
		{ CST_R64, CST_CREG },
		{ ACT_PUT2, 0x0F, 0x20, ACT_MEM },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NO64, MODRM2(1, 0),
		{ CST_CREG, CST_R32 },
		{ ACT_PUT2, 0x0F, 0x22, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_LM64 | INS_OD64, MODRM2(1, 0),
		{ CST_CREG, CST_R64 },
		{ ACT_PUT2, 0x0F, 0x22, ACT_MEM },
	},
	// DRx
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NO64, MODRM2(0, 1),
		{ CST_R32, CST_DREG },
		{ ACT_PUT2, 0x0F, 0x21, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_LM64 | INS_OD64, MODRM2(0, 1),
		{ CST_R64, CST_DREG },
		{ ACT_PUT2, 0x0F, 0x21, ACT_MEM },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NO64, MODRM2(1, 0),
		{ CST_DREG, CST_R32 },
		{ ACT_PUT2, 0x0F, 0x23, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_LM64 | INS_OD64, MODRM2(1, 0),
		{ CST_DREG, CST_R64 },
		{ ACT_PUT2, 0x0F, 0x23, ACT_MEM },
	},
	// TRx
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NO64, MODRM2(0, 1),
		{ CST_R32, CST_TREG },
		{ ACT_PUT2, 0x0F, 0x24, ACT_MEM },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NO64, MODRM2(1, 0),
		{ CST_TREG, CST_R32 },
		{ ACT_PUT2, 0x0F, 0x26, ACT_MEM },
	},
	// NEC
	{
		CPU_8086, CPU_ALL,
		0, NO_MODRM,
		{ CST_AH, CST_PSW },
		{ ACT_PUT, 0x9F },
	},
	{
		CPU_8086, CPU_ALL,
		0, NO_MODRM,
		{ CST_PSW, CST_AH },
		{ ACT_PUT, 0x9E },
	},
};

static const instruction_pattern_t pattern_mov_3[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16, MODRM2(2, 1),
		{ CST_ES, CST_R16, CST_M16 },
		{ ACT_PUT, 0xC4, ACT_MEM },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16, MODRM2(2, 1),
		{ CST_DS, CST_R16, CST_M16 },
		{ ACT_PUT, 0xC5, ACT_MEM },
	},
	{
		CPU_V55, CPU_V55,
		INS_O16, MODRM2(2, 1),
		{ CST_DS3, CST_R16, CST_M16 },
		{ ACT_PUT2, 0x0F, 0x36, ACT_MEM },
	},
	{
		CPU_V55, CPU_V55,
		INS_O16, MODRM2(2, 1),
		{ CST_DS2, CST_R16, CST_M16 },
		{ ACT_PUT2, 0x0F, 0x3E, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_movs_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O8 | INS_NEEDSIZE, NO_MODRM,
		{ },
		{ ACT_PUT, 0xA4 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_NEEDSIZE, NO_MODRM,
		{ },
		{ ACT_PUT, 0xA5 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NEEDSIZE, NO_MODRM,
		{ },
		{ ACT_PUT, 0xA5 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_NEEDSIZE, NO_MODRM,
		{ },
		{ ACT_PUT, 0xA5 },
	},
};

static const instruction_pattern_t pattern_movs_2[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O8 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI, CST_DSSI },
		{ ACT_PUT, 0xA4 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI, CST_DSSI },
		{ ACT_PUT, 0xA5 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI, CST_DSSI },
		{ ACT_PUT, 0xA5 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI, CST_DSSI },
		{ ACT_PUT, 0xA5 },
	},
};

static const instruction_pattern_t pattern_movsb_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O8, NO_MODRM,
		{ },
		{ ACT_PUT, 0xA4 },
	},
};

static const instruction_pattern_t pattern_movsb_2[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O8 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI, CST_DSSI },
		{ ACT_PUT, 0xA4 },
	},
};

static const instruction_pattern_t pattern_movsw_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16, NO_MODRM,
		{ },
		{ ACT_PUT, 0xA5 },
	},
};

static const instruction_pattern_t pattern_movsw_2[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI, CST_DSSI },
		{ ACT_PUT, 0xA5 },
	},
};

static const instruction_pattern_t pattern_movsd_0[] =
{
	{
		CPU_386, CPU_ALL,
		INS_O32, NO_MODRM,
		{ },
		{ ACT_PUT, 0xA5 },
	},
};

static const instruction_pattern_t pattern_movsd_2[] =
{
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI, CST_DSSI },
		{ ACT_PUT, 0xA5 },
	},
};

static const instruction_pattern_t pattern_movsq_0[] =
{
	{
		CPU_X64, CPU_ALL,
		INS_O64, NO_MODRM,
		{ },
		{ ACT_PUT, 0xA5 },
	},
};

static const instruction_pattern_t pattern_movsq_2[] =
{
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI, CST_DSSI },
		{ ACT_PUT, 0xA5 },
	},
};

static const instruction_pattern_t pattern_movspa_0[] =
{
	{
		CPU_V25, CPU_V55,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x25 },
	},
};

static const instruction_pattern_t pattern_movspb_1[] =
{
	{
		CPU_V25, CPU_V55,
		0, MODRM1(0),
		{ CST_R16 },
		{ ACT_PUT2, 0x0F, 0x95, ACT_MEM_I, 7 },
	},
};

static const instruction_pattern_t pattern_movsx_2[] =
{
	{
		CPU_386, CPU_ALL,
		INS_O16, MODRM2(1, 0),
		{ CST_R16, CST_RM8 | CST_NEEDSIZE },
		{ ACT_PUT2, 0x0F, 0xBE, ACT_MEM },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, MODRM2(1, 0),
		{ CST_R32, CST_RM8 | CST_NEEDSIZE },
		{ ACT_PUT2, 0x0F, 0xBE, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(1, 0),
		{ CST_R64, CST_RM8 | CST_NEEDSIZE },
		{ ACT_PUT2, 0x0F, 0xBE, ACT_MEM },
	},
	/*{
		CPU_386, CPU_ALL,
		INS_O16, MODRM2(1, 0),
		{ CST_R16, CST_RM16 | CST_NEEDSIZE },
		{ ACT_PUT2, 0x0F, 0xBF, ACT_MEM },
	},*/
	{
		CPU_386, CPU_ALL,
		INS_O32, MODRM2(1, 0),
		{ CST_R32, CST_RM16 | CST_NEEDSIZE },
		{ ACT_PUT2, 0x0F, 0xBF, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(1, 0),
		{ CST_R64, CST_RM16 | CST_NEEDSIZE },
		{ ACT_PUT2, 0x0F, 0xBF, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O16 | INS_LM64, MODRM2(1, 0),
		{ CST_R16, CST_RM16 | CST_NEEDSIZE },
		{ ACT_PUT, 0x63, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O32 | INS_LM64, MODRM2(1, 0),
		{ CST_R32, CST_RM32 | CST_NEEDSIZE },
		{ ACT_PUT, 0x63, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_LM64, MODRM2(1, 0),
		{ CST_R64, CST_RM32 | CST_NEEDSIZE },
		{ ACT_PUT, 0x63, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_movsxd_2[] =
{
	{
		CPU_X64, CPU_ALL,
		INS_O16 | INS_LM64, MODRM2(1, 0),
		{ CST_R16, CST_RM16 | CST_NEEDSIZE },
		{ ACT_PUT, 0x63, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O32 | INS_LM64, MODRM2(1, 0),
		{ CST_R32, CST_RM32 | CST_NEEDSIZE },
		{ ACT_PUT, 0x63, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_LM64, MODRM2(1, 0),
		{ CST_R64, CST_RM32 | CST_NEEDSIZE },
		{ ACT_PUT, 0x63, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_movzx_2[] =
{
	{
		CPU_386, CPU_ALL,
		INS_O16, MODRM2(1, 0),
		{ CST_R16, CST_RM8 | CST_NEEDSIZE },
		{ ACT_PUT2, 0x0F, 0xB6, ACT_MEM },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, MODRM2(1, 0),
		{ CST_R32, CST_RM8 | CST_NEEDSIZE },
		{ ACT_PUT2, 0x0F, 0xB6, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(1, 0),
		{ CST_R64, CST_RM8 | CST_NEEDSIZE },
		{ ACT_PUT2, 0x0F, 0xB6, ACT_MEM },
	},
	/*{
		CPU_386, CPU_ALL,
		INS_O16, MODRM2(1, 0),
		{ CST_R16, CST_RM16 | CST_NEEDSIZE },
		{ ACT_PUT2, 0x0F, 0xB7, ACT_MEM },
	},*/
	{
		CPU_386, CPU_ALL,
		INS_O32, MODRM2(1, 0),
		{ CST_R32, CST_RM16 | CST_NEEDSIZE },
		{ ACT_PUT2, 0x0F, 0xB7, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(1, 0),
		{ CST_R64, CST_RM16 | CST_NEEDSIZE },
		{ ACT_PUT2, 0x0F, 0xB7, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_mrdec_0[] =
{
	{
		CPU_V55, CPU_V55,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x7D },
	},
};

static const instruction_pattern_t pattern_mrenc_0[] =
{
	{
		CPU_V55, CPU_V55,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x97 },
	},
};

static const instruction_pattern_t pattern_mul_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O8 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM8 },
		{ ACT_PUT, 0xF6, ACT_MEM_I, 4 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM16 },
		{ ACT_PUT, 0xF7, ACT_MEM_I, 4 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM32 },
		{ ACT_PUT, 0xF7, ACT_MEM_I, 4 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM64 },
		{ ACT_PUT, 0xF7, ACT_MEM_I, 4 },
	},
};

static const instruction_pattern_t pattern_neg_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O8 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM8 },
		{ ACT_PUT, 0xF6, ACT_MEM_I, 3 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM16 },
		{ ACT_PUT, 0xF7, ACT_MEM_I, 3 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM32 },
		{ ACT_PUT, 0xF7, ACT_MEM_I, 3 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM64 },
		{ ACT_PUT, 0xF7, ACT_MEM_I, 3 },
	},
};

static const instruction_pattern_t pattern_nop_0[] =
{
	{
		CPU_8086, CPU_ALL,
		0, NO_MODRM,
		{ },
		{ ACT_PUT, 0x90 },
	},
};

static const instruction_pattern_t pattern_nop_1[] =
{
	{
		CPU_686, CPU_ALL,
		INS_O16, MODRM1(0),
		{ CST_RM16 },
		{ ACT_PUT2, 0x0F, 0x1F, ACT_MEM_I, 0 },
	},
	{
		CPU_686, CPU_ALL,
		INS_O32, MODRM1(0),
		{ CST_RM32 },
		{ ACT_PUT2, 0x0F, 0x1F, ACT_MEM_I, 0 },
	},
};

static const instruction_pattern_t pattern_not_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O8 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM8 },
		{ ACT_PUT, 0xF6, ACT_MEM_I, 2 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM16 },
		{ ACT_PUT, 0xF7, ACT_MEM_I, 2 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM32 },
		{ ACT_PUT, 0xF7, ACT_MEM_I, 2 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM64 },
		{ ACT_PUT, 0xF7, ACT_MEM_I, 2 },
	},
};

static const instruction_pattern_t pattern_not1_1[] =
{
	{
		CPU_V30, CPU_V55,
		0, NO_MODRM,
		{ CST_CY },
		{ ACT_PUT, 0xF5 },
	},
};

static const instruction_pattern_t pattern_out_2[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O8, NO_MODRM,
		{ CST_UI8 | CST_NOTRUNC | CST_NOSIZE, CST_AL },
		{ ACT_PUT, 0xE6, ACT_I8_0 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16, NO_MODRM,
		{ CST_UI8 | CST_NOTRUNC | CST_NOSIZE, CST_AX },
		{ ACT_PUT, 0xE7, ACT_I8_0 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, NO_MODRM,
		{ CST_UI8 | CST_NOTRUNC | CST_NOSIZE, CST_EAX },
		{ ACT_PUT, 0xE7, ACT_I8_0 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O8, NO_MODRM,
		{ CST_DX | CST_IMPLSIZE, CST_AL },
		{ ACT_PUT, 0xEE },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16, NO_MODRM,
		{ CST_DX | CST_IMPLSIZE, CST_AX },
		{ ACT_PUT, 0xEF },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, NO_MODRM,
		{ CST_DX | CST_IMPLSIZE, CST_EAX },
		{ ACT_PUT, 0xEF },
	},
};

static const instruction_pattern_t pattern_outs_0[] =
{
	{
		CPU_186, CPU_ALL,
		INS_O8 | INS_NEEDSIZE, NO_MODRM,
		{ },
		{ ACT_PUT, 0x6E },
	},
	{
		CPU_186, CPU_ALL,
		INS_O16 | INS_NEEDSIZE, NO_MODRM,
		{ },
		{ ACT_PUT, 0x6F },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NEEDSIZE, NO_MODRM,
		{ },
		{ ACT_PUT, 0x6F },
	},
};

static const instruction_pattern_t pattern_outs_2[] =
{
	{
		CPU_186, CPU_ALL,
		INS_O8 | INS_NEEDSIZE, NO_MODRM,
		{ CST_DX | CST_IMPLSIZE, CST_DSSI },
		{ ACT_PUT, 0x6E },
	},
	{
		CPU_186, CPU_ALL,
		INS_O16 | INS_NEEDSIZE, NO_MODRM,
		{ CST_DX | CST_IMPLSIZE, CST_DSSI },
		{ ACT_PUT, 0x6F },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NEEDSIZE, NO_MODRM,
		{ CST_DX | CST_IMPLSIZE, CST_DSSI },
		{ ACT_PUT, 0x6F },
	},
};

static const instruction_pattern_t pattern_outsb_0[] =
{
	{
		CPU_186, CPU_ALL,
		INS_O8, NO_MODRM,
		{ },
		{ ACT_PUT, 0x6E },
	},
};

static const instruction_pattern_t pattern_outsb_2[] =
{
	{
		CPU_186, CPU_ALL,
		INS_O8 | INS_NEEDSIZE, NO_MODRM,
		{ CST_DX, CST_DSSI },
		{ ACT_PUT, 0x6E },
	},
};

static const instruction_pattern_t pattern_outsw_0[] =
{
	{
		CPU_186, CPU_ALL,
		INS_O16, NO_MODRM,
		{ },
		{ ACT_PUT, 0x6F },
	},
};

static const instruction_pattern_t pattern_outsw_2[] =
{
	{
		CPU_186, CPU_ALL,
		INS_O16 | INS_NEEDSIZE, NO_MODRM,
		{ CST_DX, CST_DSSI },
		{ ACT_PUT, 0x6F, CST_DX },
	},
};

static const instruction_pattern_t pattern_outsd_0[] =
{
	{
		CPU_386, CPU_ALL,
		INS_O32, NO_MODRM,
		{ },
		{ ACT_PUT, 0x6F },
	},
};

static const instruction_pattern_t pattern_outsd_2[] =
{
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NEEDSIZE, NO_MODRM,
		{ CST_DX, CST_DSSI },
		{ ACT_PUT, 0x6F },
	},
};

static const instruction_pattern_t pattern_pop_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16, REGOPD(0),
		{ CST_R16 },
		{ ACT_ADD_REG, 0x58 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NO64, REGOPD(0),
		{ CST_R32 },
		{ ACT_ADD_REG, 0x58 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_LM64 | INS_OD64, REGOPD(0),
		{ CST_R64 },
		{ ACT_ADD_REG, 0x58 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16, MODRM1(0),
		{ CST_RM16 },
		{ ACT_PUT, 0x8F, ACT_MEM_I, 0 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NO64, MODRM1(0),
		{ CST_RM32 },
		{ ACT_PUT, 0x8F, ACT_MEM_I, 0 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_LM64 | INS_OD64, MODRM1(0),
		{ CST_RM64 },
		{ ACT_PUT, 0x8F, ACT_MEM_I, 0 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_NO64, NO_MODRM,
		{ CST_ES },
		{ ACT_PUT, 0x07 },
	},
	{
		CPU_8086, CPU_8086,
		0, NO_MODRM,
		{ CST_CS },
		{ ACT_PUT, 0x0F },
	},
	{
		CPU_8086, CPU_ALL,
		INS_NO64, NO_MODRM,
		{ CST_SS },
		{ ACT_PUT, 0x17 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_NO64, NO_MODRM,
		{ CST_DS },
		{ ACT_PUT, 0x1F },
	},
	{
		CPU_8086, CPU_ALL,
		INS_OD64, NO_MODRM,
		{ CST_FS },
		{ ACT_PUT2, 0x0F, 0xA1 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_OD64, NO_MODRM,
		{ CST_GS },
		{ ACT_PUT2, 0x0F, 0xA9 },
	},
	// NEC
	{
		CPU_V55, CPU_V55,
		0, NO_MODRM,
		{ CST_DS3 },
		{ ACT_PUT2, 0x0F, 0x77 },
	},
	{
		CPU_V55, CPU_V55,
		0, NO_MODRM,
		{ CST_DS2 },
		{ ACT_PUT2, 0x0F, 0x7F },
	},
	{
		CPU_8086, CPU_ALL,
		0, NO_MODRM,
		{ CST_PSW },
		{ ACT_PUT, 0x9D },
	},
	{
		CPU_8086, CPU_ALL,
		0, NO_MODRM,
		{ CST_RSYMBOL },
		{ ACT_PUT, 0x61 },
	},
};

static const instruction_pattern_t pattern_popa_0[] =
{
	{
		CPU_186, CPU_ALL,
		INS_O16 | INS_NO64, NO_MODRM,
		{ },
		{ ACT_PUT, 0x61 },
	},
};

static const instruction_pattern_t pattern_popad_0[] =
{
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NO64, NO_MODRM,
		{ },
		{ ACT_PUT, 0x61 },
	},
};

static const instruction_pattern_t pattern_popcnt_2[] =
{
	{
		CPU_X64, CPU_ALL,
		INS_O16 | INS_F3, MODRM2(1, 0),
		{ CST_R16, CST_RM16 },
		{ ACT_PUT2, 0x0F, 0xB8, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O32 | INS_F3, MODRM2(1, 0),
		{ CST_R32, CST_RM32 },
		{ ACT_PUT2, 0x0F, 0xB8, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_F3, MODRM2(1, 0),
		{ CST_R64, CST_RM64 },
		{ ACT_PUT2, 0x0F, 0xB8, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_popf_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16, NO_MODRM,
		{ },
		{ ACT_PUT, 0x9D },
	},
};

static const instruction_pattern_t pattern_popfd_0[] =
{
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NO64, NO_MODRM,
		{ },
		{ ACT_PUT, 0x9D },
	},
};

static const instruction_pattern_t pattern_popfq_0[] =
{
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_LM64, NO_MODRM,
		{ },
		{ ACT_PUT, 0x9D },
	},
};

static const instruction_pattern_t pattern_push_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16, REGOPD(0),
		{ CST_R16 },
		{ ACT_ADD_REG, 0x50 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NO64, REGOPD(0),
		{ CST_R32 },
		{ ACT_ADD_REG, 0x50 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_LM64 | INS_OD64, REGOPD(0),
		{ CST_R64 },
		{ ACT_ADD_REG, 0x50 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16, MODRM1(0),
		{ CST_RM16 },
		{ ACT_PUT, 0xFF, ACT_MEM_I, 6 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NO64, MODRM1(0),
		{ CST_RM32 },
		{ ACT_PUT, 0xFF, ACT_MEM_I, 6 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_LM64 | INS_OD64, MODRM1(0),
		{ CST_RM64 },
		{ ACT_PUT, 0xFF, ACT_MEM_I, 6 },
	},
	{
		CPU_186, CPU_ALL,
		INS_OB | INS_OD64, NO_MODRM,
		{ CST_SI8 | CST_NOTRUNC },
		{ ACT_PUT, 0x6A, ACT_I8_0 },
	},
	{
		CPU_186, CPU_ALL,
		INS_O16 | INS_OB | INS_OD64, NO_MODRM,
		{ CST_I16 },
		{ ACT_PUT, 0x68, ACT_I16_0 },
	},
	{
		CPU_186, CPU_ALL,
		INS_O32 | INS_NO64, NO_MODRM,
		{ CST_I32 },
		{ ACT_PUT, 0x68, ACT_I32_0 },
	},
	{
		CPU_186, CPU_ALL,
		INS_O64 | INS_LM64 | INS_OB | INS_OD64, NO_MODRM,
		{ CST_I32 },
		{ ACT_PUT, 0x68, ACT_I32_0 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_NO64, NO_MODRM,
		{ CST_ES },
		{ ACT_PUT, 0x06 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_NO64, NO_MODRM,
		{ CST_CS },
		{ ACT_PUT, 0x0E },
	},
	{
		CPU_8086, CPU_ALL,
		INS_NO64, NO_MODRM,
		{ CST_SS },
		{ ACT_PUT, 0x16 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_NO64, NO_MODRM,
		{ CST_DS },
		{ ACT_PUT, 0x1E },
	},
	{
		CPU_8086, CPU_ALL,
		INS_OD64, NO_MODRM,
		{ CST_FS },
		{ ACT_PUT2, 0x0F, 0xA0 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_OD64, NO_MODRM,
		{ CST_GS },
		{ ACT_PUT2, 0x0F, 0xA8 },
	},
	// NEC
	{
		CPU_V55, CPU_V55,
		0, NO_MODRM,
		{ CST_DS3 },
		{ ACT_PUT2, 0x0F, 0x76 },
	},
	{
		CPU_V55, CPU_V55,
		0, NO_MODRM,
		{ CST_DS2 },
		{ ACT_PUT2, 0x0F, 0x7E },
	},
	{
		CPU_8086, CPU_ALL,
		0, NO_MODRM,
		{ CST_PSW },
		{ ACT_PUT, 0x9C },
	},
	{
		CPU_8086, CPU_ALL,
		0, NO_MODRM,
		{ CST_RSYMBOL },
		{ ACT_PUT, 0x60 },
	},
};

static const instruction_pattern_t pattern_pusha_0[] =
{
	{
		CPU_186, CPU_ALL,
		INS_O16 | INS_NO64, NO_MODRM,
		{ },
		{ ACT_PUT, 0x60 },
	},
};

static const instruction_pattern_t pattern_pushad_0[] =
{
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NO64, NO_MODRM,
		{ },
		{ ACT_PUT, 0x60 },
	},
};

static const instruction_pattern_t pattern_pushf_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16, NO_MODRM,
		{ },
		{ ACT_PUT, 0x9C },
	},
};

static const instruction_pattern_t pattern_pushfd_0[] =
{
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NO64, NO_MODRM,
		{ },
		{ ACT_PUT, 0x9C },
	},
};

static const instruction_pattern_t pattern_pushfq_0[] =
{
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_LM64, NO_MODRM,
		{ },
		{ ACT_PUT, 0x9C },
	},
};

static const instruction_pattern_t pattern_qhout_1[] =
{
	{
		CPU_V55, CPU_V55,
		0, NO_MODRM,
		{ CST_UI16 | CST_NOSIZE },
		{ ACT_PUT2, 0x0F, 0xE0, ACT_I16_0 },
	},
};

static const instruction_pattern_t pattern_qout_1[] =
{
	{
		CPU_V55, CPU_V55,
		0, NO_MODRM,
		{ CST_UI16 | CST_NOSIZE },
		{ ACT_PUT2, 0x0F, 0xE1, ACT_I16_0 },
	},
};

static const instruction_pattern_t pattern_qtin_1[] =
{
	{
		CPU_V55, CPU_V55,
		0, NO_MODRM,
		{ CST_UI16 | CST_NOSIZE },
		{ ACT_PUT2, 0x0F, 0xE2, ACT_I16_0 },
	},
};

#define _GENERATE_RCL_LIKE(__name, __value) \
static const instruction_pattern_t pattern_##__name##_2[] = \
{ \
	{ \
		CPU_8086, CPU_ALL, \
		INS_O8, MODRM1(0), \
		{ CST_RM8, CST_EQ1 | CST_NOSIZE }, \
		{ ACT_PUT, 0xD0, ACT_MEM_I, (__value) }, \
	}, \
	{ \
		CPU_8086, CPU_ALL, \
		INS_O16, MODRM1(0), \
		{ CST_RM16, CST_EQ1 | CST_NOSIZE }, \
		{ ACT_PUT, 0xD1, ACT_MEM_I, (__value) }, \
	}, \
	{ \
		CPU_386, CPU_ALL, \
		INS_O32, MODRM1(0), \
		{ CST_RM32, CST_EQ1 | CST_NOSIZE }, \
		{ ACT_PUT, 0xD1, ACT_MEM_I, (__value) }, \
	}, \
	{ \
		CPU_X64, CPU_ALL, \
		INS_O64, MODRM1(0), \
		{ CST_RM64, CST_EQ1 | CST_NOSIZE }, \
		{ ACT_PUT, 0xD1, ACT_MEM_I, (__value) }, \
	}, \
	{ \
		CPU_186, CPU_ALL, \
		INS_O8, MODRM1(0), \
		{ CST_RM8, CST_UI8 | CST_NOSIZE }, \
		{ ACT_PUT, 0xC0, ACT_MEM_I, (__value), ACT_I8_1 }, \
	}, \
	{ \
		CPU_186, CPU_ALL, \
		INS_O16, MODRM1(0), \
		{ CST_RM16, CST_UI8 | CST_NOSIZE }, \
		{ ACT_PUT, 0xC1, ACT_MEM_I, (__value), ACT_I8_1 }, \
	}, \
	{ \
		CPU_386, CPU_ALL, \
		INS_O32, MODRM1(0), \
		{ CST_RM32, CST_UI8 | CST_NOSIZE }, \
		{ ACT_PUT, 0xC1, ACT_MEM_I, (__value), ACT_I8_1 }, \
	}, \
	{ \
		CPU_X64, CPU_ALL, \
		INS_O64, MODRM1(0), \
		{ CST_RM64, CST_UI8 | CST_NOSIZE }, \
		{ ACT_PUT, 0xC1, ACT_MEM_I, (__value), ACT_I8_1 }, \
	}, \
	{ \
		CPU_8086, CPU_ALL, \
		INS_O8, MODRM1(0), \
		{ CST_RM8, CST_CL | CST_IMPLSIZE }, \
		{ ACT_PUT, 0xD2, ACT_MEM_I, (__value) }, \
	}, \
	{ \
		CPU_8086, CPU_ALL, \
		INS_O16, MODRM1(0), \
		{ CST_RM16, CST_CL | CST_IMPLSIZE }, \
		{ ACT_PUT, 0xD3, ACT_MEM_I, (__value) }, \
	}, \
	{ \
		CPU_386, CPU_ALL, \
		INS_O32, MODRM1(0), \
		{ CST_RM32, CST_CL | CST_IMPLSIZE }, \
		{ ACT_PUT, 0xD3, ACT_MEM_I, (__value) }, \
	}, \
	{ \
		CPU_X64, CPU_ALL, \
		INS_O64, MODRM1(0), \
		{ CST_RM64, CST_CL | CST_IMPLSIZE }, \
		{ ACT_PUT, 0xD3, ACT_MEM_I, (__value) }, \
	}, \
}

_GENERATE_RCL_LIKE(rcl, 2);
_GENERATE_RCL_LIKE(rcr, 3);
_GENERATE_RCL_LIKE(rol, 0);
_GENERATE_RCL_LIKE(ror, 1);
_GENERATE_RCL_LIKE(sal, 4);
_GENERATE_RCL_LIKE(sar, 7);
_GENERATE_RCL_LIKE(shl, 4);
_GENERATE_RCL_LIKE(shr, 5);

static const instruction_pattern_t pattern_rdm_0[] =
{
	{
		CPU_GX2, CPU_GX2,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x3A },
	},
};

static const instruction_pattern_t pattern_rdfsbase_1[] =
{
	{
		CPU_X64, CPU_ALL,
		INS_O32 | INS_F3, MODRM1(0),
		{ CST_R32, CST_RM32 },
		{ ACT_PUT2, 0x0F, 0xAE, ACT_MEM_I, 0 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_F3, MODRM1(0),
		{ CST_R64, CST_RM64 },
		{ ACT_PUT2, 0x0F, 0xAE, ACT_MEM_I, 0 },
	},
};

static const instruction_pattern_t pattern_rdgsbase_1[] =
{
	{
		CPU_X64, CPU_ALL,
		INS_O32 | INS_F3, MODRM1(0),
		{ CST_R32, CST_RM32 },
		{ ACT_PUT2, 0x0F, 0xAE, ACT_MEM_I, 1 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_F3, MODRM1(0),
		{ CST_R64, CST_RM64 },
		{ ACT_PUT2, 0x0F, 0xAE, ACT_MEM_I, 1 },
	},
};

static const instruction_pattern_t pattern_rdmsr_0[] =
{
	{
		CPU_586, CPU_ALL,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x32 },
	},
};

static const instruction_pattern_t pattern_rdpid_1[] =
{
	{
		CPU_X64, CPU_ALL,
		INS_O32 | INS_F3, MODRM1(0),
		{ CST_R32 },
		{ ACT_PUT2, 0x0F, 0xC7, ACT_MEM_I, 7 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_F3, MODRM1(0),
		{ CST_R64 },
		{ ACT_PUT2, 0x0F, 0xC7, ACT_MEM_I, 7 },
	},
};

static const instruction_pattern_t pattern_rdpmc_0[] =
{
	{
		CPU_586, CPU_ALL,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x33 },
	},
};

static const instruction_pattern_t pattern_rdshr_1[] =
{
	{
		CPU_MX, CPU_MX,
		0, MODRM1(0),
		{ CST_RM32 },
		{ ACT_PUT2, 0x0F, 0x36, ACT_MEM_I, 0 },
	},
};

static const instruction_pattern_t pattern_rdtsc_0[] =
{
	{
		CPU_586, CPU_ALL,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x31 },
	},
};

static const instruction_pattern_t pattern_rdtscp_0[] =
{
	{
		CPU_X64, CPU_ALL,
		0, NO_MODRM,
		{ },
		{ ACT_PUT3, 0x0F, 0x01, 0xF9 },
	},
};

static const instruction_pattern_t pattern_ret_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_OB | INS_OD64, NO_MODRM,
		{ },
		{ ACT_PUT, 0xC3 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_OB | INS_OD64, NO_MODRM,
		{ },
		{ ACT_PUT, 0xC3 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_OB | INS_OD64, NO_MODRM,
		{ },
		{ ACT_PUT, 0xC3 },
	},
};

static const instruction_pattern_t pattern_ret_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_OB | INS_OD64, NO_MODRM,
		{ CST_UI16 | CST_NOSIZE },
		{ ACT_PUT, 0xC2, ACT_I16_0 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_OB | INS_OD64, NO_MODRM,
		{ CST_UI16 | CST_NOSIZE },
		{ ACT_PUT, 0xC2, ACT_I16_0 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_OB | INS_OD64, NO_MODRM,
		{ CST_UI16 | CST_NOSIZE },
		{ ACT_PUT, 0xC2, ACT_I16_0 },
	},
};

static const instruction_pattern_t pattern_retw_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_OD64, NO_MODRM,
		{ },
		{ ACT_PUT, 0xC3 },
	},
};

static const instruction_pattern_t pattern_retw_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_OB | INS_OD64, NO_MODRM,
		{ CST_UI16 | CST_NOSIZE },
		{ ACT_PUT, 0xC2, ACT_I16_0 },
	},
};

static const instruction_pattern_t pattern_retd_0[] =
{
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_OD64, NO_MODRM,
		{ },
		{ ACT_PUT, 0xC3 },
	},
};

static const instruction_pattern_t pattern_retd_1[] =
{
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_OB | INS_OD64, NO_MODRM,
		{ CST_UI16 | CST_NOSIZE },
		{ ACT_PUT, 0xC2, ACT_I16_0 },
	},
};

static const instruction_pattern_t pattern_retq_0[] =
{
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_OD64, NO_MODRM,
		{ },
		{ ACT_PUT, 0xC3 },
	},
};

static const instruction_pattern_t pattern_retq_1[] =
{
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_OB | INS_OD64, NO_MODRM,
		{ CST_UI16 | CST_NOSIZE },
		{ ACT_PUT, 0xC2, ACT_I16_0 },
	},
};

static const instruction_pattern_t pattern_retf_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_OB | INS_OD64, NO_MODRM,
		{ },
		{ ACT_PUT, 0xCB },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_OB | INS_OD64, NO_MODRM,
		{ },
		{ ACT_PUT, 0xCB },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_OB | INS_OD64, NO_MODRM,
		{ },
		{ ACT_PUT, 0xCB },
	},
};

static const instruction_pattern_t pattern_retf_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_OB | INS_OD64, NO_MODRM,
		{ CST_UI16 | CST_NOSIZE },
		{ ACT_PUT, 0xCA, ACT_I16_0 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_OB | INS_OD64, NO_MODRM,
		{ CST_UI16 | CST_NOSIZE },
		{ ACT_PUT, 0xCA, ACT_I16_0 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_OB | INS_OD64, NO_MODRM,
		{ CST_UI16 | CST_NOSIZE },
		{ ACT_PUT, 0xCA, ACT_I16_0 },
	},
};

static const instruction_pattern_t pattern_retfw_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_OD64, NO_MODRM,
		{ },
		{ ACT_PUT, 0xCB },
	},
};

static const instruction_pattern_t pattern_retfw_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_OB | INS_OD64, NO_MODRM,
		{ CST_UI16 | CST_NOSIZE },
		{ ACT_PUT, 0xCA, ACT_I16_0 },
	},
};

static const instruction_pattern_t pattern_retfd_0[] =
{
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_OD64, NO_MODRM,
		{ },
		{ ACT_PUT, 0xCB },
	},
};

static const instruction_pattern_t pattern_retfd_1[] =
{
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_OB | INS_OD64, NO_MODRM,
		{ CST_UI16 | CST_NOSIZE },
		{ ACT_PUT, 0xCA, ACT_I16_0 },
	},
};

static const instruction_pattern_t pattern_retfq_0[] =
{
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_OD64, NO_MODRM,
		{ },
		{ ACT_PUT, 0xCB },
	},
};

static const instruction_pattern_t pattern_retfq_1[] =
{
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_OB | INS_OD64, NO_MODRM,
		{ CST_UI16 | CST_NOSIZE },
		{ ACT_PUT, 0xCA, ACT_I16_0 },
	},
};

static const instruction_pattern_t pattern_retrbi_0[] =
{
	{
		CPU_V25, CPU_V55,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x91 },
	},
};

static const instruction_pattern_t pattern_retxa_1[] =
{
	{
		CPU_V33, CPU_V33,
		0, NO_MODRM,
		{ CST_UI8 | CST_NOSIZE },
		{ ACT_PUT2, 0x0F, 0xE0, ACT_I8_0 },
	},
};

static const instruction_pattern_t pattern_rol4_1[] =
{
	{
		CPU_V30, CPU_V55,
		0, MODRM1(0),
		{ CST_RM8 },
		{ ACT_PUT2, 0x0F, 0x28, ACT_MEM_I, 0 },
	},
};

static const instruction_pattern_t pattern_ror4_1[] =
{
	{
		CPU_V30, CPU_V55,
		0, MODRM1(0),
		{ CST_RM8 },
		{ ACT_PUT2, 0x0F, 0x2A, ACT_MEM_I, 0 },
	},
};

static const instruction_pattern_t pattern_rsdc_2[] =
{
	{
		CPU_CYRIX, CPU_GX2,
		0, MODRM2(1, 0),
		{ CST_SEG, CST_M | CST_NOSIZE },
		{ ACT_PUT2, 0x0F, 0x79, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_rsm_0[] =
{
	{
		CPU_386, CPU_ALL, // technically, 386SL/486SL/Pentium
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0xAA },
	},
};

static const instruction_pattern_t pattern_rsldt_1[] =
{
	{
		CPU_CYRIX, CPU_GX2,
		0, MODRM1(0),
		{ CST_M | CST_NOSIZE },
		{ ACT_PUT2, 0x0F, 0x7B, ACT_MEM_I, 0 },
	},
};

static const instruction_pattern_t pattern_rsts_1[] =
{
	{
		CPU_CYRIX, CPU_GX2,
		0, MODRM1(0),
		{ CST_M | CST_NOSIZE },
		{ ACT_PUT2, 0x0F, 0x7D, ACT_MEM_I, 0 },
	},
};

static const instruction_pattern_t pattern_rstwdt_2[] =
{
	{
		CPU_186, CPU_ALL,
		0, NO_MODRM,
		{ CST_UI8 | CST_NOSIZE, CST_UI8 | CST_NOSIZE },
		{ ACT_PUT2, 0x0F, 0x96, ACT_I8_0, ACT_I8_1 },
	},
};

static const instruction_pattern_t pattern_sahf_0[] =
{
	{
		CPU_8086, CPU_ALL,
		0, NO_MODRM,
		{ },
		{ ACT_PUT, 0x9E },
	},
};

static const instruction_pattern_t pattern_salc_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_NO64, NO_MODRM,
		{ },
		{ ACT_PUT, 0xD6 },
	},
};

static const instruction_pattern_t pattern_scas_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O8 | INS_NEEDSIZE, NO_MODRM,
		{ },
		{ ACT_PUT, 0xAE },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_NEEDSIZE, NO_MODRM,
		{ },
		{ ACT_PUT, 0xAF },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NEEDSIZE, NO_MODRM,
		{ },
		{ ACT_PUT, 0xAF },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_NEEDSIZE, NO_MODRM,
		{ },
		{ ACT_PUT, 0xAF },
	},
};

static const instruction_pattern_t pattern_scas_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O8 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI },
		{ ACT_PUT, 0xAE },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI },
		{ ACT_PUT, 0xAF },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI },
		{ ACT_PUT, 0xAF },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI },
		{ ACT_PUT, 0xAF },
	},
};

static const instruction_pattern_t pattern_scasb_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O8, NO_MODRM,
		{ },
		{ ACT_PUT, 0xAE },
	},
};

static const instruction_pattern_t pattern_scasb_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O8 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI },
		{ ACT_PUT, 0xAE },
	},
};

static const instruction_pattern_t pattern_scasw_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16, NO_MODRM,
		{ },
		{ ACT_PUT, 0xAF },
	},
};

static const instruction_pattern_t pattern_scasw_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI },
		{ ACT_PUT, 0xAF },
	},
};

static const instruction_pattern_t pattern_scasd_0[] =
{
	{
		CPU_386, CPU_ALL,
		INS_O32, NO_MODRM,
		{ },
		{ ACT_PUT, 0xAF },
	},
};

static const instruction_pattern_t pattern_scasd_1[] =
{
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI },
		{ ACT_PUT, 0xAF },
	},
};

static const instruction_pattern_t pattern_scasq_0[] =
{
	{
		CPU_X64, CPU_ALL,
		INS_O64, NO_MODRM,
		{ },
		{ ACT_PUT, 0xAF },
	},
};

static const instruction_pattern_t pattern_scasq_1[] =
{
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI },
		{ ACT_PUT, 0xAF },
	},
};

static const instruction_pattern_t pattern_scheol_0[] =
{
	{
		CPU_V55, CPU_V55,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x78 },
	},
};

static const instruction_pattern_t pattern_set1_1[] =
{
	{
		CPU_V30, CPU_V55,
		0, NO_MODRM,
		{ CST_CY },
		{ ACT_PUT, 0xF9 },
	},
	{
		CPU_V30, CPU_V55,
		0, NO_MODRM,
		{ CST_DIR },
		{ ACT_PUT, 0xFD },
	},
};

static const instruction_pattern_t pattern_set_cc_1[] =
{
	{
		CPU_386, CPU_ALL,
		0, MODRM1(0),
		{ CST_RM8 },
		{ ACT_PUT, 0x0F, ACT_ADD_COND, 0x90, ACT_MEM_I, 0 },
	},
};

static const instruction_pattern_t pattern_setmo_1[] =
{
	{
		CPU_8086, CPU_8086,
		INS_O8, MODRM1(0),
		{ CST_RM8 },
		{ ACT_PUT, 0xD0, ACT_MEM_I, 6 },
	},
	{
		CPU_8086, CPU_8086,
		INS_O16, MODRM1(0),
		{ CST_RM16 },
		{ ACT_PUT, 0xD1, ACT_MEM_I, 6 },
	},
};

static const instruction_pattern_t pattern_setmo_2[] =
{
	{
		CPU_8086, CPU_8086,
		INS_O8, MODRM1(0),
		{ CST_RM8, CST_EQ1 | CST_NOSIZE },
		{ ACT_PUT, 0xD0, ACT_MEM_I, 6 },
	},
	{
		CPU_8086, CPU_8086,
		INS_O16, MODRM1(0),
		{ CST_RM16, CST_EQ1 | CST_NOSIZE },
		{ ACT_PUT, 0xD1, ACT_MEM_I, 6 },
	},
	{
		CPU_8086, CPU_8086,
		INS_O8, MODRM1(0),
		{ CST_RM8, CST_CL | CST_IMPLSIZE },
		{ ACT_PUT, 0xD2, ACT_MEM_I, 6 },
	},
	{
		CPU_8086, CPU_8086,
		INS_O16, MODRM1(0),
		{ CST_RM16, CST_CL | CST_IMPLSIZE },
		{ ACT_PUT, 0xD3, ACT_MEM_I, 6 },
	},
};

static const instruction_pattern_t pattern_setmoc_1[] =
{
	{
		CPU_8086, CPU_8086,
		INS_O8, MODRM1(0),
		{ CST_RM8 },
		{ ACT_PUT, 0xD2, ACT_MEM_I, 6 },
	},
	{
		CPU_8086, CPU_8086,
		INS_O16, MODRM1(0),
		{ CST_RM16 },
		{ ACT_PUT, 0xD3, ACT_MEM_I, 6 },
	},
};

static const instruction_pattern_t pattern_sgdt_1[] =
{
	{
		CPU_286, CPU_ALL,
		0, MODRM1(0),
		{ CST_M },
		{ ACT_PUT2, 0x0F, 0x01, ACT_MEM_I, 0 },
	},
};

static const instruction_pattern_t pattern_shld_3[] =
{
	{
		CPU_386, CPU_ALL,
		INS_O16, MODRM2(0, 1),
		{ CST_RM16, CST_R16, CST_UI8 | CST_NOSIZE },
		{ ACT_PUT2, 0x0F, 0xA4, ACT_MEM, ACT_I8_2 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, MODRM2(0, 1),
		{ CST_RM32, CST_R32, CST_UI8 | CST_NOSIZE },
		{ ACT_PUT2, 0x0F, 0xA4, ACT_MEM, ACT_I8_2 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(0, 1),
		{ CST_RM64, CST_R64, CST_UI8 | CST_NOSIZE },
		{ ACT_PUT2, 0x0F, 0xA4, ACT_MEM, ACT_I8_2 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O16, MODRM2(0, 1),
		{ CST_RM16, CST_R16, CST_CL | CST_IMPLSIZE },
		{ ACT_PUT2, 0x0F, 0xA5, ACT_MEM },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, MODRM2(0, 1),
		{ CST_RM32, CST_R32, CST_CL | CST_IMPLSIZE },
		{ ACT_PUT2, 0x0F, 0xA5, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(0, 1),
		{ CST_RM64, CST_R64, CST_CL | CST_IMPLSIZE },
		{ ACT_PUT2, 0x0F, 0xA5, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_shrd_3[] =
{
	{
		CPU_386, CPU_ALL,
		INS_O16, MODRM2(0, 1),
		{ CST_RM16, CST_R16, CST_UI8 | CST_NOSIZE },
		{ ACT_PUT2, 0x0F, 0xAC, ACT_MEM, ACT_I8_2 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, MODRM2(0, 1),
		{ CST_RM32, CST_R32, CST_UI8 | CST_NOSIZE },
		{ ACT_PUT2, 0x0F, 0xAC, ACT_MEM, ACT_I8_2 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(0, 1),
		{ CST_RM64, CST_R64, CST_UI8 | CST_NOSIZE },
		{ ACT_PUT2, 0x0F, 0xAC, ACT_MEM, ACT_I8_2 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O16, MODRM2(0, 1),
		{ CST_RM16, CST_R16, CST_CL | CST_IMPLSIZE },
		{ ACT_PUT2, 0x0F, 0xAD, ACT_MEM },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, MODRM2(0, 1),
		{ CST_RM32, CST_R32, CST_CL | CST_IMPLSIZE },
		{ ACT_PUT2, 0x0F, 0xAD, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(0, 1),
		{ CST_RM64, CST_R64, CST_CL | CST_IMPLSIZE },
		{ ACT_PUT2, 0x0F, 0xAD, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_sidt_1[] =
{
	{
		CPU_286, CPU_ALL,
		0, MODRM1(0),
		{ CST_M },
		{ ACT_PUT2, 0x0F, 0x01, ACT_MEM_I, 1 },
	},
};

static const instruction_pattern_t pattern_sldt_1[] =
{
	{
		CPU_286, CPU_ALL,
		0, MODRM1(0),
		{ CST_RM16 | CST_IMPLSIZE },
		{ ACT_PUT2, 0x0F, 0x00, ACT_MEM_I, 0 },
	},
};

static const instruction_pattern_t pattern_smint_0[] =
{
	{
		CPU_CYRIX, CPU_CYRIX,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x7E },
	},
	{
		CPU_MX, CPU_GX2,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x38 },
	},
};

static const instruction_pattern_t pattern_smsw_1[] =
{
	{
		CPU_286, CPU_ALL,
		0, MODRM1(0),
		{ CST_RM16 | CST_IMPLSIZE },
		{ ACT_PUT2, 0x0F, 0x01, ACT_MEM_I, 4 },
	},
};

static const instruction_pattern_t pattern_stc_0[] =
{
	{
		CPU_8086, CPU_ALL,
		0, NO_MODRM,
		{ },
		{ ACT_PUT, 0xF9 },
	},
};

static const instruction_pattern_t pattern_std_0[] =
{
	{
		CPU_8086, CPU_ALL,
		0, NO_MODRM,
		{ },
		{ ACT_PUT, 0xFD },
	},
};

static const instruction_pattern_t pattern_sti_0[] =
{
	{
		CPU_8086, CPU_ALL,
		0, NO_MODRM,
		{ },
		{ ACT_PUT, 0xFB },
	},
};

static const instruction_pattern_t pattern_stop_0[] =
{
	{
		CPU_V25, CPU_V55,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x9E },
	},
};

static const instruction_pattern_t pattern_storeall_0[] =
{
	{
		CPU_286, CPU_286,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x04 },
	},
};

static const instruction_pattern_t pattern_stos_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O8 | INS_NEEDSIZE, NO_MODRM,
		{ },
		{ ACT_PUT, 0xAA },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_NEEDSIZE, NO_MODRM,
		{ },
		{ ACT_PUT, 0xAB },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NEEDSIZE, NO_MODRM,
		{ },
		{ ACT_PUT, 0xAB },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_NEEDSIZE, NO_MODRM,
		{ },
		{ ACT_PUT, 0xAB },
	},
};

static const instruction_pattern_t pattern_stos_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O8 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI },
		{ ACT_PUT, 0xAA },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI },
		{ ACT_PUT, 0xAB },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI },
		{ ACT_PUT, 0xAB },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI },
		{ ACT_PUT, 0xAB },
	},
};

static const instruction_pattern_t pattern_stosb_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O8, NO_MODRM,
		{ },
		{ ACT_PUT, 0xAA },
	},
};

static const instruction_pattern_t pattern_stosb_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O8 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI },
		{ ACT_PUT, 0xAA },
	},
};

static const instruction_pattern_t pattern_stosw_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16, NO_MODRM,
		{ },
		{ ACT_PUT, 0xAB },
	},
};

static const instruction_pattern_t pattern_stosw_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI },
		{ ACT_PUT, 0xAB },
	},
};

static const instruction_pattern_t pattern_stosd_0[] =
{
	{
		CPU_386, CPU_ALL,
		INS_O32, NO_MODRM,
		{ },
		{ ACT_PUT, 0xAB },
	},
};

static const instruction_pattern_t pattern_stosd_1[] =
{
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI },
		{ ACT_PUT, 0xAB },
	},
};

static const instruction_pattern_t pattern_stosq_0[] =
{
	{
		CPU_X64, CPU_ALL,
		INS_O64, NO_MODRM,
		{ },
		{ ACT_PUT, 0xAB },
	},
};

static const instruction_pattern_t pattern_stosq_1[] =
{
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_NEEDSIZE, NO_MODRM,
		{ CST_ESDI },
		{ ACT_PUT, 0xAB },
	},
};

static const instruction_pattern_t pattern_str_1[] =
{
	{
		CPU_286, CPU_ALL,
		0, MODRM1(0),
		{ CST_RM16 | CST_IMPLSIZE },
		{ ACT_PUT2, 0x0F, 0x00, ACT_MEM_I, 1 },
	},
};

static const instruction_pattern_t pattern_sub4s_0[] =
{
	{
		CPU_V30, CPU_V55,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x22 },
	},
};

static const instruction_pattern_t pattern_sub4s_2[] =
{
	{
		CPU_V30, CPU_V55,
		0, NO_MODRM,
		{ CST_ESDI, CST_DSSI },
		{ ACT_PUT2, 0x0F, 0x22 },
	},
};

static const instruction_pattern_t pattern_svdc_2[] =
{
	{
		CPU_CYRIX, CPU_GX2,
		0, MODRM2(0, 1),
		{ CST_M | CST_NOSIZE, CST_SEG },
		{ ACT_PUT2, 0x0F, 0x78, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_svldt_1[] =
{
	{
		CPU_CYRIX, CPU_GX2,
		0, MODRM1(0),
		{ CST_M | CST_NOSIZE },
		{ ACT_PUT2, 0x0F, 0x7A, ACT_MEM_I, 0 },
	},
};

static const instruction_pattern_t pattern_svts_1[] =
{
	{
		CPU_CYRIX, CPU_GX2,
		0, MODRM1(0),
		{ CST_M | CST_NOSIZE },
		{ ACT_PUT2, 0x0F, 0x7C, ACT_MEM_I, 0 },
	},
};

static const instruction_pattern_t pattern_swapgs_0[] =
{
	{
		CPU_X64, CPU_ALL,
		0, NO_MODRM,
		{ },
		{ ACT_PUT3, 0x0F, 0x01, 0xF8 },
	},
};

static const instruction_pattern_t pattern_syscall_0[] =
{
	{
		CPU_686, CPU_ALL,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x05 },
	},
};

static const instruction_pattern_t pattern_sysenter_0[] =
{
	{
		CPU_686, CPU_ALL,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x34 },
	},
};

static const instruction_pattern_t pattern_sysexit_0[] =
{
	{
		CPU_686, CPU_ALL,
		INS_O32, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x35 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x35 },
	},
};

static const instruction_pattern_t pattern_sysret_0[] =
{
	{
		CPU_686, CPU_ALL,
		INS_O32, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x07 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x07 },
	},
};

static const instruction_pattern_t pattern_test_2[] =
{
	{
		CPU_8086, CPU_ALL,
		0, NO_MODRM,
		{ CST_AL, CST_I8 },
		{ ACT_PUT, 0xA8, ACT_I8_1 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16, NO_MODRM,
		{ CST_AX, CST_I16 },
		{ ACT_PUT, 0xA9, ACT_I16_1 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, NO_MODRM,
		{ CST_EAX, CST_I32 },
		{ ACT_PUT, 0xA9, ACT_I32_1 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, NO_MODRM,
		{ CST_RAX, CST_SI32 },
		{ ACT_PUT, 0xA9, ACT_I32_1 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O8 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM8, CST_I8 },
		{ ACT_PUT, 0xF6, ACT_MEM_I, 0, ACT_I8_1 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM16, CST_I16 },
		{ ACT_PUT, 0xF7, ACT_MEM_I, 0, ACT_I16_1 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM32, CST_I32 },
		{ ACT_PUT, 0xF7, ACT_MEM_I, 0, ACT_I32_1 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_NEEDSIZE, MODRM1(0),
		{ CST_RM64, CST_SI32 },
		{ ACT_PUT, 0xF7, ACT_MEM_I, 0, ACT_I32_1 },
	},
	{
		CPU_8086, CPU_ALL,
		0, MODRM2(0, 1),
		{ CST_RM8, CST_R8 },
		{ ACT_PUT, 0x84, ACT_MEM },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16, MODRM2(0, 1),
		{ CST_RM16, CST_R16 },
		{ ACT_PUT, 0x85, ACT_MEM },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, MODRM2(0, 1),
		{ CST_RM32, CST_R32 },
		{ ACT_PUT, 0x85, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(0, 1),
		{ CST_RM64, CST_R64 },
		{ ACT_PUT, 0x85, ACT_MEM },
	},
	// reversed operands
	{
		CPU_8086, CPU_ALL,
		0, MODRM2(1, 0),
		{ CST_R8, CST_RM8 },
		{ ACT_PUT, 0x84, ACT_MEM },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16, MODRM2(1, 0),
		{ CST_R16, CST_RM16 },
		{ ACT_PUT, 0x85, ACT_MEM },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, MODRM2(1, 0),
		{ CST_R32, CST_RM32 },
		{ ACT_PUT, 0x85, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(1, 0),
		{ CST_R64, CST_RM64 },
		{ ACT_PUT, 0x85, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_tsksw_1[] =
{
	{
		CPU_V25, CPU_V55,
		0, MODRM1(0),
		{ CST_R16 },
		{ ACT_PUT2, 0x0F, 0x94, ACT_MEM_I, 7 },
	},
};

static const instruction_pattern_t pattern_tzcnt_2[] =
{
	{
		CPU_X64, CPU_ALL,
		INS_O16 | INS_F3, MODRM2(1, 0),
		{ CST_R16, CST_RM16 },
		{ ACT_PUT2, 0x0F, 0xBC, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O32 | INS_F3, MODRM2(1, 0),
		{ CST_R32, CST_RM32 },
		{ ACT_PUT2, 0x0F, 0xBC, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_F3, MODRM2(1, 0),
		{ CST_R64, CST_RM64 },
		{ ACT_PUT2, 0x0F, 0xBC, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_ud0_0[] =
{
	{
		CPU_286, CPU_ALL,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0xFF },
	},
};

static const instruction_pattern_t pattern_ud0_2[] =
{
	{
		CPU_286, CPU_ALL,
		0, MODRM2(1, 0),
		{ CST_R32, CST_RM32 },
		{ ACT_PUT2, 0x0F, 0xFF, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_ud1_0[] =
{
	{
		CPU_286, CPU_ALL,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0xB9 },
	},
};

static const instruction_pattern_t pattern_ud1_2[] =
{
	{
		CPU_286, CPU_ALL,
		0, MODRM2(1, 0),
		{ CST_R32, CST_RM32 },
		{ ACT_PUT2, 0x0F, 0xB9, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_ud2_0[] =
{
	{
		CPU_286, CPU_ALL,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x0B },
	},
};

static const instruction_pattern_t pattern_umov_2[] =
{
	{
		CPU_8086, CPU_ALL,
		0, MODRM2(0, 1),
		{ CST_RM8, CST_R8 },
		{ ACT_PUT2, 0x0F, 0x10, ACT_MEM },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16, MODRM2(0, 1),
		{ CST_RM16, CST_R16 },
		{ ACT_PUT, 0x0F, 0x11, ACT_MEM },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, MODRM2(0, 1),
		{ CST_RM32, CST_R32 },
		{ ACT_PUT, 0x0F, 0x11, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(0, 1),
		{ CST_RM64, CST_R64 },
		{ ACT_PUT, 0x0F, 0x11, ACT_MEM },
	},
	{
		CPU_8086, CPU_ALL,
		0, MODRM2(1, 0),
		{ CST_R8, CST_RM8 },
		{ ACT_PUT, 0x0F, 0x12, ACT_MEM },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16, MODRM2(1, 0),
		{ CST_R16, CST_RM16 },
		{ ACT_PUT, 0x0F, 0x13, ACT_MEM },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, MODRM2(1, 0),
		{ CST_R32, CST_RM32 },
		{ ACT_PUT, 0x0F, 0x13, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(1, 0),
		{ CST_R64, CST_RM64 },
		{ ACT_PUT, 0x0F, 0x13, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_verr_1[] =
{
	{
		CPU_286, CPU_ALL,
		0, MODRM1(0),
		{ CST_RM16 | CST_IMPLSIZE },
		{ ACT_PUT2, 0x0F, 0x00, ACT_MEM_I, 4 },
	},
};

static const instruction_pattern_t pattern_verw_1[] =
{
	{
		CPU_286, CPU_ALL,
		0, MODRM1(0),
		{ CST_RM16 | CST_IMPLSIZE },
		{ ACT_PUT2, 0x0F, 0x00, ACT_MEM_I, 5 },
	},
};

static const instruction_pattern_t pattern_wait_0[] =
{
	{
		CPU_8086, CPU_ALL,
		0, NO_MODRM,
		{ },
		{ ACT_PUT, 0x9B },
	},
};

static const instruction_pattern_t pattern_wbinvd_0[] =
{
	{
		CPU_486, CPU_ALL,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x09 },
	},
};

static const instruction_pattern_t pattern_wrfsbase_1[] =
{
	{
		CPU_X64, CPU_ALL,
		INS_O32 | INS_F3, MODRM1(0),
		{ CST_R32, CST_RM32 },
		{ ACT_PUT2, 0x0F, 0xAE, ACT_MEM_I, 2 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_F3, MODRM1(0),
		{ CST_R64, CST_RM64 },
		{ ACT_PUT2, 0x0F, 0xAE, ACT_MEM_I, 2 },
	},
};

static const instruction_pattern_t pattern_wrgsbase_1[] =
{
	{
		CPU_X64, CPU_ALL,
		INS_O32 | INS_F3, MODRM1(0),
		{ CST_R32, CST_RM32 },
		{ ACT_PUT2, 0x0F, 0xAE, ACT_MEM_I, 3 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64 | INS_F3, MODRM1(0),
		{ CST_R64, CST_RM64 },
		{ ACT_PUT2, 0x0F, 0xAE, ACT_MEM_I, 3 },
	},
};

static const instruction_pattern_t pattern_wrmsr_0[] =
{
	{
		CPU_586, CPU_ALL,
		0, NO_MODRM,
		{ },
		{ ACT_PUT2, 0x0F, 0x30 },
	},
};

static const instruction_pattern_t pattern_wrshr_1[] =
{
	{
		CPU_MX, CPU_MX,
		0, MODRM1(0),
		{ CST_RM32 },
		{ ACT_PUT2, 0x0F, 0x37, ACT_MEM_I, 0 },
	},
};

static const instruction_pattern_t pattern_xadd_2[] =
{
	{
		CPU_486, CPU_ALL,
		INS_O8, MODRM2(0, 1),
		{ CST_RM8, CST_R8 },
		{ ACT_PUT2, 0x0F, 0xC0, ACT_MEM },
	},
	{
		CPU_486, CPU_ALL,
		INS_O16, MODRM2(0, 1),
		{ CST_RM16, CST_R16 },
		{ ACT_PUT2, 0x0F, 0xC1, ACT_MEM },
	},
	{
		CPU_486, CPU_ALL,
		INS_O32, MODRM2(0, 1),
		{ CST_RM32, CST_R32 },
		{ ACT_PUT2, 0x0F, 0xC1, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(0, 1),
		{ CST_RM64, CST_R64 },
		{ ACT_PUT2, 0x0F, 0xC1, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_xbts_4[] =
{
	{
		CPU_386, CPU_386,
		INS_O16, MODRM2(1, 0),
		{ CST_R16, CST_RM16, CST_AX, CST_CL | CST_IMPLSIZE },
		{ ACT_PUT2, 0x0F, 0xA6, ACT_MEM },
	},
	{
		CPU_386, CPU_386,
		INS_O32, MODRM2(1, 0),
		{ CST_R32, CST_RM32, CST_EAX, CST_CL | CST_IMPLSIZE },
		{ ACT_PUT2, 0x0F, 0xA6, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_xchg_2[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_O16, REGOPD(1),
		{ CST_AX, CST_R16 },
		{ ACT_ADD_REG, 0x90 },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16, REGOPD(0),
		{ CST_R16, CST_AX },
		{ ACT_ADD_REG, 0x90 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, REGOPD(1),
		{ CST_EAX, CST_R32 },
		{ ACT_ADD_REG, 0x90 },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, REGOPD(0),
		{ CST_R32, CST_RAX },
		{ ACT_ADD_REG, 0x90 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, REGOPD(1),
		{ CST_RAX, CST_R64 },
		{ ACT_ADD_REG, 0x90 },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, REGOPD(0),
		{ CST_R64, CST_RAX },
		{ ACT_ADD_REG, 0x90 },
	},
	{
		CPU_8086, CPU_ALL,
		0, MODRM2(0, 1),
		{ CST_RM8, CST_R8 },
		{ ACT_PUT, 0x86, ACT_MEM },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16, MODRM2(0, 1),
		{ CST_RM16, CST_R16 },
		{ ACT_PUT, 0x87, ACT_MEM },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, MODRM2(0, 1),
		{ CST_RM32, CST_R32 },
		{ ACT_PUT, 0x87, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(0, 1),
		{ CST_RM64, CST_R64 },
		{ ACT_PUT, 0x87, ACT_MEM },
	},
	// reversed operands
	{
		CPU_8086, CPU_ALL,
		0, MODRM2(1, 0),
		{ CST_R8, CST_RM8 },
		{ ACT_PUT, 0x86, ACT_MEM },
	},
	{
		CPU_8086, CPU_ALL,
		INS_O16, MODRM2(1, 0),
		{ CST_R16, CST_RM16 },
		{ ACT_PUT, 0x87, ACT_MEM },
	},
	{
		CPU_386, CPU_ALL,
		INS_O32, MODRM2(1, 0),
		{ CST_R32, CST_RM32 },
		{ ACT_PUT, 0x87, ACT_MEM },
	},
	{
		CPU_X64, CPU_ALL,
		INS_O64, MODRM2(1, 0),
		{ CST_R64, CST_RM64 },
		{ ACT_PUT, 0x87, ACT_MEM },
	},
};

static const instruction_pattern_t pattern_xlat_0[] =
{
	{
		CPU_8086, CPU_ALL,
		0, NO_MODRM,
		{ },
		{ ACT_PUT, 0xD7 },
	},
};

static const instruction_pattern_t pattern_xlat_1[] =
{
	{
		CPU_8086, CPU_ALL,
		0, NO_MODRM,
		{ CST_BXAL },
		{ ACT_PUT, 0xD7 },
	},
};

static const instruction_pattern_t pattern_f2xm1_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD9, 0xF0 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fabs_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD9, 0xE1 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fadd_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M32 },
		{ ACT_PUT, 0xD8, ACT_MEM_I, 0 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M64 },
		{ ACT_PUT, 0xDC, ACT_MEM_I, 0 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fadd_2[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, REGOPD(1),
		{ CST_ST0, CST_ST },
		{ ACT_PUT, 0xD8, ACT_ADD_REG, 0xC0 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87, REGOPD(0),
		{ CST_ST, CST_ST0 },
		{ ACT_PUT, 0xDC, ACT_ADD_REG, 0xC0 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_faddp_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xDE, 0xC1 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_faddp_2[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, REGOPD(0),
		{ CST_ST, CST_ST0 },
		{ ACT_PUT, 0xDE, ACT_ADD_REG, 0xC0 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fbld_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, MODRM1(0),
		{ CST_M80 },
		{ ACT_PUT, 0xDF, ACT_MEM_I, 4 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fbstp_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, MODRM1(0),
		{ CST_M80 },
		{ ACT_PUT, 0xDF, ACT_MEM_I, 6 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fchs_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD9, 0xE0 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fnclex_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xDB, 0xE2 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fclex_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT3, 0x9B, 0xDB, 0xE2 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fcmov_cc_2[] =
{
	{
		CPU_686, CPU_ALL,
		INS_X87, REGOPD(1),
		{ CST_ST0, CST_ST },
		{ ACT_ADD_FCMOV, 0xDA, 0xC0 },
		FPU_387,
	},
};

static const instruction_pattern_t pattern_fcom_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD8, 0xD1 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fcom_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, NO_MODRM,
		{ CST_M32 },
		{ ACT_PUT, 0xD8, ACT_MEM_I, 2 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, NO_MODRM,
		{ CST_M64 },
		{ ACT_PUT, 0xDC, ACT_MEM_I, 2 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87, REGOPD(0),
		{ CST_ST },
		{ ACT_PUT, 0xD8, ACT_ADD_REG, 0xD0 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fcomp_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD8, 0xD9 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fcomi_1[] =
{
	{
		CPU_686, CPU_ALL,
		INS_X87, REGOPD(0),
		{ CST_ST0, CST_ST },
		{ ACT_PUT, 0xDB, ACT_ADD_REG, 0xF0 },
		FPU_387,
	},
};

static const instruction_pattern_t pattern_fcomip_1[] =
{
	{
		CPU_686, CPU_ALL,
		INS_X87, REGOPD(0),
		{ CST_ST0, CST_ST },
		{ ACT_PUT, 0xDF, ACT_ADD_REG, 0xF0 },
		FPU_387,
	},
};

static const instruction_pattern_t pattern_fcomp_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, NO_MODRM,
		{ CST_M32 },
		{ ACT_PUT, 0xD8, ACT_MEM_I, 3 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, NO_MODRM,
		{ CST_M64 },
		{ ACT_PUT, 0xDC, ACT_MEM_I, 3 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87, REGOPD(0),
		{ CST_ST },
		{ ACT_PUT, 0xD8, ACT_ADD_REG, 0xD8 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fcompp_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xDE, 0xD9 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fcos_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD9, 0xFF },
		FPU_387,
	},
};

static const instruction_pattern_t pattern_fdecstp_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD9, 0xF6 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fndisi_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xDB, 0xE1 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fdisi_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT3, 0x9B, 0xDB, 0xE1 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fdiv_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M32 },
		{ ACT_PUT, 0xD8, ACT_MEM_I, 6 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M64 },
		{ ACT_PUT, 0xDC, ACT_MEM_I, 6 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fdiv_2[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, REGOPD(1),
		{ CST_ST0, CST_ST },
		{ ACT_PUT, 0xD8, ACT_ADD_REG, 0xF0 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87, REGOPD(0),
		{ CST_ST, CST_ST0 },
		{ ACT_PUT, 0xDC, ACT_ADD_REG, 0xF8 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fdivp_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xDE, 0xF9 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fdivp_2[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, REGOPD(0),
		{ CST_ST, CST_ST0 },
		{ ACT_PUT, 0xDE, ACT_ADD_REG, 0xF8 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fdivr_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M32 },
		{ ACT_PUT, 0xD8, ACT_MEM_I, 7 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M64 },
		{ ACT_PUT, 0xDC, ACT_MEM_I, 7 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fdivr_2[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, REGOPD(1),
		{ CST_ST0, CST_ST },
		{ ACT_PUT, 0xD8, ACT_ADD_REG, 0xF8 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87, REGOPD(0),
		{ CST_ST, CST_ST0 },
		{ ACT_PUT, 0xDC, ACT_ADD_REG, 0xF0 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fdivrp_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xDE, 0xF1 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fdivrp_2[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, REGOPD(0),
		{ CST_ST, CST_ST0 },
		{ ACT_PUT, 0xDE, ACT_ADD_REG, 0xF0 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fneni_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xDB, 0xE0 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_feni_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT3, 0x9B, 0xDB, 0xE0 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_ffree_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, REGOPD(0),
		{ CST_ST },
		{ ACT_PUT, 0xDD, ACT_ADD_REG, 0xC0 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_ffreep_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, REGOPD(0),
		{ CST_ST },
		{ ACT_PUT, 0xDF, ACT_ADD_REG, 0xC0 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fiadd_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M16 },
		{ ACT_PUT, 0xDE, ACT_MEM_I, 0 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M32 },
		{ ACT_PUT, 0xDA, ACT_MEM_I, 0 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_ficom_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M16 },
		{ ACT_PUT, 0xDE, ACT_MEM_I, 2 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M32 },
		{ ACT_PUT, 0xDA, ACT_MEM_I, 2 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_ficomp_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M16 },
		{ ACT_PUT, 0xDE, ACT_MEM_I, 3 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M32 },
		{ ACT_PUT, 0xDA, ACT_MEM_I, 3 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fidiv_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M16 },
		{ ACT_PUT, 0xDE, ACT_MEM_I, 6 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M32 },
		{ ACT_PUT, 0xDA, ACT_MEM_I, 6 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fidivr_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M16 },
		{ ACT_PUT, 0xDE, ACT_MEM_I, 7 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M32 },
		{ ACT_PUT, 0xDA, ACT_MEM_I, 7 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fild_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M16 },
		{ ACT_PUT, 0xDF, ACT_MEM_I, 0 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M32 },
		{ ACT_PUT, 0xDB, ACT_MEM_I, 0 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M64 },
		{ ACT_PUT, 0xDF, ACT_MEM_I, 5 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fimul_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M16 },
		{ ACT_PUT, 0xDE, ACT_MEM_I, 1 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M32 },
		{ ACT_PUT, 0xDA, ACT_MEM_I, 1 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fincstp_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD9, 0xF7 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fninit_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xDB, 0xE3 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_finit_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT3, 0x9B, 0xDB, 0xE3 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fist_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M16 },
		{ ACT_PUT, 0xDF, ACT_MEM_I, 2 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M32 },
		{ ACT_PUT, 0xDB, ACT_MEM_I, 2 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fistp_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M16 },
		{ ACT_PUT, 0xDF, ACT_MEM_I, 3 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M32 },
		{ ACT_PUT, 0xDB, ACT_MEM_I, 3 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M64 },
		{ ACT_PUT, 0xDB, ACT_MEM_I, 7 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fisttp_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M16 },
		{ ACT_PUT, 0xDF, ACT_MEM_I, 1 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M32 },
		{ ACT_PUT, 0xDB, ACT_MEM_I, 1 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M64 },
		{ ACT_PUT, 0xDD, ACT_MEM_I, 1 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fisub_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M16 },
		{ ACT_PUT, 0xDE, ACT_MEM_I, 4 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M32 },
		{ ACT_PUT, 0xDA, ACT_MEM_I, 4 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fisubr_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M16 },
		{ ACT_PUT, 0xDE, ACT_MEM_I, 5 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M32 },
		{ ACT_PUT, 0xDA, ACT_MEM_I, 5 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fld_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M32 },
		{ ACT_PUT, 0xD9, ACT_MEM_I, 0 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M64 },
		{ ACT_PUT, 0xDD, ACT_MEM_I, 0 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M80 },
		{ ACT_PUT, 0xDB, ACT_MEM_I, 5 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87, REGOPD(0),
		{ CST_ST },
		{ ACT_PUT, 0xD9, ACT_ADD_REG, 0xC0 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fld1_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD9, 0xE8 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fldcw_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, MODRM1(0),
		{ CST_M },
		{ ACT_PUT, 0xD9, ACT_MEM_I, 5 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fldenv_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, MODRM1(0),
		{ CST_M },
		{ ACT_PUT, 0xD9, ACT_MEM_I, 4 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fldl2e_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD9, 0xEA },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fldl2t_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD9, 0xE9 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fldlg2_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD9, 0xEC },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fldln2_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD9, 0xED },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fldpi_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD9, 0xEB },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fldz_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD9, 0xEE },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fmul_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M32 },
		{ ACT_PUT, 0xD8, ACT_MEM_I, 1 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M64 },
		{ ACT_PUT, 0xDC, ACT_MEM_I, 1 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fmul_2[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, REGOPD(1),
		{ CST_ST0, CST_ST },
		{ ACT_PUT, 0xD8, ACT_ADD_REG, 0xC8 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87, REGOPD(0),
		{ CST_ST, CST_ST0 },
		{ ACT_PUT, 0xDC, ACT_ADD_REG, 0xC8 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fmulp_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xDE, 0xC9 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fmulp_2[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, REGOPD(0),
		{ CST_ST, CST_ST0 },
		{ ACT_PUT, 0xDE, ACT_ADD_REG, 0xC8 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fnop_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD9, 0xD0 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fpatan_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD9, 0xF3 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fprem_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD9, 0xF8 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fprem1_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD9, 0xF5 },
		FPU_387,
	},
};

static const instruction_pattern_t pattern_fptan_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD9, 0xF2 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_frndint_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD9, 0xFC },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_frstor_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, MODRM1(0),
		{ CST_M },
		{ ACT_PUT, 0xDD, ACT_MEM_I, 4 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_frstpm_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xDB, 0xF4 }, // TODO: or 0xDB, 0xE5
		FPU_287,
	},
};

static const instruction_pattern_t pattern_fnsave_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, MODRM1(0),
		{ CST_M },
		{ ACT_PUT, 0xDD, ACT_MEM_I, 6 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fsave_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, MODRM1(0),
		{ CST_M },
		{ ACT_PUT2, 0x9B, 0xDD, ACT_MEM_I, 6 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fscale_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD9, 0xFD },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fnsetpm_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xDB, 0xE4 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fsetpm_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT3, 0x9B, 0xDB, 0xE4 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fsin_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD9, 0xFE },
		FPU_387,
	},
};

static const instruction_pattern_t pattern_fsincos_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD9, 0xFB },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fsqrt_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD9, 0xFA },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fst_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, NO_MODRM,
		{ CST_M32 },
		{ ACT_PUT, 0xD9, ACT_MEM_I, 2 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, NO_MODRM,
		{ CST_M64 },
		{ ACT_PUT, 0xDD, ACT_MEM_I, 2 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87, REGOPD(0),
		{ CST_ST },
		{ ACT_PUT, 0xDD, ACT_ADD_REG, 0xD0 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fnstcw_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, MODRM1(0),
		{ CST_M16 },
		{ ACT_PUT, 0xD9, ACT_MEM_I, 7 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fstcw_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, MODRM1(0),
		{ CST_M16 },
		{ ACT_PUT2, 0x9B, 0xD9, ACT_MEM_I, 7 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fnstdw_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ CST_AX },
		{ ACT_PUT2, 0xDF, 0xE1 },
		FPU_387,
	},
};

static const instruction_pattern_t pattern_fstdw_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ CST_AX },
		{ ACT_PUT3, 0x9B, 0xDF, 0xE1 },
		FPU_387,
	},
};

static const instruction_pattern_t pattern_fnstenv_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, MODRM1(0),
		{ CST_M },
		{ ACT_PUT, 0xD9, ACT_MEM_I, 6 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fstenv_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, MODRM1(0),
		{ CST_M },
		{ ACT_PUT2, 0x9B, 0xD9, ACT_MEM_I, 6 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fstpnce_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, REGOPD(0),
		{ CST_ST },
		{ ACT_PUT, 0xD9, ACT_ADD_REG, 0xD8 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fstp_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, NO_MODRM,
		{ CST_M32 },
		{ ACT_PUT, 0xD9, ACT_MEM_I, 3 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, NO_MODRM,
		{ CST_M64 },
		{ ACT_PUT, 0xDD, ACT_MEM_I, 3 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, NO_MODRM,
		{ CST_M80 },
		{ ACT_PUT, 0xDB, ACT_MEM_I, 7 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87, REGOPD(0),
		{ CST_ST },
		{ ACT_PUT, 0xDD, ACT_ADD_REG, 0xD8 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fnstsg_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ CST_AX },
		{ ACT_PUT2, 0xDF, 0xE2 },
		FPU_387,
	},
};

static const instruction_pattern_t pattern_fstsg_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ CST_AX },
		{ ACT_PUT3, 0x9B, 0xDF, 0xE2 },
		FPU_387,
	},
};

static const instruction_pattern_t pattern_fnstsw_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, MODRM1(0),
		{ CST_M16 },
		{ ACT_PUT, 0xDD, ACT_MEM_I, 7 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ CST_AX },
		{ ACT_PUT2, 0xDF, 0xE0 },
		FPU_287,
	},
};

static const instruction_pattern_t pattern_fstsw_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, MODRM1(0),
		{ CST_M16 },
		{ ACT_PUT2, 0x9B, 0xDD, ACT_MEM_I, 7 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ CST_AX },
		{ ACT_PUT3, 0x9B, 0xDF, 0xE0 },
		FPU_287,
	},
};

static const instruction_pattern_t pattern_fsub_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M32 },
		{ ACT_PUT, 0xD8, ACT_MEM_I, 4 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M64 },
		{ ACT_PUT, 0xDC, ACT_MEM_I, 4 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fsub_2[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, REGOPD(1),
		{ CST_ST0, CST_ST },
		{ ACT_PUT, 0xD8, ACT_ADD_REG, 0xE0 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87, REGOPD(0),
		{ CST_ST, CST_ST0 },
		{ ACT_PUT, 0xDC, ACT_ADD_REG, 0xE8 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fsubp_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xDE, 0xE9 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fsubp_2[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, REGOPD(0),
		{ CST_ST, CST_ST0 },
		{ ACT_PUT, 0xDE, ACT_ADD_REG, 0xE8 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fsubr_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M32 },
		{ ACT_PUT, 0xD8, ACT_MEM_I, 5 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87 | INS_NEEDSIZE, MODRM1(0),
		{ CST_M64 },
		{ ACT_PUT, 0xDC, ACT_MEM_I, 5 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fsubr_2[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, REGOPD(1),
		{ CST_ST0, CST_ST },
		{ ACT_PUT, 0xD8, ACT_ADD_REG, 0xE8 },
		FPU_8087,
	},
	{
		CPU_8086, CPU_ALL,
		INS_X87, REGOPD(0),
		{ CST_ST, CST_ST0 },
		{ ACT_PUT, 0xDC, ACT_ADD_REG, 0xE0 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fsubrp_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xDE, 0xE1 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fsubrp_2[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, REGOPD(0),
		{ CST_ST, CST_ST0 },
		{ ACT_PUT, 0xDE, ACT_ADD_REG, 0xE0 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_ftst_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD9, 0xE4 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fucom_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xDD, 0xE1 },
		FPU_387,
	},
};

static const instruction_pattern_t pattern_fucom_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, REGOPD(0),
		{ CST_ST },
		{ ACT_PUT, 0xDD, ACT_ADD_REG, 0xE0 },
		FPU_387,
	},
};

static const instruction_pattern_t pattern_fucomi_1[] =
{
	{
		CPU_686, CPU_ALL,
		INS_X87, REGOPD(0),
		{ CST_ST0, CST_ST },
		{ ACT_PUT, 0xDB, ACT_ADD_REG, 0xE8 },
		FPU_387,
	},
};

static const instruction_pattern_t pattern_fucomip_1[] =
{
	{
		CPU_686, CPU_ALL,
		INS_X87, REGOPD(0),
		{ CST_ST0, CST_ST },
		{ ACT_PUT, 0xDF, ACT_ADD_REG, 0xE8 },
		FPU_387,
	},
};

static const instruction_pattern_t pattern_fucomp_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xDD, 0xE9 },
		FPU_387,
	},
};

static const instruction_pattern_t pattern_fucomp_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, REGOPD(0),
		{ CST_ST },
		{ ACT_PUT, 0xDD, ACT_ADD_REG, 0xE8 },
		FPU_387,
	},
};

static const instruction_pattern_t pattern_fucompp_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xDA, 0xE9 },
		FPU_387,
	},
};

static const instruction_pattern_t pattern_fxam_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD9, 0xE5 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fxch_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD9, 0xC9 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fxch_1[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, REGOPD(0),
		{ CST_ST },
		{ ACT_PUT, 0xD9, ACT_ADD_REG, 0xC8 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fxtract_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD9, 0xF4 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fyl2x_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD9, 0xF1 },
		FPU_8087,
	},
};

static const instruction_pattern_t pattern_fyl2xp1_0[] =
{
	{
		CPU_8086, CPU_ALL,
		INS_X87, NO_MODRM,
		{ },
		{ ACT_PUT2, 0xD9, 0xF9 },
		FPU_8087,
	},
};

#define PATTERN(__pattern) { (__pattern), sizeof(__pattern) / sizeof((__pattern)[0]) }
#define NOPATTERN { NULL, 0 }

static const pattern_t x86_patterns[_MNEM_TOTAL] =
{
	[MNEM_AAA] = { { PATTERN(pattern_aaa_0) } },
	[MNEM_AAD] = { { PATTERN(pattern_aad_0), PATTERN(pattern_aad_1) } },
	[MNEM_AAM] = { { PATTERN(pattern_aam_0), PATTERN(pattern_aam_1) } },
	[MNEM_AAS] = { { PATTERN(pattern_aas_0) } },
	[MNEM_ADC] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_adc_2) } },
	[MNEM_ADD] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_add_2) } },
	[MNEM_ADD4S] = { { PATTERN(pattern_add4s_0), NOPATTERN, PATTERN(pattern_add4s_2) } },
	[MNEM_AND] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_and_2) } },
	[MNEM_ALBIT] = { { PATTERN(pattern_albit_0) } },
	[MNEM_ARPL] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_arpl_2) } },
	[MNEM_BB0_RESET] = { { PATTERN(pattern_bb0_reset_0) } },
	[MNEM_BB1_RESET] = { { PATTERN(pattern_bb1_reset_0) } },
	[MNEM_BOUND] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_bound_2) } },
	[MNEM_BRK] = { { NOPATTERN, PATTERN(pattern_brk_1) } },
	[MNEM_BRKCS] = { { NOPATTERN, PATTERN(pattern_brkcs_1) } },
	[MNEM_BRKEM] = { { NOPATTERN, PATTERN(pattern_brkem_1) } },
	[MNEM_BRKEM2] = { { NOPATTERN, PATTERN(pattern_brkem2_1) } },
	[MNEM_BRKN] = { { NOPATTERN, PATTERN(pattern_brkn_1) } },
	[MNEM_BRKS] = { { NOPATTERN, PATTERN(pattern_brks_1) } },
	[MNEM_BRKXA] = { { NOPATTERN, PATTERN(pattern_brkxa_1) } },
	[MNEM_BSCH] = { { NOPATTERN, PATTERN(pattern_bsch_1) } },
	[MNEM_BSF] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_bsf_2) } },
	[MNEM_BSR] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_bsr_2) } },
	[MNEM_BSWAP] = { { NOPATTERN, PATTERN(pattern_bswap_1) } },
	[MNEM_BT] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_bt_2) } },
	[MNEM_BTC] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_btc_2) } },
	[MNEM_BTCLR] = { { NOPATTERN, NOPATTERN, NOPATTERN, PATTERN(pattern_btclr_3) } },
	[MNEM_BTCLRL] = { { NOPATTERN, NOPATTERN, NOPATTERN, PATTERN(pattern_btclrl_3) } },
	[MNEM_BTR] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_btr_2) } },
	[MNEM_BTS] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_bts_2) } },
	[MNEM_CALL] = { { NOPATTERN, PATTERN(pattern_call_1) } },
	[MNEM_CBW] = { { PATTERN(pattern_cbw_0) } },
	[MNEM_CDQ] = { { PATTERN(pattern_cdq_0) } },
	[MNEM_CDQE] = { { PATTERN(pattern_cdqe_0) } },
	[MNEM_CLC] = { { PATTERN(pattern_clc_0) } },
	[MNEM_CLD] = { { PATTERN(pattern_cld_0) } },
	[MNEM_CLI] = { { PATTERN(pattern_cli_0) } },
	[MNEM_CLR1] = { { NOPATTERN, PATTERN(pattern_clr1_1), PATTERN(pattern_clr1_2) } },
	[MNEM_CLTS] = { { PATTERN(pattern_clts_0) } },
	[MNEM_CMC] = { { PATTERN(pattern_cmc_0) } },
	[MNEM_CMOV_CC] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_cmov_cc_2) } },
	[MNEM_CMP] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_cmp_2) } },
	[MNEM_CMP4S] = { { PATTERN(pattern_cmp4s_0), NOPATTERN, PATTERN(pattern_cmp4s_2) } },
	[MNEM_CMPS] = { { PATTERN(pattern_cmps_0), NOPATTERN, PATTERN(pattern_cmps_2) } },
	[MNEM_CMPSB] = { { PATTERN(pattern_cmpsb_0), NOPATTERN, PATTERN(pattern_cmpsb_2) } },
	[MNEM_CMPSW] = { { PATTERN(pattern_cmpsw_0), NOPATTERN, PATTERN(pattern_cmpsw_2) } },
	[MNEM_CMPSD] = { { PATTERN(pattern_cmpsd_0), NOPATTERN, PATTERN(pattern_cmpsd_2) } },
	[MNEM_CMPSQ] = { { PATTERN(pattern_cmpsq_0), NOPATTERN, PATTERN(pattern_cmpsq_2) } },
	[MNEM_CMPXCHG] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_cmpxchg_2) } },
	[MNEM_CMPXCHG8B] = { { NOPATTERN, PATTERN(pattern_cmpxchg8b_1) } },
	[MNEM_CMPXCHG16B] = { { NOPATTERN, PATTERN(pattern_cmpxchg16b_1) } },
	[MNEM_CNVTRP] = { { PATTERN(pattern_cnvtrp_0) } },
	[MNEM_COLTRP] = { { PATTERN(pattern_coltrp_0) } },
	[MNEM_CPU_READ] = { { PATTERN(pattern_cpu_read_0) } },
	[MNEM_CPU_WRITE] = { { PATTERN(pattern_cpu_write_0) } },
	[MNEM_CPUID] = { { PATTERN(pattern_cpuid_0) } },
	[MNEM_CQO] = { { PATTERN(pattern_cqo_0) } },
	[MNEM_CWD] = { { PATTERN(pattern_cwd_0) } },
	[MNEM_CWDE] = { { PATTERN(pattern_cwde_0) } },
	[MNEM_DAA] = { { PATTERN(pattern_daa_0) } },
	[MNEM_DAS] = { { PATTERN(pattern_das_0) } },
	[MNEM_DEC] = { { NOPATTERN, PATTERN(pattern_dec_1) } },
	[MNEM_DIV] = { { NOPATTERN, PATTERN(pattern_div_1) } },
	[MNEM_DMINT] = { { PATTERN(pattern_dmint_0) } },
	[MNEM_ENTER] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_enter_2) } },
	[MNEM_EXT] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_ext_2) } },
	[MNEM_FINT] = { { PATTERN(pattern_fint_0) } },
	[MNEM_GETBIT] = { { PATTERN(pattern_getbit_0) } },
	[MNEM_HLT] = { { PATTERN(pattern_hlt_0) } },
	[MNEM_IBTS] = { { NOPATTERN, NOPATTERN, NOPATTERN, NOPATTERN, PATTERN(pattern_ibts_4) } },
	[MNEM_IDIV] = { { NOPATTERN, PATTERN(pattern_idiv_1) } },
	[MNEM_IDLE] = { { PATTERN(pattern_idle_0) } },
	[MNEM_IMUL] = { { NOPATTERN, PATTERN(pattern_imul_1), PATTERN(pattern_imul_2), PATTERN(pattern_imul_3) } },
	[MNEM_IN] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_in_2) } },
	[MNEM_INC] = { { NOPATTERN, PATTERN(pattern_inc_1) } },
	[MNEM_INS] = { { PATTERN(pattern_ins_0), NOPATTERN, PATTERN(pattern_ins_2) } },
	[MNEM_INSB] = { { PATTERN(pattern_insb_0), NOPATTERN, PATTERN(pattern_insb_2) } },
	[MNEM_INSW] = { { PATTERN(pattern_insw_0), NOPATTERN, PATTERN(pattern_insw_2) } },
	[MNEM_INSD] = { { PATTERN(pattern_insd_0), NOPATTERN, PATTERN(pattern_insd_2) } },
	[MNEM_INS_NEC] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_ins_nec_2) } },
	[MNEM_INT] = { { NOPATTERN, PATTERN(pattern_int_1) } },
	[MNEM_INT1] = { { NOPATTERN, PATTERN(pattern_int1_0) } },
	[MNEM_INT3] = { { NOPATTERN, PATTERN(pattern_int3_0) } },
	[MNEM_INTO] = { { PATTERN(pattern_into_0) } },
	[MNEM_INVD] = { { PATTERN(pattern_invd_0) } },
	[MNEM_INVLPG] = { { NOPATTERN, PATTERN(pattern_invlpg_1) } },
	[MNEM_IRET] = { { PATTERN(pattern_iret_0) } },
	[MNEM_IRETD] = { { PATTERN(pattern_iretd_0) } },
	[MNEM_IRETQ] = { { PATTERN(pattern_iretq_0) } },
	[MNEM_J_CC] = { { NOPATTERN, PATTERN(pattern_j_cc_1) } },
	[MNEM_JCXZ] = { { NOPATTERN, PATTERN(pattern_jcxz_1) } },
	[MNEM_JECXZ] = { { NOPATTERN, PATTERN(pattern_jecxz_1) } },
	[MNEM_JRCXZ] = { { NOPATTERN, PATTERN(pattern_jrcxz_1) } },
	[MNEM_JMP] = { { NOPATTERN, PATTERN(pattern_jmp_1) } },
	[MNEM_JMPE] = { { NOPATTERN, PATTERN(pattern_jmpe_1) } },
	[MNEM_LAHF] = { { PATTERN(pattern_lahf_0) } },
	[MNEM_LAR] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_lar_2) } },
	[MNEM_LDEA] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_lea_2) } },
	[MNEM_LDS] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_lds_2) } },
	[MNEM_LEA] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_lea_2) } },
	[MNEM_LEAVE] = { { PATTERN(pattern_leave_0) } },
	[MNEM_LES] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_les_2) } },
	[MNEM_LFS] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_lfs_2) } },
	[MNEM_LGDT] = { { NOPATTERN, PATTERN(pattern_lgdt_1) } },
	[MNEM_LGS] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_lgs_2) } },
	[MNEM_LIDT] = { { NOPATTERN, PATTERN(pattern_lidt_1) } },
	[MNEM_LLDT] = { { NOPATTERN, PATTERN(pattern_lldt_1) } },
	[MNEM_LMSW] = { { NOPATTERN, PATTERN(pattern_lmsw_1) } },
	[MNEM_LOADALL] = { { PATTERN(pattern_loadall_0) } },
	[MNEM_LOADALL286] = { { PATTERN(pattern_loadall286_0) } },
	[MNEM_LOADALL386] = { { PATTERN(pattern_loadall386_0) } },
	[MNEM_LODS] = { { PATTERN(pattern_lods_0), PATTERN(pattern_lods_1) } },
	[MNEM_LODSB] = { { PATTERN(pattern_lodsb_0), PATTERN(pattern_lodsb_1) } },
	[MNEM_LODSW] = { { PATTERN(pattern_lodsw_0), PATTERN(pattern_lodsw_1) } },
	[MNEM_LODSD] = { { PATTERN(pattern_lodsd_0), PATTERN(pattern_lodsd_1) } },
	[MNEM_LODSQ] = { { PATTERN(pattern_lodsq_0), PATTERN(pattern_lodsq_1) } },
	[MNEM_LOOP] = { { NOPATTERN, PATTERN(pattern_loop_1) } },
	[MNEM_LOOPE] = { { NOPATTERN, PATTERN(pattern_loope_1) } },
	[MNEM_LOOPNE] = { { NOPATTERN, PATTERN(pattern_loopne_1) } },
	[MNEM_LSL] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_lsl_2) } },
	[MNEM_LSS] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_lss_2) } },
	[MNEM_LTR] = { { NOPATTERN, PATTERN(pattern_ltr_1) } },
	[MNEM_LZCNT] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_lzcnt_2) } },
	[MNEM_MHDEC] = { { PATTERN(pattern_mhdec_0) } },
	[MNEM_MHENC] = { { PATTERN(pattern_mhenc_0) } },
	[MNEM_MOV] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_mov_2) } },
	[MNEM_MOV_NEC] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_mov_2), PATTERN(pattern_mov_3) } },
	[MNEM_MOVS] = { { PATTERN(pattern_movs_0), NOPATTERN, PATTERN(pattern_movs_2) } },
	[MNEM_MOVSB] = { { PATTERN(pattern_movsb_0), NOPATTERN, PATTERN(pattern_movsb_2) } },
	[MNEM_MOVSW] = { { PATTERN(pattern_movsw_0), NOPATTERN, PATTERN(pattern_movsw_2) } },
	[MNEM_MOVSD] = { { PATTERN(pattern_movsd_0), NOPATTERN, PATTERN(pattern_movsd_2) } },
	[MNEM_MOVSQ] = { { PATTERN(pattern_movsq_0), NOPATTERN, PATTERN(pattern_movsq_2) } },
	[MNEM_MOVSPA] = { { PATTERN(pattern_movspa_0) } },
	[MNEM_MOVSPB] = { { NOPATTERN, PATTERN(pattern_movspb_1) } },
	[MNEM_MOVSX] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_movsx_2) } },
	[MNEM_MOVSXD] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_movsxd_2) } },
	[MNEM_MOVZX] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_movzx_2) } },
	[MNEM_MRDEC] = { { PATTERN(pattern_mrdec_0) } },
	[MNEM_MRENC] = { { PATTERN(pattern_mrenc_0) } },
	[MNEM_MUL] = { { NOPATTERN, PATTERN(pattern_mul_1) } },
	[MNEM_MUL_NEC] = { { NOPATTERN, PATTERN(pattern_imul_1), PATTERN(pattern_mul_nec_2), PATTERN(pattern_imul_3) } },
	[MNEM_NEG] = { { NOPATTERN, PATTERN(pattern_neg_1) } },
	[MNEM_NOP] = { { PATTERN(pattern_nop_0), PATTERN(pattern_nop_1) } },
	[MNEM_NOPL] = { { NOPATTERN, PATTERN(pattern_nop_1) } },
	[MNEM_NOT] = { { NOPATTERN, PATTERN(pattern_not_1) } },
	[MNEM_NOT1] = { { NOPATTERN, PATTERN(pattern_not1_1), PATTERN(pattern_not1_2) } },
	[MNEM_OR] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_or_2) } },
	[MNEM_OUT] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_out_2) } },
	[MNEM_OUTS] = { { PATTERN(pattern_outs_0), NOPATTERN, PATTERN(pattern_outs_2) } },
	[MNEM_OUTSB] = { { PATTERN(pattern_outsb_0), NOPATTERN, PATTERN(pattern_outsb_2) } },
	[MNEM_OUTSW] = { { PATTERN(pattern_outsw_0), NOPATTERN, PATTERN(pattern_outsw_2) } },
	[MNEM_OUTSD] = { { PATTERN(pattern_outsd_0), NOPATTERN, PATTERN(pattern_outsd_2) } },
	[MNEM_POP] = { { NOPATTERN, PATTERN(pattern_pop_1) } },
	[MNEM_POPA] = { { PATTERN(pattern_popa_0) } },
	[MNEM_POPAD] = { { PATTERN(pattern_popad_0) } },
	[MNEM_POPCNT] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_popcnt_2) } },
	[MNEM_POPF] = { { PATTERN(pattern_popf_0) } },
	[MNEM_POPFD] = { { PATTERN(pattern_popfd_0) } },
	[MNEM_POPFQ] = { { PATTERN(pattern_popfq_0) } },
	[MNEM_PUSH] = { { NOPATTERN, PATTERN(pattern_push_1) } },
	[MNEM_PUSHA] = { { PATTERN(pattern_pusha_0) } },
	[MNEM_PUSHAD] = { { PATTERN(pattern_pushad_0) } },
	[MNEM_PUSHF] = { { PATTERN(pattern_pushf_0) } },
	[MNEM_PUSHFD] = { { PATTERN(pattern_pushfd_0) } },
	[MNEM_PUSHFQ] = { { PATTERN(pattern_pushfq_0) } },
	[MNEM_QHOUT] = { { NOPATTERN, PATTERN(pattern_qhout_1) } },
	[MNEM_QOUT] = { { NOPATTERN, PATTERN(pattern_qout_1) } },
	[MNEM_QTIN] = { { NOPATTERN, PATTERN(pattern_qtin_1) } },
	[MNEM_RCL] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_rcl_2) } },
	[MNEM_RCR] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_rcr_2) } },
	[MNEM_RDM] = { { PATTERN(pattern_rdm_0) } },
	[MNEM_RDFSBASE] = { { NOPATTERN, PATTERN(pattern_rdfsbase_1) } },
	[MNEM_RDGSBASE] = { { NOPATTERN, PATTERN(pattern_rdgsbase_1) } },
	[MNEM_RDMSR] = { { PATTERN(pattern_rdmsr_0) } },
	[MNEM_RDPID] = { { NOPATTERN, PATTERN(pattern_rdpid_1) } },
	[MNEM_RDPMC] = { { PATTERN(pattern_rdpmc_0) } },
	[MNEM_RDSHR] = { { NOPATTERN, PATTERN(pattern_rdshr_1) } },
	[MNEM_RDTSC] = { { PATTERN(pattern_rdtsc_0) } },
	[MNEM_RDTSCP] = { { PATTERN(pattern_rdtscp_0) } },
	[MNEM_RET] = { { PATTERN(pattern_ret_0), PATTERN(pattern_ret_1) } },
	[MNEM_RETW] = { { PATTERN(pattern_retw_0), PATTERN(pattern_retw_1) } },
	[MNEM_RETD] = { { PATTERN(pattern_retd_0), PATTERN(pattern_retd_1) } },
	[MNEM_RETQ] = { { PATTERN(pattern_retq_0), PATTERN(pattern_retq_1) } },
	[MNEM_RETF] = { { PATTERN(pattern_retf_0), PATTERN(pattern_retf_1) } },
	[MNEM_RETFW] = { { PATTERN(pattern_retfw_0), PATTERN(pattern_retfw_1) } },
	[MNEM_RETFD] = { { PATTERN(pattern_retfd_0), PATTERN(pattern_retfd_1) } },
	[MNEM_RETFQ] = { { PATTERN(pattern_retfq_0), PATTERN(pattern_retfq_1) } },
	[MNEM_RETI] = { { PATTERN(pattern_iret_0) } },
	[MNEM_RETRBI] = { { PATTERN(pattern_retrbi_0) } },
	[MNEM_RETXA] = { { NOPATTERN, PATTERN(pattern_retxa_1) } },
	[MNEM_ROL] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_rol_2) } },
	[MNEM_ROL4] = { { NOPATTERN, PATTERN(pattern_rol4_1) } },
	[MNEM_ROR] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_ror_2) } },
	[MNEM_ROR4] = { { NOPATTERN, PATTERN(pattern_ror4_1) } },
	[MNEM_RSDC] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_rsdc_2) } },
	[MNEM_RSM] = { { PATTERN(pattern_rsm_0) } },
	[MNEM_RSLDT] = { { NOPATTERN, PATTERN(pattern_rsldt_1) } },
	[MNEM_RSTS] = { { NOPATTERN, PATTERN(pattern_rsts_1) } },
	[MNEM_RSTWDT] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_rstwdt_2) } },
	[MNEM_SAHF] = { { PATTERN(pattern_sahf_0) } },
	[MNEM_SAL] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_sal_2) } },
	[MNEM_SALC] = { { PATTERN(pattern_salc_0) } },
	[MNEM_SAR] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_sar_2) } },
	[MNEM_SBB] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_sbb_2) } },
	[MNEM_SCAS] = { { PATTERN(pattern_scas_0), PATTERN(pattern_scas_1) } },
	[MNEM_SCASB] = { { PATTERN(pattern_scasb_0), PATTERN(pattern_scasb_1) } },
	[MNEM_SCASW] = { { PATTERN(pattern_scasw_0), PATTERN(pattern_scasw_1) } },
	[MNEM_SCASD] = { { PATTERN(pattern_scasd_0), PATTERN(pattern_scasd_1) } },
	[MNEM_SCASQ] = { { PATTERN(pattern_scasq_0), PATTERN(pattern_scasq_1) } },
	[MNEM_SCHEOL] = { { PATTERN(pattern_scheol_0) } },
	[MNEM_SET1] = { { NOPATTERN, PATTERN(pattern_set1_1), PATTERN(pattern_set1_2) } },
	[MNEM_SET_CC] = { { NOPATTERN, PATTERN(pattern_set_cc_1) } },
	[MNEM_SETMO] = { { NOPATTERN, PATTERN(pattern_setmo_1), PATTERN(pattern_setmo_2) } },
	[MNEM_SETMOC] = { { NOPATTERN, PATTERN(pattern_setmoc_1) } },
	[MNEM_SGDT] = { { NOPATTERN, PATTERN(pattern_sgdt_1) } },
	[MNEM_SHL] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_shl_2) } },
	[MNEM_SHLD] = { { NOPATTERN, NOPATTERN, NOPATTERN, PATTERN(pattern_shld_3) } },
	[MNEM_SHR] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_shr_2) } },
	[MNEM_SHRD] = { { NOPATTERN, NOPATTERN, NOPATTERN, PATTERN(pattern_shrd_3) } },
	[MNEM_SIDT] = { { NOPATTERN, PATTERN(pattern_sidt_1) } },
	[MNEM_SLDT] = { { NOPATTERN, PATTERN(pattern_sldt_1) } },
	[MNEM_SMINT] = { { PATTERN(pattern_smint_0) } },
	[MNEM_SMSW] = { { NOPATTERN, PATTERN(pattern_smsw_1) } },
	[MNEM_STC] = { { PATTERN(pattern_stc_0) } },
	[MNEM_STD] = { { PATTERN(pattern_std_0) } },
	[MNEM_STI] = { { PATTERN(pattern_sti_0) } },
	[MNEM_STOP] = { { PATTERN(pattern_stop_0) } },
	[MNEM_STOREALL] = { { PATTERN(pattern_storeall_0) } },
	[MNEM_STOS] = { { PATTERN(pattern_stos_0), PATTERN(pattern_stos_1) } },
	[MNEM_STOSB] = { { PATTERN(pattern_stosb_0), PATTERN(pattern_stosb_1) } },
	[MNEM_STOSW] = { { PATTERN(pattern_stosw_0), PATTERN(pattern_stosw_1) } },
	[MNEM_STOSD] = { { PATTERN(pattern_stosd_0), PATTERN(pattern_stosd_1) } },
	[MNEM_STOSQ] = { { PATTERN(pattern_stosq_0), PATTERN(pattern_stosq_1) } },
	[MNEM_STR] = { { NOPATTERN, PATTERN(pattern_str_1) } },
	[MNEM_SUB] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_sub_2) } },
	[MNEM_SUB4S] = { { PATTERN(pattern_sub4s_0), NOPATTERN, PATTERN(pattern_sub4s_2) } },
	[MNEM_SVDC] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_svdc_2) } },
	[MNEM_SVLDT] = { { NOPATTERN, PATTERN(pattern_svldt_1) } },
	[MNEM_SVTS] = { { NOPATTERN, PATTERN(pattern_svts_1) } },
	[MNEM_SWAPGS] = { { PATTERN(pattern_swapgs_0) } },
	[MNEM_SYSCALL] = { { PATTERN(pattern_syscall_0) } },
	[MNEM_SYSENTER] = { { PATTERN(pattern_sysenter_0) } },
	[MNEM_SYSEXIT] = { { PATTERN(pattern_sysexit_0) } },
	[MNEM_SYSRET] = { { PATTERN(pattern_sysret_0) } },
	[MNEM_TEST] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_test_2) } },
	[MNEM_TEST1] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_test1_2) } },
	[MNEM_TSKSW] = { { NOPATTERN, PATTERN(pattern_tsksw_1) } },
	[MNEM_TZCNT] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_tzcnt_2) } },
	[MNEM_UD0] = { { PATTERN(pattern_ud0_0), NOPATTERN, PATTERN(pattern_ud0_2) } },
	[MNEM_UD1] = { { PATTERN(pattern_ud1_0), NOPATTERN, PATTERN(pattern_ud1_2) } },
	[MNEM_UD2] = { { PATTERN(pattern_ud2_0) } },
	[MNEM_UMOV] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_umov_2) } },
	[MNEM_VERR] = { { NOPATTERN, PATTERN(pattern_verr_1) } },
	[MNEM_VERW] = { { NOPATTERN, PATTERN(pattern_verw_1) } },
	[MNEM_WAIT] = { { PATTERN(pattern_wait_0) } },
	[MNEM_WBINVD] = { { PATTERN(pattern_wbinvd_0) } },
	[MNEM_WRFSBASE] = { { NOPATTERN, PATTERN(pattern_wrfsbase_1) } },
	[MNEM_WRGSBASE] = { { NOPATTERN, PATTERN(pattern_wrgsbase_1) } },
	[MNEM_WRMSR] = { { PATTERN(pattern_wrmsr_0) } },
	[MNEM_WRSHR] = { { NOPATTERN, PATTERN(pattern_wrshr_1) } },
	[MNEM_XADD] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_xadd_2) } },
	[MNEM_XBTS] = { { NOPATTERN, NOPATTERN, NOPATTERN, NOPATTERN, PATTERN(pattern_xbts_4) } },
	[MNEM_XCHG] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_xchg_2) } },
	[MNEM_XLAT] = { { PATTERN(pattern_xlat_0), PATTERN(pattern_xlat_1) } },
	[MNEM_XOR] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_xor_2) } },

	[MNEM_F2XM1] = { { PATTERN(pattern_f2xm1_0) } },
	[MNEM_FABS] = { { PATTERN(pattern_fabs_0) } },
	[MNEM_FADD] = { { NOPATTERN, PATTERN(pattern_fadd_1), PATTERN(pattern_fadd_2) } },
	[MNEM_FADDP] = { { PATTERN(pattern_faddp_0), NOPATTERN, PATTERN(pattern_faddp_2) } },
	[MNEM_FBLD] = { { NOPATTERN, PATTERN(pattern_fbld_1) } },
	[MNEM_FBSTP] = { { NOPATTERN, PATTERN(pattern_fbstp_1) } },
	[MNEM_FCHS] = { { PATTERN(pattern_fchs_0) } },
	[MNEM_FNCLEX] = { { PATTERN(pattern_fnclex_0) } },
	[MNEM_FCLEX] = { { PATTERN(pattern_fclex_0) } },
	[MNEM_FCMOV_CC] = { { NOPATTERN, NOPATTERN, PATTERN(pattern_fcmov_cc_2) } },
	[MNEM_FCOM] = { { PATTERN(pattern_fcom_0), PATTERN(pattern_fcom_1) } },
	[MNEM_FCOMI] = { { NOPATTERN, PATTERN(pattern_fcomi_1) } },
	[MNEM_FCOMIP] = { { NOPATTERN, PATTERN(pattern_fcomip_1) } },
	[MNEM_FCOMP] = { { PATTERN(pattern_fcomp_0), PATTERN(pattern_fcomp_1) } },
	[MNEM_FCOMPP] = { { PATTERN(pattern_fcompp_0) } },
	[MNEM_FCOS] = { { PATTERN(pattern_fcos_0) } },
	[MNEM_FDECSTP] = { { PATTERN(pattern_fdecstp_0) } },
	[MNEM_FNDISI] = { { PATTERN(pattern_fndisi_0) } },
	[MNEM_FDISI] = { { PATTERN(pattern_fdisi_0) } },
	[MNEM_FDIV] = { { NOPATTERN, PATTERN(pattern_fdiv_1), PATTERN(pattern_fdiv_2) } },
	[MNEM_FDIVP] = { { PATTERN(pattern_fdivp_0), NOPATTERN, PATTERN(pattern_fdivp_2) } },
	[MNEM_FDIVR] = { { NOPATTERN, PATTERN(pattern_fdivr_1), PATTERN(pattern_fdivr_2) } },
	[MNEM_FDIVRP] = { { PATTERN(pattern_fdivrp_0), NOPATTERN, PATTERN(pattern_fdivrp_2) } },
	[MNEM_FNENI] = { { PATTERN(pattern_fneni_0) } },
	[MNEM_FENI] = { { PATTERN(pattern_feni_0) } },
	[MNEM_FFREE] = { { NOPATTERN, PATTERN(pattern_ffree_1) } },
	[MNEM_FFREEP] = { { NOPATTERN, PATTERN(pattern_ffreep_1) } },
	[MNEM_FIADD] = { { NOPATTERN, PATTERN(pattern_fiadd_1) } },
	[MNEM_FICOM] = { { NOPATTERN, PATTERN(pattern_ficom_1) } },
	[MNEM_FICOMP] = { { NOPATTERN, PATTERN(pattern_ficomp_1) } },
	[MNEM_FIDIV] = { { NOPATTERN, PATTERN(pattern_fidiv_1) } },
	[MNEM_FIDIVR] = { { NOPATTERN, PATTERN(pattern_fidivr_1) } },
	[MNEM_FILD] = { { NOPATTERN, PATTERN(pattern_fild_1) } },
	[MNEM_FIMUL] = { { NOPATTERN, PATTERN(pattern_fimul_1) } },
	[MNEM_FINCSTP] = { { PATTERN(pattern_fincstp_0) } },
	[MNEM_FNINIT] = { { PATTERN(pattern_fninit_0) } },
	[MNEM_FINIT] = { { PATTERN(pattern_finit_0) } },
	[MNEM_FIST] = { { NOPATTERN, PATTERN(pattern_fist_1) } },
	[MNEM_FISTP] = { { NOPATTERN, PATTERN(pattern_fistp_1) } },
	[MNEM_FISTTP] = { { NOPATTERN, PATTERN(pattern_fisttp_1) } },
	[MNEM_FISUB] = { { NOPATTERN, PATTERN(pattern_fisub_1) } },
	[MNEM_FISUBR] = { { NOPATTERN, PATTERN(pattern_fisubr_1) } },
	[MNEM_FLD] = { { NOPATTERN, PATTERN(pattern_fld_1) } },
	[MNEM_FLD1] = { { PATTERN(pattern_fld1_0) } },
	[MNEM_FLDCW] = { { NOPATTERN, PATTERN(pattern_fldcw_1) } },
	[MNEM_FLDENV] = { { NOPATTERN, PATTERN(pattern_fldenv_1) } },
	[MNEM_FLDL2E] = { { PATTERN(pattern_fldl2e_0) } },
	[MNEM_FLDL2T] = { { PATTERN(pattern_fldl2t_0) } },
	[MNEM_FLDLG2] = { { PATTERN(pattern_fldlg2_0) } },
	[MNEM_FLDLN2] = { { PATTERN(pattern_fldln2_0) } },
	[MNEM_FLDPI] = { { PATTERN(pattern_fldpi_0) } },
	[MNEM_FLDZ] = { { PATTERN(pattern_fldz_0) } },
	[MNEM_FMUL] = { { NOPATTERN, PATTERN(pattern_fmul_1), PATTERN(pattern_fmul_2) } },
	[MNEM_FMULP] = { { PATTERN(pattern_fmulp_0), NOPATTERN, PATTERN(pattern_fmulp_2) } },
	[MNEM_FNOP] = { { PATTERN(pattern_fnop_0) } },
	[MNEM_FPATAN] = { { PATTERN(pattern_fpatan_0) } },
	[MNEM_FPREM] = { { PATTERN(pattern_fprem_0) } },
	[MNEM_FPREM1] = { { PATTERN(pattern_fprem1_0) } },
	[MNEM_FPTAN] = { { PATTERN(pattern_fptan_0) } },
	[MNEM_FRNDINT] = { { PATTERN(pattern_frndint_0) } },
	[MNEM_FRSTOR] = { { NOPATTERN, PATTERN(pattern_frstor_1) } },
	[MNEM_FRSTPM] = { { PATTERN(pattern_frstpm_0) } },
	[MNEM_FNSAVE] = { { NOPATTERN, PATTERN(pattern_fnsave_1) } },
	[MNEM_FSAVE] = { { NOPATTERN, PATTERN(pattern_fsave_1) } },
	[MNEM_FSCALE] = { { PATTERN(pattern_fscale_0) } },
	[MNEM_FNSETPM] = { { PATTERN(pattern_fnsetpm_0) } },
	[MNEM_FSETPM] = { { PATTERN(pattern_fsetpm_0) } },
	[MNEM_FSIN] = { { PATTERN(pattern_fsin_0) } },
	[MNEM_FSINCOS] = { { PATTERN(pattern_fsincos_0) } },
	[MNEM_FSQRT] = { { PATTERN(pattern_fsqrt_0) } },
	[MNEM_FST] = { { NOPATTERN, PATTERN(pattern_fst_1) } },
	[MNEM_FNSTCW] = { { NOPATTERN, PATTERN(pattern_fnstcw_1) } },
	[MNEM_FSTCW] = { { NOPATTERN, PATTERN(pattern_fstcw_1) } },
	[MNEM_FNSTDW] = { { NOPATTERN, PATTERN(pattern_fnstdw_1) } },
	[MNEM_FSTDW] = { { NOPATTERN, PATTERN(pattern_fstdw_1) } },
	[MNEM_FNSTENV] = { { NOPATTERN, PATTERN(pattern_fnstenv_1) } },
	[MNEM_FSTENV] = { { NOPATTERN, PATTERN(pattern_fstenv_1) } },
	[MNEM_FSTP] = { { NOPATTERN, PATTERN(pattern_fstp_1) } },
	[MNEM_FSTPNCE] = { { NOPATTERN, PATTERN(pattern_fstpnce_1) } },
	[MNEM_FNSTSG] = { { NOPATTERN, PATTERN(pattern_fnstsg_1) } },
	[MNEM_FSTSG] = { { NOPATTERN, PATTERN(pattern_fstsg_1) } },
	[MNEM_FNSTSW] = { { NOPATTERN, PATTERN(pattern_fnstsw_1) } },
	[MNEM_FSTSW] = { { NOPATTERN, PATTERN(pattern_fstsw_1) } },
	[MNEM_FSUB] = { { NOPATTERN, PATTERN(pattern_fsub_1), PATTERN(pattern_fsub_2) } },
	[MNEM_FSUBP] = { { PATTERN(pattern_fsubp_0), NOPATTERN, PATTERN(pattern_fsubp_2) } },
	[MNEM_FSUBR] = { { NOPATTERN, PATTERN(pattern_fsubr_1), PATTERN(pattern_fsubr_2) } },
	[MNEM_FSUBRP] = { { PATTERN(pattern_fsubrp_0), NOPATTERN, PATTERN(pattern_fsubrp_2) } },
	[MNEM_FTST] = { { PATTERN(pattern_ftst_0) } },
	[MNEM_FUCOM] = { { PATTERN(pattern_fucom_0), PATTERN(pattern_fucom_1) } },
	[MNEM_FUCOMI] = { { NOPATTERN, PATTERN(pattern_fucomi_1) } },
	[MNEM_FUCOMIP] = { { NOPATTERN, PATTERN(pattern_fucomip_1) } },
	[MNEM_FUCOMP] = { { PATTERN(pattern_fucomp_0), PATTERN(pattern_fucomp_1) } },
	[MNEM_FUCOMPP] = { { PATTERN(pattern_fucompp_0) } },
	[MNEM_FXAM] = { { PATTERN(pattern_fxam_0) } },
	[MNEM_FXCH] = { { PATTERN(pattern_fxch_0), PATTERN(pattern_fxch_1) } },
	[MNEM_FXTRACT] = { { PATTERN(pattern_fxtract_0) } },
	[MNEM_FYL2X] = { { PATTERN(pattern_fyl2x_0) } },
	[MNEM_FYL2XP1] = { { PATTERN(pattern_fyl2xp1_0) } },
};

