
#include "../asm.h"
#include "../x86/isa.h"

typedef enum constraint_t
{
	CST_ANY,
	CST_I3,
	CST_I8,
	CST_I16,
	CST_I32,
	CST_WIDTH,
	CST_REL8,
	CST_REL16,
	CST_R,
	CST_P,
	CST_M,
} constraint_t;

typedef enum immediate_type_t
{
	IMMTYPE_NONE,
	IMMTYPE_BYTE,
	IMMTYPE_RELBYTE,
	IMMTYPE_WORD,
	IMMTYPE_RELWORD,
	IMMTYPE_FARWORD,
	IMMTYPE_IMM_DISP, // TSL

	// hack to enable WID instruction
	IMMTYPE_WID,

	// hack to enable double length instructions
	IMMTYPE_MOV_SRC,
	IMMTYPE_MOV_DST,
} immediate_type_t;

struct instruction_pattern_t
{
	size_t opdcount;
	constraint_t constraint[MAX_OPD_COUNT + 1];
	int memopd, regopd;
	immediate_type_t immtype;
	int immopd;
	uint16_t opcode;
};

static match_result_t instruction_pattern_match(const instruction_pattern_t * pattern, instruction_t * ins, bool forgiving)
{
	match_result_t result = { .type = MATCH_PERFECT };

	if(pattern->opdcount != ins->operand_count)
	{
		result.type = MATCH_FAILED;
		return result;
	}

	if(pattern->immtype == IMMTYPE_MOV_DST)
	{
		result.type = MATCH_FAILED;
		return result;
	}

	for(size_t operand_index = 0; operand_index < ins->operand_count; operand_index++)
	{
		reference_t ref[1];
		switch(pattern->constraint[operand_index])
		{
		case CST_ANY:
			break;
		case CST_I3:
			if(ins->operand[operand_index].type != OPD_IMM)
			{
				result.type = MATCH_FAILED;
				return result;
			}
			if(forgiving)
				break;
			evaluate_expression(ins->operand[operand_index].parameter, ref, ins->code_offset);
			if(!is_scalar(ref) || !uint_fits(ref->value) || uint_get(ref->value) > 7)
				result.type = MATCH_TRUNCATED;
			int_clear(ref->value);
			break;
		case CST_I8:
			if(ins->operand[operand_index].type != OPD_IMM)
			{
				result.type = MATCH_FAILED;
				return result;
			}
			/*if(forgiving)
				break;
			evaluate_expression(ins->operand[operand_index].parameter, ref, ins->code_offset);
			if(!is_scalar(ref) || integer_get_size(ref->value) > 1)
				result.type = MATCH_TRUNCATED;
			int_clear(ref->value);*/
			break;
		case CST_REL8:
			if(ins->operand[operand_index].type != OPD_IMM)
			{
				result.type = MATCH_FAILED;
				return result;
			}
			if(forgiving)
				break;
			evaluate_expression(ins->operand[operand_index].parameter, ref, ins->code_offset);
			uint_sub_ui(ref->value, ins->following->code_offset);

			current_section = ins->containing_section;
			if(!is_self_relative(ref))
				result.type = MATCH_TRUNCATED;
			else if(integer_get_size(ref->value) > 1)
				result.type = MATCH_FAILED;

			int_clear(ref->value);
			break;
		case CST_I16:
			if(ins->operand[operand_index].type != OPD_IMM)
			{
				result.type = MATCH_FAILED;
				return result;
			}
			/*if(forgiving)
				break;
			evaluate_expression(ins->operand[operand_index].parameter, ref, ins->code_offset);
			uint_sub_ui(ref->value, ins->following->code_offset);
			current_section = ins->containing_section;
			if(!is_self_relative(ref) || integer_get_size(ref->value) > 1)
				result.type = MATCH_FAILED;
			int_clear(ref->value);*/
			break;
		case CST_I32:
			if(ins->operand[operand_index].type != OPD_IMM && ins->operand[operand_index].type != OPD_FARIMM)
			{
				result.type = MATCH_FAILED;
				return result;
			}
			break;
		case CST_WIDTH:
			if(ins->operand[operand_index].type != OPD_IMM)
			{
				result.type = MATCH_FAILED;
				return result;
			}
			if(forgiving)
				break;
			evaluate_expression(ins->operand[operand_index].parameter, ref, ins->code_offset);
			if(!is_scalar(ref) || !uint_fits(ref->value))
			{
				result.type = MATCH_FAILED;
				return result;
			}
			{
				unsigned width = uint_get(ref->value);
				if(width != 8 && width != 16)
				{
					result.type = MATCH_FAILED;
					return result;
				}
			}
			int_clear(ref->value);
			break;
		case CST_REL16:
			if(ins->operand[operand_index].type != OPD_IMM)
			{
				result.type = MATCH_FAILED;
				return result;
			}
			if(forgiving)
				break;
			evaluate_expression(ins->operand[operand_index].parameter, ref, ins->code_offset);
			uint_sub_ui(ref->value, ins->following->code_offset);
			if(!is_scalar(ref) || integer_get_size(ref->value) > 2) // TODO: check self_relative instead
				result.type = MATCH_TRUNCATED;
			int_clear(ref->value);
			break;
		case CST_R:
			if(ins->operand[operand_index].type != OPD_GPRW)
			{
				result.type = MATCH_FAILED;
				return result;
			}
			break;
		case CST_P:
			if(ins->operand[operand_index].type != OPD_GPRW)
			{
				result.type = MATCH_FAILED;
				return result;
			}
			if((int)ins->operand[operand_index].base != X89_GA
			&& (int)ins->operand[operand_index].base != X89_GB
			&& (int)ins->operand[operand_index].base != X89_GC
			&& (int)ins->operand[operand_index].base != X89_TP)
			{
				result.type = MATCH_FAILED;
				return result;
			}
			break;
		case CST_M:
			if(ins->operand[operand_index].type != OPD_MEM)
			{
				result.type = MATCH_FAILED;
				return result;
			}
			break;
		}
	}

	return result;
}

