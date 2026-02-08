
#include "../asm.h"
#include "isa.h"

#define UNDEF 0x02 // opcode unused by either 6800 or 6809

#include "../../obj/680x/680x/gen.h"

static inline size_t get_opcode_length(int opcode)
{
	if(opcode < 0x100)
		return 1;
	else
		return 2;
}

static uint8_t calculate_actual_index_byte(instruction_t * ins, bool forgiving)
{
	reference_t ref[1];
	uint8_t generated_index_byte = ins->operand[0].index | 0x80;
	switch(generated_index_byte & 0x0F)
	{
	case 0x04:
		/* n,R */
		if(!forgiving)
		{
			evaluate_expression(ins->operand[0].parameter, ref, ins->code_offset);
			if(!is_scalar(ref) || uinteger_get_size(ref->value) > 1)
			{
				generated_index_byte = (generated_index_byte & 0xF0) | 0x09;
			}
			else if(int_is_zero(ref->value))
			{
			}
			else if(0 <= int_cmp_si(ref->value, -0x10) && int_cmp_si(ref->value, 0x0F) <= 0)
			{
				generated_index_byte = (generated_index_byte & 0x60) | (int_get(ref->value) & 0x1F);
			}
			else
			{
				generated_index_byte = (generated_index_byte & 0xF0) | 0x08;
			}
			int_clear(ref->value);
		}
		break;
	case 0x0C:
		/* n,PCR */
		if(!forgiving)
		{
			evaluate_expression(ins->operand[0].parameter, ref, ins->code_offset);
			uint_sub_ui(ref->value, ins->following->code_offset);
			if(!is_scalar(ref) || uinteger_get_size(ref->value) > 1)
			{
				generated_index_byte = generated_index_byte | 1;
			}
			else if(0 <= int_cmp_si(ref->value, -0x10) && int_cmp_si(ref->value, 0x0F) <= 0)
			{
			}
			else
			{
				generated_index_byte = generated_index_byte | 1;
			}
			int_clear(ref->value);
		}
		break;
	default:
		break;
	}
	return generated_index_byte;
}

static uint8_t get_index_byte_displacement_size(instruction_t * ins, uint8_t generated_index_byte)
{
	switch(generated_index_byte & 0x8F)
	{
	case 0x88:
	case 0x8C:
		return 1;
	case 0x89:
	case 0x8D:
	case 0x8F:
		return 2;
	default:
		return 0;
	}
}

static bool is_index_byte_displacement_relative(instruction_t * ins, uint8_t generated_index_byte)
{
	switch(generated_index_byte & 0x8F)
	{
	case 0x8C:
	case 0x8D:
		return true;
	default:
		return false;
	}
}

static inline size_t get_instruction_length(instruction_t * ins, int opcode, int operand_type, bool forgiving)
{
	size_t length = get_opcode_length(opcode);
	switch(operand_type)
	{
	case OPD_NONE:
		break;
	case OPD_IMMB:
		length += 1;
		break;
	case OPD_IMMW:
		length += 2;
		break;
	case OPD_DIR:
		length += 1;
		break;
	case OPD_IND:
		if(ins->cpu == CPU_6809)
		{
			uint8_t generated_index_byte = calculate_actual_index_byte(ins, forgiving);
			size_t dispsize = get_index_byte_displacement_size(ins, generated_index_byte);
			length += 1 + dispsize;
		}
		else
		{
			length += 1;
		}
		break;
	case OPD_EXT:
		length += 2;
		break;
	case OPD_RELB:
		length += 1;
		break;
	case OPD_RELW:
		length += 2;
		break;
	case OPD_REG2:
	case OPD_REGLIST:
		length += 1;
		break;
	}
	return length;
}

