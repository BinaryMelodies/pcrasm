
#include "isa.h"

enum instruction_type
{
	INS_0,
	INS_4040,
	INS_4,
	INS_R,
	INS_P,
	INS_12,
	INS_R8,
	INS_P8,
	INS_C8,
};

static enum instruction_type i4_patterns[] =
{
	[MNEM_HLT] = INS_4040,
	[MNEM_BBS] = INS_4040,
	[MNEM_LCR] = INS_4040,
	[MNEM_OR4] = INS_4040,
	[MNEM_OR5] = INS_4040,
	[MNEM_AN6] = INS_4040,
	[MNEM_AN7] = INS_4040,
	[MNEM_DB0] = INS_4040,
	[MNEM_DB1] = INS_4040,
	[MNEM_SB0] = INS_4040,
	[MNEM_SB1] = INS_4040,
	[MNEM_EIN] = INS_4040,
	[MNEM_DIN] = INS_4040,
	[MNEM_RPM] = INS_4040,
	[MNEM_JCN] = INS_C8,
	[MNEM_FIM] = INS_P8,
	[MNEM_SRC] = INS_P,
	[MNEM_FIN] = INS_P,
	[MNEM_JIN] = INS_P,
	[MNEM_JUN] = INS_12,
	[MNEM_JMS] = INS_12,
	[MNEM_INC] = INS_R,
	[MNEM_ISZ] = INS_R8,
	[MNEM_ADD] = INS_R,
	[MNEM_SUB] = INS_R,
	[MNEM_LD]  = INS_R,
	[MNEM_XCH] = INS_R,
	[MNEM_BBL] = INS_4,
	[MNEM_LDM] = INS_4,
};

size_t i4_instruction_compute_length(instruction_t * ins, bool forgiving)
{
	if(ins->mnemonic < 0 || ins->mnemonic > sizeof i4_patterns / sizeof i4_patterns[0])
		return 0;
	switch(i4_patterns[ins->mnemonic])
	{
	case INS_0:
	case INS_4:
	case INS_R:
	case INS_P:
		return 1;
	case INS_4040:
		return ins->cpu == CPU_4040 ? 1 : 0;
	case INS_12:
	case INS_R8:
	case INS_P8:
	case INS_C8:
		return 2;
	}
	return 0;
}

void i4_generate_instruction(instruction_t * ins)
{
	reference_t ref[1];

	if(ins->mnemonic < 0 || ins->mnemonic > sizeof i4_patterns / sizeof i4_patterns[0])
		return;

	switch(i4_patterns[ins->mnemonic])
	{
	case INS_0:
	case INS_4040:
		output_byte(ins->mnemonic);
		break;
	case INS_4:
	case INS_R:
		evaluate_expression(ins->operand[0].parameter, ref, ins->code_offset);
		output_byte(ins->mnemonic + (uint_get(ref->value) & 0x0F));
		int_clear(ref->value);
		break;
	case INS_P:
		evaluate_expression(ins->operand[0].parameter, ref, ins->code_offset);
		output_byte(ins->mnemonic + (uint_get(ref->value) & 0x0E));
		int_clear(ref->value);
		break;
	case INS_12:
		evaluate_expression(ins->operand[0].parameter, ref, ins->code_offset);
		output_byte(ins->mnemonic + ((uint_get(ref->value) >> 8) & 0x0F));
		output_byte(uint_get(ref->value));
		int_clear(ref->value);
		break;
	case INS_R8:
	case INS_C8:
		evaluate_expression(ins->operand[0].parameter, ref, ins->code_offset);
		output_byte(ins->mnemonic + (uint_get(ref->value) & 0x0F));
		int_clear(ref->value);

		evaluate_expression(ins->operand[1].parameter, ref, ins->code_offset);
		output_byte(uint_get(ref->value));
		int_clear(ref->value);
		break;

	case INS_P8:
		evaluate_expression(ins->operand[0].parameter, ref, ins->code_offset);
		output_byte(ins->mnemonic + (uint_get(ref->value) & 0x0E));
		int_clear(ref->value);

		evaluate_expression(ins->operand[1].parameter, ref, ins->code_offset);
		output_byte(uint_get(ref->value));
		int_clear(ref->value);
		break;
	}
}