static size_t instruction_pattern_get_length(const instruction_pattern_t * pattern, instruction_t * ins)
{
	size_t length = 0;

restart:
	length += 2;
	if(pattern->memopd != -1 && ins->operand[pattern->memopd].x89_mode == X89_OFFSET)
	{
		length ++;
	}
	if(pattern->immopd != -1)
	{
		switch(pattern->immtype)
		{
		case IMMTYPE_NONE:
		case IMMTYPE_WID:
		case IMMTYPE_MOV_SRC:
		case IMMTYPE_MOV_DST:
			// this should not occur
			break;
		case IMMTYPE_BYTE:
		case IMMTYPE_RELBYTE:
			length += 1;
			break;
		case IMMTYPE_WORD:
		case IMMTYPE_RELWORD:
		case IMMTYPE_IMM_DISP:
			length += 2;
			break;
		case IMMTYPE_FARWORD:
			length += 4;
			break;
		}
	}

	if(pattern->immtype == IMMTYPE_MOV_SRC)
	{
		pattern++;
		goto restart;
	}

	return length;
}

static void instruction_pattern_generate(const instruction_pattern_t * pattern, instruction_t * ins)
{
	reference_t ref[1];
	uint16_t opcode;
restart:
	opcode = pattern->opcode;
	if(pattern->memopd != -1)
	{
		opcode |= ins->operand[pattern->memopd].x89_mode << 1;
		opcode |= ins->operand[pattern->memopd].base << 8;
	}
	switch(pattern->immtype)
	{
	case IMMTYPE_NONE:
	case IMMTYPE_MOV_SRC:
	case IMMTYPE_MOV_DST:
		break;
	case IMMTYPE_BYTE:
	case IMMTYPE_RELBYTE:
		opcode |= 0x08;
		break;
	case IMMTYPE_WORD:
	case IMMTYPE_RELWORD:
		opcode |= 0x10;
		break;
	case IMMTYPE_FARWORD:
		opcode |= 0x10;
		break;
	case IMMTYPE_IMM_DISP:
		opcode |= 0x18;
		break;
	case IMMTYPE_WID:
		{
			unsigned width;
			evaluate_expression(ins->operand[0].parameter, ref, ins->code_offset);
			width = uint_get(ref->value);
			int_clear(ref->value);
			if(width == 16)
				opcode |= 0x0040;

			evaluate_expression(ins->operand[1].parameter, ref, ins->code_offset);
			width = uint_get(ref->value);
			int_clear(ref->value);
			if(width == 16)
				opcode |= 0x0020;
		}
		break;
	}
	if(pattern->regopd != -1)
	{
		if(ins->operand[pattern->regopd].type == OPD_IMM)
		{
			evaluate_expression(ins->operand[pattern->regopd].parameter, ref, ins->code_offset);
			opcode |= uint_get(ref->value) << 5;
			int_clear(ref->value);
		}
		else
		{
			opcode |= ins->operand[pattern->regopd].base << 5;
		}
	}
	output_byte(opcode);
	output_byte(opcode >> 8);
	if(pattern->memopd != -1 && ins->operand[pattern->memopd].x89_mode == X89_OFFSET)
	{
		evaluate_expression(ins->operand[pattern->memopd].parameter, ref, ins->code_offset);
		output_word(ref, DATA_LE, 1);
		int_clear(ref->value);
	}
	if(pattern->immopd != -1)
	{
		evaluate_expression(ins->operand[pattern->immopd].parameter, ref, ins->code_offset);
		switch(pattern->immtype)
		{
		case IMMTYPE_NONE:
		case IMMTYPE_WID:
		case IMMTYPE_MOV_SRC:
		case IMMTYPE_MOV_DST:
			// this should not occur
			break;
		case IMMTYPE_BYTE:
			output_word(ref, DATA_LE, 1);
			break;
		case IMMTYPE_RELBYTE:
			uint_sub_ui(ref->value, ins->following->code_offset);
			ref->wrt_section = WRT_NONE;
			output_word_pcrel(ref, DATA_LE, 1);
			break;
		case IMMTYPE_WORD:
			output_word(ref, DATA_LE, 2);
			break;
		case IMMTYPE_RELWORD:
			uint_sub_ui(ref->value, ins->following->code_offset);
			ref->wrt_section = WRT_NONE;
			output_word_pcrel(ref, DATA_LE, 2);
			break;
		case IMMTYPE_FARWORD:
			output_word(ref, DATA_LE, 2);
			int_clear(ref->value);
			if(ins->operand[pattern->immopd].type == OPD_IMM)
			{
				if(ref->var.type == VAR_NONE)
				{
					int_init(ref->value);
					ref->var.type = VAR_SECTION;
					ref->var.segment_of = true;
					ref->var.internal.section_index = ins->containing_section;
					output_word(ref, DATA_LE, 2);
				}
				else
				{
					int_init(ref->value);
					ref->var.segment_of = true;
					output_word(ref, DATA_LE, 2);
				}
			}
			else if(ins->operand[pattern->immopd].type == OPD_FARIMM)
			{
				evaluate_expression(ins->operand[pattern->immopd].segment_value, ref, ins->code_offset);
				output_word(ref, DATA_LE, 2);
			}
			break;
		case IMMTYPE_IMM_DISP:
			output_word(ref, DATA_LE, 1);
			int_clear(ref->value);
			evaluate_expression(ins->operand[pattern->immopd + 1].parameter, ref, ins->code_offset);
			uint_sub_ui(ref->value, ins->following->code_offset);
			ref->wrt_section = WRT_NONE;
			output_word_pcrel(ref, DATA_LE, 1);
			break;
		}
		int_clear(ref->value);
	}

	if(pattern->immtype == IMMTYPE_MOV_SRC)
	{
		pattern++;
		goto restart;
	}
}

