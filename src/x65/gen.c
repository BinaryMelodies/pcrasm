
#include "../asm.h"
#include "isa.h"

typedef enum addressing_mode_t
{
	// 65c816: ZPG is direct page; 65ce02: ZPG is base page
	// 65ce02: Z is Z register, zero otherwise
	MODE_IMPL,

	MODE_ZPG_X_IND,
	MODE_ZPG,
	MODE_IMM8,
	MODE_IMMA,
	MODE_IMMX,
	MODE_ABS,
	MODE_ZPG_IND_Y,
	MDOE_ZPG_X,
	MODE_ABS_Y,
	MODE_ABS_X,

	MODE_A,
	MODE_ZPG_Y,
	MODE_ABS_IND,

	MODE_ZPG_IND_Z,
	MODE_ABS_X_IND,

	MODE_STK,
	MODE_LNG_IND,
	MODE_LNG,
	MODE_STK_IND_Y,
	MODE_LNG_IND_Y,
	MODE_LNG_X,

	MODE_ABS_LNG_IND,

	MODE_IMM16,
	MODE_REL8,
	MODE_REL16,
	MODE_ZPG_REL8,
	MODE_IMM_IMM,

	_MODE_COUNT,
} addressing_mode_t;

typedef struct instruction_format_t
{
	uint8_t opcode;
//	cpu_type_t min_cpu, max_cpu;
} instruction_format_t;

static inline size_t x65_get_operand_count(instruction_t * ins, addressing_mode_t mode)
{
	switch(mode)
	{
	case MODE_IMPL:
	case MODE_A:
		return 0;
	case MODE_ZPG_REL8:
	case MODE_IMM_IMM:
		return 2;
	default:
		return 1;
	}
}

static inline size_t x65_get_operand_length(instruction_t * ins, addressing_mode_t mode, int operand_index)
{
	switch(mode)
	{
	case MODE_IMPL:
	case MODE_A:
		return 0;
	case MODE_ZPG_X_IND:
	case MODE_ZPG:
	case MODE_IMM8:
	case MODE_ZPG_IND_Y:
	case MDOE_ZPG_X:
	case MODE_ZPG_Y:
	case MODE_ZPG_IND_Z:
	case MODE_STK:
	case MODE_LNG_IND:
	case MODE_STK_IND_Y:
	case MODE_LNG_IND_Y:
	case MODE_REL8:
		return 1;
	case MODE_IMMA:
		return OCTETSIN(ins->abits);
	case MODE_IMMX:
		return OCTETSIN(ins->xbits);
	case MODE_ABS:
	case MODE_ABS_Y:
	case MODE_ABS_X:
	case MODE_ABS_IND:
	case MODE_ABS_X_IND:
	case MODE_ABS_LNG_IND:
	case MODE_IMM16:
	case MODE_REL16:
		return 2;
	case MODE_ZPG_REL8:
	case MODE_IMM_IMM:
		if(operand_index == -1)
			return 2;
		else
			return 1;
	case MODE_LNG:
	case MODE_LNG_X:
		return 3;
	default:
//		printf("Failed in line %d\n", ins->line_number);
		return -1;
	}
}

static inline bool x65_is_operand_pcrel(instruction_t * ins, addressing_mode_t mode, int operand_index)
{
	switch(mode)
	{
	case MODE_REL8:
	case MODE_REL16:
		return true;
	case MODE_ZPG_REL8:
		return operand_index == 1;
	default:
		return false;
	}
}