operand_type_t match_mode(instruction_t * ins, const unsigned * patterns, bool forgiving)
{
	reference_t ref[1];
	operand_type_t generated_type = ins->operand[0].type;
	switch(generated_type)
	{
	case OPD_IMMB:
		if(patterns[OPD_IMMB] == UNDEF)
			generated_type = OPD_IMMW;
		else if(!forgiving && patterns[OPD_IMMW] != UNDEF)
		{
			evaluate_expression(ins->operand[0].parameter, ref, ins->code_offset);
			if(!is_scalar(ref) || uinteger_get_size(ref->value) > 1)
				generated_type = OPD_IMMW;
			int_clear(ref->value);
		}
		break;
	case OPD_RELB:
		if(patterns[OPD_RELB] == UNDEF)
			generated_type = OPD_RELW;
		else if(!forgiving && patterns[OPD_RELW] != UNDEF)
		{
			evaluate_expression(ins->operand[0].parameter, ref, ins->code_offset);
			if(!is_scalar(ref)) // TODO
			{
				generated_type = OPD_RELW;
			}
			else
			{
				uint_sub_ui(ref->value, get_opcode_length(patterns[OPD_RELB]) + 1);
				if(integer_get_size(ref->value) > 1)
					generated_type = OPD_RELW;
			}
			int_clear(ref->value);
		}
		break;
	case OPD_DIR:
		if(patterns[OPD_DIR] == UNDEF)
			generated_type = OPD_EXT;
		else if(ins->cpu == CPU_6800 && !forgiving && patterns[OPD_EXT] != UNDEF)
		{
			evaluate_expression(ins->operand[0].parameter, ref, ins->code_offset);
			if(!is_scalar(ref) || uinteger_get_size(ref->value) > 1)
				generated_type = OPD_EXT;
			int_clear(ref->value);
		}
		break;
	default:
		break;
	}
	if(patterns[generated_type] == UNDEF)
	{
		fprintf(stderr, "Invalid instruction in line %ld\n", ins->line_number);
		return (operand_type_t)-1;
	}
	return generated_type;
}

size_t m680x_instruction_compute_length(instruction_t * ins, bool forgiving)
{
	const unsigned * patterns = ins->cpu == CPU_6809 ? m6809_patterns[ins->mnemonic] : m6800_patterns[ins->mnemonic];
	operand_type_t operand_type = match_mode(ins, patterns, forgiving);
	return get_instruction_length(ins, patterns[operand_type], operand_type, forgiving);
}

static void generate_instruction_pattern(instruction_t * ins, operand_type_t operand_type, unsigned opcode)
{
	reference_t ref[1];
	switch(get_opcode_length(opcode))
	{
	case 1:
		output_byte(opcode);
		break;
	case 2:
		output_word16be(opcode);
		break;
	}

	size_t dispsize;
	bool pcrel;

	switch(operand_type)
	{
	case OPD_NONE:
		return;

	case OPD_REG2:
	case OPD_REGLIST:
		output_byte(ins->operand[0].index);
		return;

	case OPD_IMMB:
	case OPD_DIR:
		dispsize = 1;
		pcrel = false;
		break;
	case OPD_IMMW:
	case OPD_EXT:
		dispsize = 2;
		pcrel = false;
		break;
	case OPD_IND:
		if(ins->cpu == CPU_6809)
		{
			uint8_t generated_index_byte = calculate_actual_index_byte(ins, false);
			output_byte(generated_index_byte);
			dispsize = get_index_byte_displacement_size(ins, generated_index_byte);
			pcrel = is_index_byte_displacement_relative(ins, generated_index_byte);
		}
		else
		{
			dispsize = 2;
			pcrel = false;
		}
		break;
	case OPD_RELB:
		dispsize = 1;
		pcrel = true;
		break;
	case OPD_RELW:
		dispsize = 2;
		pcrel = true;
		break;
	}

	evaluate_expression(ins->operand[0].parameter, ref, ins->code_offset);
	if(pcrel)
	{
		uint_sub_ui(ref->value, ins->following->code_offset);
	}
	output_word_as(ref, DATA_BE, dispsize, pcrel, 0);
	int_clear(ref->value);
}

void m680x_generate_instruction(instruction_t * ins)
{
	const unsigned * patterns = ins->cpu == CPU_6809 ? m6809_patterns[ins->mnemonic] : m6800_patterns[ins->mnemonic];
	operand_type_t operand_type = match_mode(ins, patterns, false);
	int opcode = patterns[operand_type];
	generate_instruction_pattern(ins, operand_type, opcode);
}