static const instruction_patterns_t x89_patterns[_MNEM_TOTAL];

static const instruction_pattern_t * find_pattern(instruction_t * ins, bool forgiving)
{
	const struct instruction_patterns_t * patterns = &x89_patterns[ins->mnemonic];
	if(patterns->pattern == NULL)
		return NULL;

	size_t perfect_match_length = (size_t)-1;
	size_t last_match = (size_t)-1;
	for(size_t pattern_index = 0; pattern_index < patterns->count; pattern_index++)
	{
		match_result_t match = instruction_pattern_match(&patterns->pattern[pattern_index], ins, forgiving);
		if(match.type == MATCH_PERFECT)
		{
			// find shortest match ("tightest")
			size_t new_length = instruction_pattern_get_length(&patterns->pattern[pattern_index], ins);
			if(new_length < perfect_match_length)
			{
				perfect_match_length = new_length;
				last_match = pattern_index;
			}
		}
		else if(match.type != MATCH_FAILED)
		{
			if(perfect_match_length == (size_t)-1)
			{
				last_match = pattern_index;
			}
			// TODO: which match should be the preferred one?
		}
	}

	if(last_match == (size_t)-1)
	{
		return NULL;
	}
	else
	{
		return &patterns->pattern[last_match];
	}
}

void x89_generate_instruction(instruction_t * ins)
{
	const instruction_pattern_t * pattern = find_pattern(ins, false);

if(!pattern) { fprintf(stderr, "fail\n"); return; }

	instruction_pattern_generate(pattern, ins);
}