static inline uint32_t x65_get_elf_hint_type(instruction_t * ins, addressing_mode_t mode, int operand_index)
{
	uint32_t rel_type;

	switch(mode)
	{
	case MODE_IMPL:
	case MODE_A:
		rel_type = 0;
		break;
	case MODE_ZPG_X_IND:
	case MODE_ZPG:
		rel_type = R_MOS_ADDR8;
		break;
	case MODE_ZPG_IND_Y:
	case MDOE_ZPG_X:
	case MODE_ZPG_Y:
	case MODE_ZPG_IND_Z:
	case MODE_STK:
	case MODE_LNG_IND:
	case MODE_STK_IND_Y:
	case MODE_LNG_IND_Y:
	case MODE_IMM8:
		rel_type = R_MOS_IMM8;
		break;
	case MODE_REL8:
		rel_type = R_MOS_PCREL_8;
		break;
	case MODE_IMMA:
		rel_type = ins->abits == BITSIZE8 ? R_MOS_IMM8 : R_MOS_IMM16;
		break;
	case MODE_IMMX:
		rel_type = ins->xbits == BITSIZE8 ? R_MOS_IMM8 : R_MOS_IMM16;
		break;
	case MODE_ABS:
	case MODE_ABS_Y:
	case MODE_ABS_X:
	case MODE_ABS_IND:
	case MODE_ABS_X_IND:
	case MODE_ABS_LNG_IND:
		rel_type = R_MOS_ADDR16;
		break;
	case MODE_IMM16:
		rel_type = R_MOS_IMM16;
		break;
	case MODE_REL16:
		rel_type = R_MOS_PCREL_16;
		break;
	case MODE_ZPG_REL8:
		rel_type = operand_index == 1 ? R_MOS_PCREL_8 : R_MOS_ADDR8;
		break;
	case MODE_IMM_IMM:
		rel_type = R_MOS_IMM8;
		break;
	case MODE_LNG:
	case MODE_LNG_X:
		rel_type = R_MOS_ADDR24;
		break;
	default:
//		printf("Failed in line %d\n", ins->line_number);
		return 0;
	}

	if(rel_type == R_MOS_IMM8 || R_MOS_ADDR8)
	{
		switch(ins->operand[0].mode)
		{
		case MODE_HIGH_BYTE:
			rel_type = R_MOS_ADDR16_HI;
			break;
		case MODE_BANK:
			rel_type = R_MOS_ADDR24_BANK;
			break;
		default:
			break;
		}
	}

	return rel_type;
}

static bool expression_fits(instruction_t * ins, expression_t * exp, size_t bytes, bool pcrel)
{
	bool result;
	reference_t ref[1];
	evaluate_expression(exp, ref, ins->code_offset);
	if(!is_scalar(ref))
	{
		// TODO: what should be the reported size of undefined entries?
		result = 2 <= bytes;
	}
	else
	{
		if(pcrel)
		{
			uint_sub_ui(ref->value, ins->following->code_offset);
			result = integer_get_size(ref->value) <= bytes;
		}
		else
		{
			result = uinteger_get_size(ref->value) <= bytes;
		}
	}
	int_clear(ref->value);
	return result;
}

static bool operand_fits(instruction_t * ins, operand_t * opd, size_t bytes, bool pcrel)
{
	switch(opd->mode)
	{
	case MODE_DEFAULT:
		return expression_fits(ins, opd->parameter, bytes, pcrel);
	case MODE_LOW_BYTE:
	case MODE_HIGH_BYTE:
	case MODE_BANK:
		return 1 <= bytes;
	case MODE_WORD:
		return 2 <= bytes;
	case MODE_LONG:
		return 3 <= bytes;
	}
	assert(false);
}

static match_result_t check_operand_mode(const instruction_format_t * fmt, instruction_t * ins, addressing_mode_t mode, bool forgiving)
{
//	if(ins->cpu < fmt[mode].min_cpu || ins->cpu > fmt[mode].max_cpu)
//		return MATCH_FAILED; // TODO

	if(fmt == NULL)
	{
		return (match_result_t) { .type = MATCH_FAILED };
	}

	if(fmt[mode].opcode == 0x00 && !(ins->mnemonic == MNEM_BRK && mode == MODE_IMPL))
	{
		return (match_result_t) { .type = MATCH_FAILED };
	}

	if(forgiving)
	{
		return (match_result_t) { .type = MATCH_PERFECT };
	}

	for(size_t operand_index = 0; operand_index < x65_get_operand_count(ins, mode); operand_index++)
	{
		if(!operand_fits(ins, &ins->operand[0],
			x65_get_operand_length(ins, mode, operand_index),
			x65_is_operand_pcrel(ins, mode, operand_index)))
		{
			return (match_result_t) { .type = MATCH_TRUNCATED };
		}
	}

	return (match_result_t) { .type = MATCH_PERFECT };
}

