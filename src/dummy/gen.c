
#include <assert.h>
#include <stdio.h>
#include "../asm.h"
#include "isa.h"

size_t dummy_instruction_compute_length(instruction_t * ins, bool forgiving)
{
	return 0;
}

void dummy_generate_instruction(instruction_t * ins)
{
}