size_t x89_instruction_compute_length(instruction_t * ins, bool forgiving)
{
	const instruction_pattern_t * pattern = find_pattern(ins, forgiving);

if(!pattern) { fprintf(stderr, "fail\n"); return -1; }

	return instruction_pattern_get_length(pattern, ins);
}

#define OPDS_NONE       0, {                          }, -1, -1, IMMTYPE_NONE,     -1
#define OPDS_R          1, { CST_R                    }, -1,  0, IMMTYPE_NONE,     -1
#define OPDS_M          1, { CST_M                    },  0, -1, IMMTYPE_NONE,     -1
#define OPDS_REL8       1, { CST_REL8                 }, -1, -1, IMMTYPE_RELBYTE,   0
#define OPDS_REL16      1, { CST_REL16                }, -1, -1, IMMTYPE_RELWORD,   0
#define OPDS_R_M        2, { CST_R, CST_M             },  1,  0, IMMTYPE_NONE,     -1
#define OPDS_P_M        2, { CST_P, CST_M             },  1,  0, IMMTYPE_NONE,     -1
#define OPDS_P_I32      2, { CST_P, CST_I32           }, -1,  0, IMMTYPE_FARWORD,   1
#define OPDS_M_R        2, { CST_M, CST_R             },  0,  1, IMMTYPE_NONE,     -1
#define OPDS_M_P        2, { CST_M, CST_P             },  0,  1, IMMTYPE_NONE,     -1
#define OPDS_R_I8       2, { CST_R, CST_I8            }, -1,  0, IMMTYPE_BYTE,      1
#define OPDS_M_I8       2, { CST_M, CST_I8            },  0, -1, IMMTYPE_BYTE,      1
#define OPDS_R_I16      2, { CST_R, CST_I16           }, -1,  0, IMMTYPE_WORD,      1
#define OPDS_M_I16      2, { CST_M, CST_I16           },  0, -1, IMMTYPE_WORD,      1
#define OPDS_M_I3       2, { CST_M, CST_I3            },  0,  1, IMMTYPE_NONE,     -1
#define OPDS_R_REL8     2, { CST_R, CST_REL8          }, -1,  0, IMMTYPE_RELBYTE,   1
#define OPDS_R_REL16    2, { CST_R, CST_REL16         }, -1,  0, IMMTYPE_RELWORD,   1
#define OPDS_M_REL8     2, { CST_M, CST_REL8          },  0, -1, IMMTYPE_RELBYTE,   1
#define OPDS_M_REL16    2, { CST_M, CST_REL16         },  0, -1, IMMTYPE_RELWORD,   1
#define OPDS_M_I3_REL8  3, { CST_M, CST_I3, CST_REL8  },  0,  1, IMMTYPE_RELBYTE,   2
#define OPDS_M_I3_REL16 3, { CST_M, CST_I3, CST_REL16 },  0,  1, IMMTYPE_RELWORD,   2
#define OPDS_M_I8_REL8  3, { CST_M, CST_I8, CST_REL8  },  0, -1, IMMTYPE_IMM_DISP,  1
#define OPDS_WID        2, { CST_WIDTH, CST_WIDTH     }, -1, -1, IMMTYPE_WID,      -1 // hack to enable WID instruction
#define OPDS_M_M        2, { CST_M, CST_M             },  1, -1, IMMTYPE_MOV_SRC,  -1
#define OPDS_M_M_2      2, { CST_M, CST_M             },  0, -1, IMMTYPE_MOV_DST,  -1 // hack to enable double instructions

static instruction_pattern_t pattern_add[] =
{
	{
		OPDS_R_M,
		0xA001,
	},
	{
		OPDS_M_R,
		0xD001,
	},
};

static instruction_pattern_t pattern_addb[] =
{
	{
		OPDS_R_M,
		0xA000,
	},
	{
		OPDS_M_R,
		0xD000,
	},
};

static instruction_pattern_t pattern_addbi[] =
{
	{
		OPDS_R_I8,
		0x2000,
	},
	{
		OPDS_M_I8,
		0xC000,
	},
};

static instruction_pattern_t pattern_addi[] =
{
	{
		OPDS_R_I16,
		0x2001,
	},
	{
		OPDS_M_I16,
		0xC001,
	},
};