static addressing_mode_t check_operand(const instruction_format_t * fmt, instruction_t * ins, bool forgiving)
{
	if(fmt == NULL)
		return -1;

#define _CHECK(__mode) \
		do { \
			match = check_operand_mode(fmt, ins, (__mode), forgiving); \
			if(match.type == MATCH_PERFECT) \
				return (__mode); \
			else if(match.type == MATCH_TRUNCATED) \
				best_match = (__mode); \
		} while(0)

	match_result_t match;
	addressing_mode_t best_match = -1;
	switch(ins->operand[0].type)
	{
	case OPD_NONE:
		_CHECK(MODE_IMPL);
		break;
	case OPD_IMM:
		_CHECK(MODE_IMM8); // must appear before MODE_IMMA, MODE_IMMX, MODE_IMM16
		_CHECK(MODE_IMMA); // must appear before MODE_IMM16
		_CHECK(MODE_IMMX); // must appear before MODE_IMM16
		_CHECK(MODE_IMM16);
		break;
	case OPD_REG_A:
		_CHECK(MODE_A);
		break;
	case OPD_MEM:
		_CHECK(MODE_ZPG); // must appear before MODE_ABS, MODE_LNG
		_CHECK(MODE_ABS); // must appear before MODE_LNG
		_CHECK(MODE_LNG);
		_CHECK(MODE_REL8); // must appear before MODE_REL16
		_CHECK(MODE_REL16);
		break;
	case OPD_MEM_X:
		_CHECK(MDOE_ZPG_X); // must appear before MODE_ABS_X, MODE_LNG_X
		_CHECK(MODE_ABS_X); // must appear before MODE_LNG_X
		_CHECK(MODE_LNG_X);
		break;
	case OPD_MEM_Y:
		_CHECK(MODE_ZPG_Y); // must appear before MODE_ABS_Y
		_CHECK(MODE_ABS_Y);
		break;
	case OPD_IND:
		if(ins->cpu != CPU_65CE02)
			_CHECK(MODE_ZPG_IND_Z);
		_CHECK(MODE_ABS_IND);
		break;
	case OPD_IND_X:
		_CHECK(MODE_ZPG_X_IND); // must appear before MODE_ABS_X_IND
		_CHECK(MODE_ABS_X_IND);
		break;
	case OPD_IND_Y:
		_CHECK(MODE_ZPG_IND_Y);
		break;
	case OPD_IND_Z:
		_CHECK(MODE_ZPG_IND_Z);
		break;
	case OPD_LNG:
		_CHECK(MODE_LNG_IND); // must appear before MODE_ABS_LNG_IND
		_CHECK(MODE_ABS_LNG_IND);
		break;
	case OPD_LNG_Y:
		_CHECK(MODE_LNG_IND_Y);
		break;
	case OPD_STK:
		_CHECK(MODE_STK);
		break;
	case OPD_STK_Y:
		_CHECK(MODE_STK_IND_Y);
		break;
	case OPD_PAIR:
		_CHECK(MODE_ZPG_REL8);
		_CHECK(MODE_IMM_IMM);
		break;
	}
	return best_match;
}

static const instruction_format_t * x65_6502_patterns[];
static const instruction_format_t * x65_65c02_patterns[];
static const instruction_format_t * x65_wdc65c02_patterns[];
static const instruction_format_t * x65_65ce02_patterns[];
static const instruction_format_t * x65_65c816_patterns[];