static instruction_pattern_t pattern_and[] =
{
	{
		OPDS_R_M,
		0xA801,
	},
	{
		OPDS_M_R,
		0xD801,
	},
};

static instruction_pattern_t pattern_andb[] =
{
	{
		OPDS_R_M,
		0xA800,
	},
	{
		OPDS_M_R,
		0xD800,
	},
};

static instruction_pattern_t pattern_andbi[] =
{
	{
		OPDS_R_I8,
		0x2800,
	},
	{
		OPDS_M_I8,
		0xC800,
	},
};

static instruction_pattern_t pattern_andi[] =
{
	{
		OPDS_R_I16,
		0x2801,
	},
	{
		OPDS_M_I16,
		0xC801,
	},
};

static instruction_pattern_t pattern_call[] =
{
	{
		OPDS_M_REL8,
		0x9C81,
	},
	{
		OPDS_M_REL16,
		0x9C81,
	},
};

static instruction_pattern_t pattern_lcall[] =
{
	{
		OPDS_M_REL16,
		0x9C81,
	},
};

static instruction_pattern_t pattern_clr[] =
{
	{
		OPDS_M_I3,
		0xF800,
	},
};

static instruction_pattern_t pattern_dec[] =
{
	{
		OPDS_R,
		0x3C00,
	},
	{
		OPDS_M,
		0xEC01,
	},
};

static instruction_pattern_t pattern_decb[] =
{
	{
		OPDS_M,
		0xEC00,
	},
};

static instruction_pattern_t pattern_hlt[] =
{
	{
		OPDS_NONE,
		0x4820,
	},
};

static instruction_pattern_t pattern_inc[] =
{
	{
		OPDS_R,
		0x3800,
	},
	{
		OPDS_M,
		0xE801,
	},
};

static instruction_pattern_t pattern_incb[] =
{
	{
		OPDS_M,
		0xE800,
	},
};

static instruction_pattern_t pattern_jbt[] =
{
	{
		OPDS_M_I3_REL8,
		0xBC00,
	},
	{
		OPDS_M_I3_REL16,
		0xBC00,
	},
};

static instruction_pattern_t pattern_ljbt[] =
{
	{
		OPDS_M_I3_REL16,
		0xBC00,
	},
};

static instruction_pattern_t pattern_jmce[] =
{
	{
		OPDS_M_REL8,
		0xB000,
	},
	{
		OPDS_M_I3_REL16,
		0xB000,
	},
};

static instruction_pattern_t pattern_ljmce[] =
{
	{
		OPDS_M_I3_REL16,
		0xB000,
	},
};

static instruction_pattern_t pattern_jmcne[] =
{
	{
		OPDS_M_REL8,
		0xB400,
	},
	{
		OPDS_M_I3_REL16,
		0xB400,
	},
};

static instruction_pattern_t pattern_ljmcne[] =
{
	{
		OPDS_M_I3_REL16,
		0xB400,
	},
};

// this is actually an ADD(B)I instruction
static instruction_pattern_t pattern_jmp[] =
{
	{
		OPDS_REL8,
		0x2080,
	},
	{
		OPDS_REL16,
		0x2081,
	},
};

// this is actually an ADDI instruction
static instruction_pattern_t pattern_ljmp[] =
{
	{
		OPDS_REL16,
		0x2081,
	},
};

static instruction_pattern_t pattern_jnbt[] =
{
	{
		OPDS_M_I3_REL8,
		0xB800,
	},
	{
		OPDS_M_I3_REL16,
		0xB800,
	},
};

static instruction_pattern_t pattern_ljnbt[] =
{
	{
		OPDS_M_I3_REL16,
		0xB800,
	},
};

static instruction_pattern_t pattern_jnz[] =
{
	{
		OPDS_R_REL8,
		0x4000,
	},
	{
		OPDS_R_REL16,
		0x4000,
	},
	{
		OPDS_M_REL8,
		0xE001,
	},
	{
		OPDS_M_REL16,
		0xE001,
	},
};

static instruction_pattern_t pattern_ljnz[] =
{
	{
		OPDS_R_REL16,
		0x4000,
	},
	{
		OPDS_M_REL16,
		0xE001,
	},
};

static instruction_pattern_t pattern_jnzb[] =
{
	{
		OPDS_M_REL8,
		0xE000,
	},
	{
		OPDS_M_REL16,
		0xE000,
	},
};

static instruction_pattern_t pattern_ljnzb[] =
{
	{
		OPDS_M_REL16,
		0xE000,
	},
};

static instruction_pattern_t pattern_jz[] =
{
	{
		OPDS_R_REL8,
		0x4400,
	},
	{
		OPDS_R_REL16,
		0x4400,
	},
	{
		OPDS_M_REL8,
		0xE401,
	},
	{
		OPDS_M_REL16,
		0xE401,
	},
};

static instruction_pattern_t pattern_ljz[] =
{
	{
		OPDS_R_REL16,
		0x4400,
	},
	{
		OPDS_M_REL16,
		0xE401,
	},
};

static instruction_pattern_t pattern_jzb[] =
{
	{
		OPDS_M_REL8,
		0xE400,
	},
	{
		OPDS_M_REL16,
		0xE400,
	},
};

static instruction_pattern_t pattern_ljzb[] =
{
	{
		OPDS_M_REL16,
		0xE400,
	},
};

static instruction_pattern_t pattern_lpd[] =
{
	{
		OPDS_P_M,
		0x8801,
	},
};

static instruction_pattern_t pattern_lpdi[] =
{
	{
		OPDS_P_I32,
		0x0801,
	},
};

static instruction_pattern_t pattern_mov[] =
{
	{
		OPDS_M_R,
		0x8401,
	},
	{
		OPDS_R_M,
		0x8001,
	},
	{
		OPDS_M_M,
		0x9001,
	},
	{
		OPDS_M_M_2, // this will not be matched, but has to proceed the previous pattern
		0xCC01,
	},
};

static instruction_pattern_t pattern_movb[] =
{
	{
		OPDS_M_R,
		0x8400,
	},
	{
		OPDS_R_M,
		0x8000,
	},
	{
		OPDS_M_M,
		0x9000,
	},
	{
		OPDS_M_M_2, // this will not be matched, but has to proceed the previous pattern
		0xCC00,
	},
};

static instruction_pattern_t pattern_movbi[] =
{
	{
		OPDS_R_I8,
		0x3000,
	},
	{
		OPDS_M_I8,
		0x4C00,
	},
};

static instruction_pattern_t pattern_movi[] =
{
	{
		OPDS_R_I16,
		0x3001,
	},
	{
		OPDS_M_I16,
		0x4C01,
	},
};

static instruction_pattern_t pattern_movp[] =
{
	{
		OPDS_M_P,
		0x9801,
	},
	{
		OPDS_P_M,
		0x8C01,
	},
};

static instruction_pattern_t pattern_nop[] =
{
	{
		OPDS_NONE,
		0x0000,
	},
};

static instruction_pattern_t pattern_not[] =
{
	{
		OPDS_R,
		0x2C00,
	},
	{
		OPDS_M,
		0xDC01,
	},
	{
		OPDS_R_M,
		0xAC01,
	},
};

static instruction_pattern_t pattern_notb[] =
{
	{
		OPDS_M,
		0xDC00,
	},
	{
		OPDS_R_M,
		0xAC00,
	},
};

static instruction_pattern_t pattern_or[] =
{
	{
		OPDS_R_M,
		0xA401,
	},
	{
		OPDS_M_R,
		0xD401,
	},
};

static instruction_pattern_t pattern_orb[] =
{
	{
		OPDS_R_M,
		0xA400,
	},
	{
		OPDS_M_R,
		0xD400,
	},
};

static instruction_pattern_t pattern_orbi[] =
{
	{
		OPDS_R_I8,
		0x2400,
	},
	{
		OPDS_M_I8,
		0xC400,
	},
};

static instruction_pattern_t pattern_ori[] =
{
	{
		OPDS_R_I16,
		0x2401,
	},
	{
		OPDS_M_I16,
		0xC401,
	},
};

static instruction_pattern_t pattern_setb[] =
{
	{
		OPDS_M_I3,
		0xF400,
	},
};

static instruction_pattern_t pattern_sintr[] =
{
	{
		OPDS_NONE,
		0x0040,
	},
};

static instruction_pattern_t pattern_tsl[] =
{
	{
		OPDS_M_I8_REL8,
		0x9400,
	},
};

static instruction_pattern_t pattern_wid[] =
{
	{
		OPDS_WID,
		0x0080,
	},
};

static instruction_pattern_t pattern_xfer[] =
{
	{
		OPDS_NONE,
		0x0060,
	},
};