static inline const instruction_format_t * get_patterns(instruction_t * ins)
{
	switch(ins->cpu)
	{
	case CPU_6502:
		return x65_6502_patterns[ins->mnemonic];
	case CPU_65C02:
		return x65_65c02_patterns[ins->mnemonic];
	case CPU_WDC65C02:
		return x65_wdc65c02_patterns[ins->mnemonic];
	case CPU_65CE02:
		return x65_65ce02_patterns[ins->mnemonic];
	case CPU_65C816:
		return x65_65c816_patterns[ins->mnemonic];
	default:
		assert(false);
	}
}

size_t x65_instruction_compute_length(instruction_t * ins, bool forgiving)
{
	addressing_mode_t mode = check_operand(get_patterns(ins), ins, forgiving);
	if(mode == -1)
	{
		printf("Failed in line %ld\n", ins->line_number);
		return 0;
	}
	return 1 + x65_get_operand_length(ins, mode, -1);
}

static inline size_t get_hint(instruction_t * ins, addressing_mode_t mode)
{
	switch(mode)
	{
	default:
		return 0;
	}
}

int x65_expression_get_hint(operand_t * opd, reference_t * ref, int fmt, size_t size, bool pcrel)
{
	switch(opd->mode)
	{
	case MODE_LOW_BYTE:
		int_and_ui(ref->value, 0xFF);
		return R_MOS_IMM8; //R_MOS_ADDR16_LO;
	case MODE_HIGH_BYTE:
		uint_shr(ref->value, 8);
		int_and_ui(ref->value, 0xFF);
		return R_MOS_ADDR16_HI;
	case MODE_WORD:
		int_and_ui(ref->value, 0xFFFF);
		return R_MOS_ADDR16;
	case MODE_LONG:
		return R_MOS_ADDR24;
	case MODE_BANK:
		uint_shr(ref->value, 16);
		int_and_ui(ref->value, 0xFF);
		return R_MOS_ADDR24_BANK;
	default:
		break;
	}

	return 0;
}

void x65_generate_instruction(instruction_t * ins)
{
	addressing_mode_t mode = check_operand(get_patterns(ins), ins, false);
	if(mode == -1)
	{
//		printf("Failed in line %d\n", ins->line_number);
		return; // failed
	}

	output_byte(get_patterns(ins)[mode].opcode);

	reference_t ref[2];
	evaluate_expression(ins->operand[0].parameters[0], &ref[0], ins->code_offset);
	evaluate_expression(ins->operand[0].parameters[1], &ref[1], ins->code_offset);

	for(size_t operand_index = 0; operand_index < x65_get_operand_count(ins, mode); operand_index++)
	{
		int generated_operand_index;
		if(mode == MODE_IMM_IMM)
			generated_operand_index = 1 - operand_index;
		else
			generated_operand_index = operand_index;
		size_t bytes = x65_get_operand_length(ins, mode, generated_operand_index);

		switch(ins->operand[0].mode)
		{
		case MODE_HIGH_BYTE:
			uint_shr(ref[generated_operand_index].value, 8);
			break;
		case MODE_BANK:
			uint_shr(ref[generated_operand_index].value, 16);
			break;
		default:
			break;
		}

		if(x65_is_operand_pcrel(ins, mode, generated_operand_index))
		{
			uint_sub_ui(ref[generated_operand_index].value, ins->following->code_offset);
			output_word_as(&ref[generated_operand_index], DATA_LE, bytes, true, x65_get_elf_hint_type(ins, mode, generated_operand_index));
		}
		else
		{
			output_word_as(&ref[generated_operand_index], DATA_LE, bytes, false, x65_get_elf_hint_type(ins, mode, generated_operand_index));
		}
	}
}

#define _CPUNAME 6502
#include "gen.h"

#define _CPUNAME 65c02
#define _INCLUDE_65C02
#include "gen.h"

#define _CPUNAME wdc65c02
#define _INCLUDE_WDC65C02
#include "gen.h"

#define _CPUNAME 65ce02
#define _INCLUDE_65CE02
#include "gen.h"

#define _CPUNAME 65c816
#undef _INCLUDE_65CE02
#define _INCLUDE_65C816
#include "gen.h"