#define PATTERN(__pattern) { (__pattern), sizeof(__pattern) / sizeof((__pattern)[0]) }

static const instruction_patterns_t x89_patterns[_MNEM_TOTAL] =
{
	[MNEM_I8089_ADD] = PATTERN(pattern_add),
	[MNEM_I8089_ADDB] = PATTERN(pattern_addb),
	[MNEM_I8089_ADDBI] = PATTERN(pattern_addbi),
	[MNEM_I8089_ADDI] = PATTERN(pattern_addi),
	[MNEM_I8089_AND] = PATTERN(pattern_and),
	[MNEM_I8089_ANDB] = PATTERN(pattern_andb),
	[MNEM_I8089_ANDBI] = PATTERN(pattern_andbi),
	[MNEM_I8089_ANDI] = PATTERN(pattern_andi),
	[MNEM_I8089_CALL] = PATTERN(pattern_call),
	[MNEM_I8089_CLR] = PATTERN(pattern_clr),
	[MNEM_I8089_DEC] = PATTERN(pattern_dec),
	[MNEM_I8089_DECB] = PATTERN(pattern_decb),
	[MNEM_I8089_HLT] = PATTERN(pattern_hlt),
	[MNEM_I8089_INC] = PATTERN(pattern_inc),
	[MNEM_I8089_INCB] = PATTERN(pattern_incb),
	[MNEM_I8089_JBT] = PATTERN(pattern_jbt),
	[MNEM_I8089_JMCE] = PATTERN(pattern_jmce),
	[MNEM_I8089_JMCNE] = PATTERN(pattern_jmcne),
	[MNEM_I8089_JMP] = PATTERN(pattern_jmp),
	[MNEM_I8089_JNBT] = PATTERN(pattern_jnbt),
	[MNEM_I8089_JNZ] = PATTERN(pattern_jnz),
	[MNEM_I8089_JNZB] = PATTERN(pattern_jnzb),
	[MNEM_I8089_JZ] = PATTERN(pattern_jz),
	[MNEM_I8089_JZB] = PATTERN(pattern_jzb),
	[MNEM_I8089_LCALL] = PATTERN(pattern_lcall),
	[MNEM_I8089_LJBT] = PATTERN(pattern_ljbt),
	[MNEM_I8089_LJMCE] = PATTERN(pattern_ljmce),
	[MNEM_I8089_LJMCNE] = PATTERN(pattern_ljmcne),
	[MNEM_I8089_LJMP] = PATTERN(pattern_ljmp),
	[MNEM_I8089_LJNBT] = PATTERN(pattern_ljnbt),
	[MNEM_I8089_LJNZ] = PATTERN(pattern_ljnz),
	[MNEM_I8089_LJNZB] = PATTERN(pattern_ljnzb),
	[MNEM_I8089_LJZ] = PATTERN(pattern_ljz),
	[MNEM_I8089_LJZB] = PATTERN(pattern_ljzb),
	[MNEM_I8089_LPD] = PATTERN(pattern_lpd),
	[MNEM_I8089_LPDI] = PATTERN(pattern_lpdi),
	[MNEM_I8089_MOV] = PATTERN(pattern_mov),
	[MNEM_I8089_MOVB] = PATTERN(pattern_movb),
	[MNEM_I8089_MOVBI] = PATTERN(pattern_movbi),
	[MNEM_I8089_MOVI] = PATTERN(pattern_movi),
	[MNEM_I8089_MOVP] = PATTERN(pattern_movp),
	[MNEM_I8089_NOP] = PATTERN(pattern_nop),
	[MNEM_I8089_NOT] = PATTERN(pattern_not),
	[MNEM_I8089_NOTB] = PATTERN(pattern_notb),
	[MNEM_I8089_OR] = PATTERN(pattern_or),
	[MNEM_I8089_ORB] = PATTERN(pattern_orb),
	[MNEM_I8089_ORBI] = PATTERN(pattern_orbi),
	[MNEM_I8089_ORI] = PATTERN(pattern_ori),
	[MNEM_I8089_SETB] = PATTERN(pattern_setb),
	[MNEM_I8089_SINTR] = PATTERN(pattern_sintr),
	[MNEM_I8089_TSL] = PATTERN(pattern_tsl),
	[MNEM_I8089_WID] = PATTERN(pattern_wid),
	[MNEM_I8089_XFER] = PATTERN(pattern_xfer),
};

