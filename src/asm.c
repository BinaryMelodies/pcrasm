
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "asm.h"
#include "symbolic.h"
#include "coff.h"
#include "elf.h"
#include "hex.h"
#include "omf.h"
#include "isa.h"

extern int yyparse(void);

void convert_common_attribute_list(definition_t * comdef, expression_t * expression, long here)
{
	for(
		;
		expression != NULL && expression->argument_count != 0;
		expression = expression->argument[1])
	{
		expression_t * attribute = expression->argument[0];
		switch((int)attribute->type)
		{
		case EXP_IDENTIFIER:
			if(strcmp(attribute->value.s, "near") == 0)
			{
				//
			}
			else if(strcmp(attribute->value.s, "far") == 0)
			{
				comdef->isfar = true;
			}
			else
			{
				fprintf(stderr, "Unknown attribute %s\n", attribute->value.s);
			}
			break;
		case EXP_ASSIGN:
			if(strcmp(attribute->argument[0]->value.s, "align") == 0)
			{
				// TODO: always integer
				int_clear(comdef->ref.value);
				evaluate_expression(attribute->argument[1], &comdef->ref, here);
				assert_scalar(&comdef->ref);
			}
			else if(strcmp(attribute->argument[0]->value.s, "count") == 0)
			{
				// TODO: always integer
				int_clear(comdef->count.value);
				evaluate_expression(attribute->argument[1], &comdef->count, here);
				assert_scalar(&comdef->count);
			}
			else
			{
				fprintf(stderr, "Unknown attribute %s\n", attribute->argument[0]->value.s);
			}
			break;
		default:
			fprintf(stderr, "Internal error\n");
			assert(false);
		}
	}
}

void convert_export_attribute_list(definition_t * expdef, expression_t * expression)
{
	for(
		;
		expression != NULL && expression->argument_count != 0;
		expression = expression->argument[1])
	{
		expression_t * attribute = expression->argument[0];
		switch((int)attribute->type)
		{
		case EXP_IDENTIFIER:
			if(strcmp(attribute->value.s, "resident") == 0)
			{
				expdef->exported.attributes |= OMF_EXPDEF_RESIDENT_NAME;
			}
			else if(strcmp(attribute->value.s, "nodata") == 0)
			{
				expdef->exported.attributes |= OMF_EXPDEF_NODATA;
			}
			else
			{
				fprintf(stderr, "Unknown attribute %s\n", attribute->value.s);
			}
			break;
		case EXP_ASSIGN:
			if(strcmp(attribute->argument[0]->value.s, "parm") == 0)
			{
				if(attribute->argument[1]->type != EXP_INTEGER)
				{
					fprintf(stderr, "Fatal error: expected integer parameter count\n");
					break;
				}
				if(!uint_fits(attribute->argument[1]->value.i))
				{
					fprintf(stderr, "Fatal error: alignment too large to fit machine word\n");
				}
				expdef->exported.parameter_count = uint_get(attribute->argument[1]->value.i);
			}
			else
			{
				fprintf(stderr, "Unknown attribute %s\n", attribute->argument[0]->value.s);
			}
			break;
		default:
			fprintf(stderr, "Internal error\n");
			assert(false);
		}
	}
}

objfile_t output;

size_t current_section;

static inline bool output_format_is_textual(output_format_t format)
{
	switch(format)
	{
	case FORMAT_DEBUG:
	case FORMAT_HEX16:
	case FORMAT_HEX32:
		return true;
	default:
		return false;
	}
}

void convert_section_attribute_list(section_t * section, expression_t * expression)
{
	uint64_t flags = 0;
	uint64_t noflags = 0;
	section->align = 0;
	for(
		;
		expression != NULL && expression->argument_count != 0;
		expression = expression->argument[1])
	{
		expression_t * attribute = expression->argument[0];
		switch((int)attribute->type)
		{
		case EXP_IDENTIFIER:
//			printf("%s\n", attribute->value.s);
			if(strcmp(attribute->value.s, "progbits") == 0)
			{
				flags |= SHF_PROGBITS;
				noflags |= SHF_NOBITS;
			}
			else if(strcmp(attribute->value.s, "nobits") == 0)
			{
				flags |= SHF_NOBITS;
				noflags |= SHF_PROGBITS;
			}
			else if(strcmp(attribute->value.s, "read") == 0)
			{
				flags |= SHF_READ;
			}
			else if(strcmp(attribute->value.s, "noread") == 0)
			{
				noflags |= SHF_READ;
			}
			else if(strcmp(attribute->value.s, "write") == 0)
			{
				flags |= SHF_WRITE;
			}
			else if(strcmp(attribute->value.s, "nowrite") == 0)
			{
				noflags |= SHF_WRITE;
			}
			else if(strcmp(attribute->value.s, "exec") == 0)
			{
				flags |= SHF_EXECINSTR;
			}
			else if(strcmp(attribute->value.s, "noexec") == 0)
			{
				noflags |= SHF_EXECINSTR;
			}
			else if(strcmp(attribute->value.s, "alloc") == 0)
			{
				flags |= SHF_ALLOC;
			}
			else if(strcmp(attribute->value.s, "noalloc") == 0)
			{
				noflags |= SHF_ALLOC;
			}
			else if(strcmp(attribute->value.s, "private") == 0)
			{
				flags |= SHF_PRIVATE;
				noflags |= SHF_PUBLIC | SHF_COMMON | SHF_STACK;
			}
			else if(strcmp(attribute->value.s, "public") == 0)
			{
				flags |= SHF_PUBLIC;
				noflags |= SHF_PRIVATE | SHF_COMMON | SHF_STACK;
			}
			else if(strcmp(attribute->value.s, "common") == 0)
			{
				flags |= SHF_COMMON;
				noflags |= SHF_PRIVATE | SHF_PUBLIC | SHF_STACK;
			}
			else if(strcmp(attribute->value.s, "stack") == 0)
			{
				flags |= SHF_STACK;
				noflags |= SHF_PRIVATE | SHF_PUBLIC | SHF_COMMON;
			}
			else if(strcmp(attribute->value.s, "use16") == 0)
			{
				flags |= SHF_USE16;
				noflags |= SHF_USE32;
			}
			else if(strcmp(attribute->value.s, "use32") == 0)
			{
				flags |= SHF_USE32;
				noflags |= SHF_USE16;
			}
			else
			{
				fprintf(stderr, "Unknown attribute %s\n", attribute->value.s);
			}

			if((flags & noflags) != 0)
			{
				fprintf(stderr, "Invalid flag %s\n", attribute->value.s);
			}
			break;
		case EXP_ASSIGN:
			if(strcmp(attribute->argument[0]->value.s, "align") == 0)
			{
				if(attribute->argument[1]->type != EXP_INTEGER)
				{
					fprintf(stderr, "Fatal error: expected integer alignment\n");
					break;
				}
				if(!uint_fits(attribute->argument[1]->value.i))
				{
					fprintf(stderr, "Fatal error: alignment too large to fit machine word\n");
				}
				section->align = uint_get(attribute->argument[1]->value.i);
			}
			else
			{
				fprintf(stderr, "Unknown attribute %s\n", attribute->argument[0]->value.s);
			}
			break;
		default:
			fprintf(stderr, "Internal error\n");
			assert(false);
		}
	}
	section->flags = flags;
}

void set_default_attributes(section_t * section)
{
	if(strcmp(section->name, ".text") == 0)
	{
		if(section->flags == 0)
			section->flags = SHF_PROGBITS | SHF_ALLOC | SHF_EXECINSTR;
		if(section->align == 0)
			section->align = 1;
	}
	else if(strcmp(section->name, ".rodata") == 0)
	{
		if(section->flags == 0)
			section->flags = SHF_PROGBITS | SHF_ALLOC;
		if(section->align == 0)
			section->align = 1;
	}
	else if(strcmp(section->name, ".data") == 0)
	{
		if(section->flags == 0)
			section->flags = SHF_PROGBITS | SHF_ALLOC | SHF_WRITE;
		if(section->align == 0)
			section->align = 0;
	}
	else if(strcmp(section->name, ".bss") == 0)
	{
		if(section->flags == 0)
			section->flags = SHF_NOBITS | SHF_ALLOC | SHF_WRITE;
		if(section->align == 0)
			section->align = 1;
	}
	else
	{
		if(section->flags == 0)
			section->flags = 0;
		if(section->align == 0)
			section->align = 1;
	}
}

block_t * block_create(uint64_t address)
{
	block_t * block = malloc(sizeof(block_t));
	memset(block, 0, sizeof(block_t));
	block->address = address;
	return block;
}

void section_init(section_t * section, char * name, section_format_t format, section_t _default)
{
	section->name = name;
	section->format = format;
	section->flags = _default.flags;
	section->align = _default.align;
	section->segment_section = -1;
	switch(section->format)
	{
	case SECTION_DATA:
	case SECTION_ZERO_DATA:
		section->data.full_size = 0;
		section->data.current_address = 0;
		section->data.first_block = NULL;
		section->data.current_block = NULL;
		section->data.first_instruction = NULL;
		section->data.last_instruction = NULL;
		break;
	case SECTION_RELOC:
		section->reloc.count = 0;
		section->reloc.buffer_size = 0;
		section->reloc.relocations = NULL;
		section->reloc.extra_count = 0;
		break;
	case SECTION_SYMTAB:
		section->symtab.count = 0;
		section->symtab.buffer_size = 0;
		section->symtab.symbols = NULL;
		break;
	case SECTION_STRTAB:
		section->strtab.size = 0;
		section->strtab.buffer_size = 0;
		section->strtab.buffer = NULL;
		break;
	}
}

size_t objfile_new_section(objfile_t * file, const char * name, section_format_t format, section_t _default)
{
	file->section = realloc(file->section, (file->section_count + 1) * sizeof(section_t *));
	section_t * section = malloc(sizeof(section_t));
	file->section[file->section_count] = section;
	section_init(
		section,
		name ? strdup(name) : NULL,
		format,
		_default);

	section_t * relocations = malloc(sizeof(section_t));
	// TODO: add section name and flags later
	char * reloc_section_name = NULL;
	if(name != NULL)
	{
		const char * prefix;
		if(elf_backend_uses_rela())
			prefix = ".rela";
		else
			prefix = ".rel";
		reloc_section_name = malloc(strlen(name) + strlen(prefix) + 1);
		strcpy(reloc_section_name, prefix);
		strcat(reloc_section_name, name);
	}

	_default.flags = SHF_RELOC;
	_default.align = output.format != FORMAT_ELF64 ? 4 : 8; /* TODO: make alignment more format specific */

	section_init(
		relocations,
		reloc_section_name,
		SECTION_RELOC,
		_default);

	file->section[file->section_count]->data.relocations = relocations;

	file->section_count ++;
	return file->section_count - 1;
}

size_t objfile_locate_section(objfile_t * file, const char * name, section_format_t format, section_t _default)
{
	for(size_t section_index = 0; section_index < file->section_count; section_index++)
	{
		section_t * section = file->section[section_index];
		if(strcmp(section->name, name) == 0)
			return section_index;
	}
	if(format == SECTION_FAIL)
	{
		fprintf(stderr, "Section %s not found\n", name);
		exit(1);
	}
	return objfile_new_section(file, name, format, _default);
}

void block_append(block_t * block, size_t count, void * data)
{
	if(block->size + count > block->buffer_size)
	{
		// an arbitrary increment
		// TODO: check alignment
		block->buffer_size = align_to(block->size + count, 16);
		block->buffer = block->buffer != NULL ? realloc(block->buffer, block->buffer_size) : malloc(block->buffer_size);
	}
	memcpy(block->buffer + block->size, data, count);
	block->size += count;
}

size_t section_get_current_offset(section_t * section)
{
	if(section->format == SECTION_ZERO_DATA)
		return section->data.full_size;
	return section->data.current_block->address + section->data.current_block->size;
}

void section_append(section_t * section, size_t count, void * data)
{
	if(section->format == SECTION_ZERO_DATA)
	{
		fprintf(stderr, "Error: writing data into zeroed section\n");
		return;
	}
	if(section->data.current_block == NULL)
	{
		section->data.current_block = section->data.first_block = block_create(section->data.current_address);
	}
	else if(section->data.current_block->address + section->data.current_block->size != section->data.current_address)
	{
		section->data.current_block->next = block_create(section->data.current_address);
		section->data.current_block = section->data.current_block->next;
	}
	block_append(section->data.current_block, count, data);
	if(section->data.current_block->address + section->data.current_block->size
		> section->data.first_block->address + section->data.full_size)
	{
		section->data.full_size = section->data.current_block->address + section->data.current_block->size - section->data.first_block->address;
	}
	section->data.current_address += count;
}

void section_move_address(section_t * section, uint64_t new_address)
{
	section->data.current_address = new_address;
}

#if 0
void section_skip(section_t * section, size_t count, uint8_t fill)
{
	if(section->format == SECTION_ZERO_DATA)
	{
		section->data.size += count;
		fprintf(stderr, "Error: writing data into zeroed section\n");
		return;
	}
	if(section->data.size + count > section->data.buffer_size)
	{
		// an arbitrary increment
		// TODO: check alignment
		section->data.buffer_size = align_to(section->data.size + count, 16);
		section->data.buffer = section->data.buffer != NULL ? realloc(section->data.buffer, section->data.buffer_size) : malloc(section->data.buffer_size);
	}
	memset(section->data.buffer + section->data.size, fill, count);
	section->data.size += count;
}
#endif

void section_add_relocation(section_t * section, uint64_t offset, size_t size, bool pc_relative, variable_t * var, integer_t addend, size_t wrt_section, size_t hint)
{
	if(section->reloc.count >= section->reloc.buffer_size)
	{
		// an arbitrary increment
		section->reloc.buffer_size += 8;
		section->reloc.relocations = section->reloc.relocations != NULL ? realloc(section->reloc.relocations, section->reloc.buffer_size * sizeof(relocation_t)) : malloc(section->reloc.buffer_size * sizeof(relocation_t));
	}
	section->reloc.relocations[section->reloc.count].offset = offset;
	section->reloc.relocations[section->reloc.count].size = size;
	section->reloc.relocations[section->reloc.count].pc_relative = pc_relative;
	int_init_set(section->reloc.relocations[section->reloc.count].addend, addend);
	section->reloc.relocations[section->reloc.count].wrt_section = wrt_section;
	section->reloc.relocations[section->reloc.count].retrolinker_symbol_index = 0;
	section->reloc.relocations[section->reloc.count].var = *var;
	section->reloc.relocations[section->reloc.count].hint = hint;
	section->reloc.count++;

	if(elf32_segments == ELF32_SEGELF && (size == 2 || size == 4))
	{
		if(!var->segment_of)
			section->reloc.extra_count++;
	}
}

size_t section_add_symbol(section_t * section, definition_t * definition)
{
	if(section->symtab.count >= section->symtab.buffer_size)
	{
		// an arbitrary increment
		section->symtab.buffer_size += 8;
		section->symtab.symbols = section->symtab.symbols != NULL ? realloc(section->symtab.symbols, section->reloc.buffer_size * sizeof(definition_t *)) : malloc(section->symtab.buffer_size * sizeof(definition_t *));
	}
	section->symtab.symbols[section->symtab.count] = definition;
	section->symtab.count++;
	return section->symtab.count - 1;
}

size_t section_add_string(section_t * section, const char * string)
{
	size_t length = strlen(string) + 1;
	if(section->strtab.size + length > section->strtab.buffer_size)
	{
		// an arbitrary increment
		section->strtab.buffer_size = align_to(section->strtab.size + length, 16);
		section->strtab.buffer = section->strtab.buffer != NULL ? realloc(section->strtab.buffer, section->reloc.buffer_size) : malloc(section->strtab.buffer_size);
	}
	strcpy(section->strtab.buffer + section->strtab.size, string);
	section->strtab.size += length;
	return section->strtab.size - length;
}

void output_set_location(uint64_t address)
{
	switch(output.format)
	{
	case FORMAT_DEBUG:
		break;
	case FORMAT_HEX16:
	case FORMAT_HEX32:
		intel_hex_set_location(address);
		break;
	case FORMAT_REL:
	case FORMAT_OMF80:
	case FORMAT_OMF86:
		section_move_address(output.section[current_section], address);
		break;
	case FORMAT_BINARY:
	case FORMAT_COFF:
	case FORMAT_WIN32:
	case FORMAT_WIN64:
	case FORMAT_ELF32:
	case FORMAT_ELF64:
		// ignored
		break;
	}
}

size_t output_limit = (size_t)-1;

void output_byte(uint8_t value)
{
	if(output_limit == 0)
		return;
	else if(output_limit != (size_t)-1)
		output_limit--;

	switch(output.format)
	{
	case FORMAT_DEBUG:
		fprintf(output.file, " %02X", value);
		if(output.file == stdout)
			fflush(stdout);
		break;
	case FORMAT_BINARY:
		fputc(value, output.file);
		break;
	case FORMAT_HEX16:
	case FORMAT_HEX32:
		intel_hex_output_byte(value);
		break;
	case FORMAT_REL:
	case FORMAT_OMF80:
	case FORMAT_OMF86:
	case FORMAT_COFF:
	case FORMAT_WIN32:
	case FORMAT_WIN64:
	case FORMAT_ELF32:
	case FORMAT_ELF64:
		section_append(output.section[current_section], 1, &value);
		break;
	}
}

void output_flush_unit(void)
{
	// TODO
}

void output_unit(uint8_t value)
{
	// TODO
	output_byte(value);
}

void output_char(unsigned char value)
{
	// TODO
	output_byte(value);
}

void output_skip(instruction_t * ins, uint64_t count)
{
	switch(output.format)
	{
	case FORMAT_DEBUG:
		fprintf(output.file, "\nSkip %08lX\n", count);
		if(output.file == stdout)
			fflush(stdout);
		break;
	case FORMAT_BINARY:
		for(size_t offset = 0; offset < count; offset++)
			output_unit(0);
		break;
	case FORMAT_HEX16:
	case FORMAT_HEX32:
		intel_hex_skip(count);
		break;
	case FORMAT_REL:
	case FORMAT_OMF80:
	case FORMAT_OMF86:
		section_move_address(output.section[current_section], output.section[current_section]->data.current_address + count);
		break;
	case FORMAT_COFF:
	case FORMAT_WIN32:
	case FORMAT_WIN64:
	case FORMAT_ELF32:
	case FORMAT_ELF64:
		if(output.section[current_section]->format == SECTION_ZERO_DATA)
		{
			output.section[current_section]->data.full_size += count;
		}
		else if((output.section[current_section]->flags & SHF_PROGBITS) != 0)
		{
			if((output.section[current_section]->flags & SHF_EXECINSTR) != 0)
			{
				for(size_t offset = 0; offset < count; offset++)
					output_unit(nop_byte(ins, offset));
			}
			else
			{
				for(size_t offset = 0; offset < count; offset++)
					output_unit(0);
			}
		}
		break;
	}
}

static inline void output_unit_offset(integer_t value, uint64_t offset)
{
#if USE_GMP
	integer_t byte;
	mpz_init(byte);
	mpz_fdiv_q_2exp(byte, value, offset);
	output_unit(mpz_get_si(byte));
	mpz_clear(byte);
#else
	output_unit(value >> offset);
#endif
}

void add_relocation(reference_t * ref, int fmt, bitsize_t size, bool pc_relative, size_t hint)
{
	section_add_relocation(
		output.section[current_section]->data.relocations,
		output.section[current_section]->data.current_address,
		size,
		pc_relative,
		&ref->var,
		ref->value,
		ref->wrt_section,
		hint);
}

void output_word_type(reference_t * ref, int fmt, bitsize_t size, bool pc_relative, size_t hint)
{
	if(
		pc_relative ? !(ref->var.type == VAR_SECTION && ref->var.internal.section_index == current_section) : ref->var.type != VAR_NONE
	)
	{
		switch(output.format)
		{
		case FORMAT_BINARY:
		case FORMAT_HEX16:
		case FORMAT_HEX32:
			break;
		case FORMAT_DEBUG:
			fprintf(output.file, " (%s%ld",
				ref->var.segment_of ? "SEG" : pc_relative ? "REL" : "ABS",
				BITSIN(size));
			if(ref->var.type == VAR_NONE)
				fprintf(output.file, " ABS");
			else if(ref->var.type == VAR_SECTION)
				fprintf(output.file, " SECTION %s", output.section[ref->var.internal.section_index]->name);
			else
				fprintf(output.file, " %s", ref->var.external->name);
			if(ref->wrt_section == WRT_DEFAULT)
				fprintf(output.file, ")");
			else if(ref->wrt_section == WRT_NONE)
				fprintf(output.file, " WRT NONE)");
			else
				fprintf(output.file, " WRT %s)", output.section[ref->wrt_section]->name);
			break;
		case FORMAT_REL:
			// TODO: maybe store relocation in stream, instead of separately?
			add_relocation(ref, fmt, size, pc_relative, hint);
			break;
		case FORMAT_OMF80:
		case FORMAT_OMF86:
			if(pc_relative)
			{
				uint_add_ui(ref->value, section_get_current_offset(output.section[current_section]) + size);
			}
			add_relocation(ref, fmt, size, pc_relative, hint);
			break;
		case FORMAT_COFF:
			if(pc_relative)
			{
#if TARGET_X80 || TARGET_X65
				// TODO: instead, consider the address of the next instruction
				uint_add_ui(ref->value, section_get_current_offset(output.section[current_section]) + size);
#else
				uint_add_ui(ref->value, section_get_current_offset(output.section[current_section]));
#endif
			}
			add_relocation(ref, fmt, size, pc_relative, hint);
#if TARGET_X80
			int_set_ui(ref->value, 0);
#endif
			break;
		case FORMAT_WIN32:
		case FORMAT_WIN64:
			if(pc_relative)
			{
				if(ref->var.type == VAR_NONE)
					uint_add_ui(ref->value, 2 * section_get_current_offset(output.section[current_section]) + size); // alternatively, the .absolut symbol can be introduced
				else
					uint_add_ui(ref->value, section_get_current_offset(output.section[current_section]) + size);
			}
			add_relocation(ref, fmt, size, pc_relative, hint);
			break;
		case FORMAT_ELF32:
		case FORMAT_ELF64:
			if(pc_relative)
			{
//uint_print_hex(stdout, ref->value); printf("-%X\n", section_get_current_offset(output.section[current_section]));
				uint_add_ui(ref->value, section_get_current_offset(output.section[current_section]));
			}
			add_relocation(ref, fmt, size, pc_relative, hint);
			if(elf_backend_uses_rela())
				int_set_ui(ref->value, 0);
			break;
		}
	}

	for(size_t offset = 0; offset < size; offset += ARCH_BITS_IN_UNIT)
	{
		switch(fmt)
		{
		default:
			if(size != BITSIZE8)
				output_unit(0);
			else
				output_unit_offset(ref->value, 0);
			break;
		case DATA_LE:
			output_unit_offset(ref->value, offset);
			break;
		case DATA_BE:
			output_unit_offset(ref->value, size - ARCH_BITS_IN_UNIT - offset);
			break;
		case DATA_PE:
			output_unit_offset(ref->value, size - ARCH_BITS_IN_UNIT - (offset ^ (~size & ARCH_BITS_IN_UNIT)));
			break;
		}
	}
}

bool update_code_offsets(instruction_stream_t * instruction_stream)
{
	bool changed = false;

	// alter ORG and EQU definitions separately from main loop to avoid changing immediate sizes
	for(
		instruction_t * ins = instruction_stream->first_instruction;
		ins != NULL;
		ins = ins->next)
	{
#if 0
		if(output.section[ins->containing_section]->data.first_instruction == ins)
		{
			// TODO
//			ins->code_offset = output.section[ins->containing_section]->data.current_address;
		}
#endif

		if(ins->following == NULL)
			continue;
#if 0
		if(ins->following == NULL && ins->containing_section == output.section_count - 1)
			continue;
#endif

		current_section = ins->containing_section;

		long new_offset = ins->code_offset + ins->code_size;

		switch(ins->mnemonic)
		{
		case PSEUDO_MNEM_ORG:
			{
				reference_t operand[1];
				evaluate_expression(ins->operand[0].parameter, operand, ins->code_offset);
				assert_scalar(operand);
				if(!uint_fits(operand->value))
				{
					fprintf(stderr, "Fatal error: org operand too large to fit machine word\n");
				}
				new_offset = uint_get(operand->value);
				int_clear(operand->value);
			}
			break;
		case PSEUDO_MNEM_EQU:
			{
				reference_t ref[1];
				evaluate_expression(ins->operand[1].parameter, ref, ins->code_offset);
				label_define(ins->operand[0].parameter->value.s, ref);
				int_clear(ref->value);
			}
			break;
		case PSEUDO_MNEM_END_TIMES:
			new_offset = ins->termination->code_offset + (ins->code_offset - ins->termination->code_offset) * ins->termination->repetition.count;
			break;
		case PSEUDO_MNEM_END_FILL:
			new_offset = ins->termination->code_offset + ins->termination->fill.count;
			break;
		default:
			break;
		}

#if 0
		if(ins->following != NULL)
		{
#endif
		if(ins->following->code_offset < new_offset)
		{
			changed = true;
			ins->following->code_offset = new_offset;
		}
#if 0
		}
		else if(output.section[ins->containing_section + 1]->data.current_address < new_offset)
		{
			changed = true;
			output.section[ins->containing_section + 1]->data.current_address = new_offset;
		}
#endif

	}

	return changed;
}

size_t instruction_get_length(instruction_t * ins, bool forgiving)
{
	switch(ins->mnemonic)
	{
	case MNEM_NONE:
	case PSEUDO_MNEM_EQU:
	case PSEUDO_MNEM_EXTERNAL:
	case PSEUDO_MNEM_GLOBAL:
	case PSEUDO_MNEM_COMMON:
	case PSEUDO_MNEM_SECTION:
	case PSEUDO_MNEM_LOCAL_LABEL:
	case PSEUDO_MNEM_ORG:
	case PSEUDO_MNEM_TIMES:
	case PSEUDO_MNEM_END_TIMES:
	case PSEUDO_MNEM_IMPORT:
	case PSEUDO_MNEM_EXPORT:
		return 0;
	case PSEUDO_MNEM_FILL:
		{
			reference_t fill_count[1];
			evaluate_expression(ins->operand[0].parameter, fill_count, ins->code_offset);
			if(!is_scalar(fill_count) || !uint_fits(fill_count->value))
			{
				fprintf(stderr, "Fatal error: repetition count too large to fit machine word\n");
			}
			ins->fill.count = uint_get(fill_count->value);
			int_clear(fill_count->value);
		}
		return 0;
	case PSEUDO_MNEM_END_FILL:
		return ins->termination->fill.count;
	case PSEUDO_MNEM_SKIP:
		{
			reference_t ref[1];
			evaluate_expression(ins->operand[0].parameter, ref, ins->code_offset);
			assert_scalar(ref);

			size_t length = uint_get(ref->value);
			int_clear(ref->value);
			return length;
		}
	default:
		break;
	}

	if(IS_PSEUDO_MNEM_DATA(ins->mnemonic))
	{
		if(ins->operand[0].parameter->type == EXP_STRING)
		{
			size_t length = strlen(ins->operand[0].parameter->value.s) * ARCH_BITS_IN_CHAR;
			return align_to(length, _GET_DATA_SIZE(DATA_FORMAT(ins->mnemonic))) / ARCH_BITS_IN_UNIT;
		}
		else
		{
			return _GET_DATA_SIZE(DATA_FORMAT(ins->mnemonic)) / ARCH_BITS_IN_UNIT;
		}
	}

	size_t length = instruction_compute_length(ins, forgiving);
	if(length == (size_t)-1)
	{
		exit(1);
	}
	return length;
}

compilation_result_t precompile_instruction_stream(instruction_stream_t * instruction_stream)
{
	current_section = -1;

	for(
		instruction_t * ins = instruction_stream->first_instruction;
		ins != NULL;
		ins = ins->next)
	{
//printf("%ld - %d,%d\n", ins->line_number, false_if_level, past_true_if_clause);

		// bind all forward/backward label references
		for(size_t operand_index = 0; operand_index < ins->operand_count; operand_index++)
		{
			local_label_bind_all(ins->operand[operand_index].parameter);
#if TARGET_X86
			local_label_bind_all(ins->operand[operand_index].segment_value);
#endif
		}

		// chain instructions by section
		if(ins->mnemonic != PSEUDO_MNEM_SECTION)
		{
			if(output.section_count == 0)
			{
				switch(output.format)
				{
				case FORMAT_DEBUG:
				case FORMAT_BINARY:
				case FORMAT_HEX16:
				case FORMAT_HEX32:
					{
						section_t attributes;
						attributes.flags = SHF_PROGBITS | SHF_EXECINSTR | SHF_ALLOC;
						attributes.align = 1;
						current_section = objfile_locate_section(&output, ".text", SECTION_DATA_WITH_RELOC, attributes);
					}
					break;
				default:
					break;
				}
			}

			if(current_section != (size_t)-1)
			{
				if(output.section[current_section]->data.last_instruction != NULL)
					*output.section[current_section]->data.last_instruction = ins;
				else
					output.section[current_section]->data.first_instruction = ins;
				output.section[current_section]->data.last_instruction = &ins->following;
			}
		}
		ins->containing_section = current_section;

		switch(ins->mnemonic)
		{
		case PSEUDO_MNEM_SECTION:
			{
				section_t attributes;
				convert_section_attribute_list(&attributes, ins->operand[1].parameter);
				attributes.name = ins->operand[0].parameter->value.s;
				set_default_attributes(&attributes);
				current_section = objfile_locate_section(&output, ins->operand[0].parameter->value.s,
					(attributes.flags & SHF_NOBITS) != 0 ? SECTION_ZERO_DATA_WITH_RELOC : SECTION_DATA_WITH_RELOC,
					attributes);
			}
			break;
		case PSEUDO_MNEM_LOCAL_LABEL:
			if(current_section == (size_t)-1)
				fprintf(stderr, "Line %ld: label appearing outside section\n", ins->line_number);
			local_label_define(ins);
			break;
		case PSEUDO_MNEM_EXTERNAL:
			label_define_external(ins->operand[0].parameter->value.s);
			break;
		case PSEUDO_MNEM_GLOBAL:
			label_set_global(ins->operand[0].parameter->value.s);
			break;
		case PSEUDO_MNEM_COMMON:
			{
				reference_t align[1];
				definition_t * comdef;
				evaluate_expression(ins->operand[1].parameter, align, ins->code_offset);
				if(!is_scalar(align) || !uint_fits(align->value))
				{
					fprintf(stderr, "Fatal error: common symbol size too large to fit machine word\n");
				}
				comdef = label_define_common(ins->operand[0].parameter->value.s, align->value);
				int_clear(align->value);
				convert_common_attribute_list(comdef, ins->operand[2].parameter, 0);
			}
			break;
		case PSEUDO_MNEM_IMPORT:
			{
				definition_t * impdef = label_define_external(ins->operand[0].parameter->value.s);
				if(ins->operand[2].parameter != NULL && ins->operand[2].parameter->type == EXP_INTEGER)
				{
					impdef->imported.mode = ENTRY_BYORDINAL;
					impdef->imported.ordinal = uint_get(ins->operand[2].parameter->value.i);
				}
				else
				{
					impdef->imported.mode = ENTRY_BYNAME;
					impdef->imported.name = ins->operand[2].parameter ? ins->operand[2].parameter->value.s : NULL;
				}
				impdef->imported.module = ins->operand[1].parameter->value.s;
			}
			break;
		case PSEUDO_MNEM_EXPORT:
			{
				definition_t * expdef = label_set_global(ins->operand[0].parameter->value.s);
				if(ins->operand[2].parameter != NULL)
				{
					expdef->exported.mode = ENTRY_BYORDINAL;
					expdef->exported.ordinal = uint_get(ins->operand[2].parameter->value.i);
				}
				else
				{
					expdef->exported.mode = ENTRY_BYNAME;
				}
				expdef->exported.name = ins->operand[1].parameter ? ins->operand[1].parameter->value.s : NULL;
				convert_export_attribute_list(expdef, ins->operand[3].parameter);
			}
			break;
		case PSEUDO_MNEM_TIMES:
			if(current_section == (size_t)-1)
				fprintf(stderr, "Line %ld: .times appearing outside section\n", ins->line_number);

			{
				reference_t repetition_count[1];
				evaluate_expression(ins->operand[0].parameter, repetition_count, ins->code_offset);
				if(!is_scalar(repetition_count) || !uint_fits(repetition_count->value))
				{
					fprintf(stderr, "Fatal error: repetition count too large to fit machine word\n");
				}
				ins->repetition.count = uint_get(repetition_count->value);
				int_clear(repetition_count->value);
			}
			break;
		case PSEUDO_MNEM_FILL:
			if(current_section == (size_t)-1)
				fprintf(stderr, "Line %ld: fill appearing outside section\n", ins->line_number);

			ins->fill.count = 0;
			break;
		default:
			break;
		}

		if(IS_PSEUDO_MNEM_DATA(ins->mnemonic))
		{
			if(current_section == (size_t)-1)
				fprintf(stderr, "Line %ld: data directive appearing outside section\n", ins->line_number);

			int fmt = DATA_FORMAT(ins->mnemonic);
			long size = _GET_DATA_SIZE(fmt);
			fmt &= DATA_LAYOUT_MASK;
			if(size > ARCH_BITS_IN_UNIT && fmt != DATA_LE && fmt != DATA_BE && fmt != DATA_PE)
			{
				fprintf(stderr, "Invalid data format");
			}
			else if(fmt == DATA_PE && size > ARCH_BITS_IN_UNIT && (size & ARCH_BITS_IN_UNIT) != 0)
			{
				fprintf(stderr, "Invalid data format");
			}
		}
		else if(ins->mnemonic > MNEM_NONE)
		{
			if(current_section == (size_t)-1)
				fprintf(stderr, "Line %ld: instruction appearing outside section\n", ins->line_number);
		}

		ins->code_size = instruction_get_length(ins, true);
	}

	// terminate sections with empty instruction (needed for trailing label definitions) TODO: is this still necessary?
	for(current_section = 0; current_section < output.section_count; current_section ++)
	{
		if(output.section[current_section]->data.last_instruction == NULL)
			continue;

		instruction_clear(&current_instruction, current_parser_state);
		current_instruction.mnemonic = MNEM_NONE;

		*output.section[current_section]->data.last_instruction = instruction_clone(&current_instruction);
	}

	return update_code_offsets(instruction_stream) ? RESULT_CHANGED : RESULT_COMPLETE;
}

compilation_result_t compile_instruction_stream(instruction_stream_t * instruction_stream)
{
	for(
		instruction_t * ins = instruction_stream->first_instruction;
		ins != NULL;
		ins = ins->next)
	{
		ins->code_size = instruction_get_length(ins, false);
	}

// TODO: verify all times/endtimes pairs are in the same section

	return update_code_offsets(instruction_stream) ? RESULT_CHANGED : RESULT_COMPLETE;
}

#ifndef expression_get_hint
# define expression_get_hint(opd, val, fmt, size, pcrel) 0
#endif

compilation_result_t generate_instruction_stream(instruction_stream_t * instruction_stream)
{
	for(
		current_section = 0;
		current_section < output.section_count;
		current_section ++)
	{
		if(output.format == FORMAT_DEBUG)
			fprintf(output.file, "Section %ld %s\n", current_section, output.section[current_section]->name);

		if(output.section[current_section]->format != SECTION_DATA && output.section[current_section]->format != SECTION_ZERO_DATA)
			continue;

		for(
			instruction_t * ins = output.section[current_section]->data.first_instruction;
			ins != NULL;
			ins = ins->following)
		{

			if(output.format == FORMAT_DEBUG)
			{
				if(ins != output.section[current_section]->data.first_instruction)
					fprintf(output.file, "\n");
				fprintf(output.file, "next [#%ld,0x%lX,(0x%lX)]:", ins->line_number, ins->code_offset, ins->code_size);
			}

			switch(ins->mnemonic)
			{
			case PSEUDO_MNEM_SECTION:
				continue;
			case PSEUDO_MNEM_ORG:
				output_set_location(ins->following->code_offset);
				continue;
			case PSEUDO_MNEM_SKIP:
				output_skip(ins, ins->code_size);
				continue;
			case PSEUDO_MNEM_END_TIMES:
				if(ins->repetition.current >= ins->termination->repetition.count - 1)
				{
					ins->repetition.current = 0;
				}
				else
				{
					ins->repetition.current ++;
					ins = ins->termination; // restart
				}
				continue;
			case PSEUDO_MNEM_FILL:
				ins->fill.old_limit = output_limit;
				output_limit = ins->fill.count;
				continue;
			case PSEUDO_MNEM_END_FILL:
				if(output_limit == 0)
				{
					output_limit = ins->termination->fill.old_limit;
				}
				else
				{
					ins = ins->termination; // restart
				}
				continue;
			default:
				break;
			}

			if(IS_PSEUDO_MNEM_DATA(ins->mnemonic))
			{
				int fmt = DATA_FORMAT(ins->mnemonic);
				long size = _GET_DATA_SIZE(fmt);
				fmt &= DATA_LAYOUT_MASK;

				if(ins->operand[0].parameter->type == EXP_STRING)
				{
					char * string = ins->operand[0].parameter->value.s;
					long length = strlen(string) * ARCH_BITS_IN_CHAR;
					length = align_to(length, size) / ARCH_BITS_IN_UNIT;
					size_t offset = 0;
					for(offset = 0; string[offset] != '\0'; offset++)
					{
						output_char(string[offset]);
					}
					output_flush_unit();
					for(offset = (offset * ARCH_BITS_IN_CHAR) / ARCH_BITS_IN_UNIT; offset < length; offset++)
					{
						output_unit(0);
					}
				}
				else
				{
					reference_t ref[1];
					evaluate_expression(ins->operand[0].parameter, ref, ins->code_offset);
					int hint = expression_get_hint(&ins->operand[0], ref, fmt, size, false);
					output_word_type(ref, fmt, size, false, hint);
					int_clear(ref->value);
				}

				continue;
			}

			if(ins->mnemonic <= 0)
				continue;

			generate_instruction(ins);
		}

		if(output.section[current_section]->data.first_instruction != NULL && output.format == FORMAT_DEBUG)
			fprintf(output.file, "\n");
	}

	return RESULT_COMPLETE;
}

void make_instruction_groups(instruction_t ** iterator, mnemonic_t current_enclosure, instruction_t * enclosure_head)
{
	while(*iterator != NULL)
	{
		instruction_t * ins = *iterator;
		*iterator = (*iterator)->next;
		switch(ins->mnemonic)
		{
		case PSEUDO_MNEM_TIMES:
			make_instruction_groups(iterator, PSEUDO_MNEM_TIMES, ins);
			break;
		case PSEUDO_MNEM_FILL:
			make_instruction_groups(iterator, PSEUDO_MNEM_FILL, ins);
			break;
		case PSEUDO_MNEM_END_TIMES:
			if(current_enclosure != PSEUDO_MNEM_TIMES)
			{
				fprintf(stderr, "Error in line %ld: no matching .times found for .endtimes\n", ins->line_number);
			}
			else
			{
//				fprintf(stderr, "Debug: .rep in line %ld terminated in line %ld\n", enclosure_head->line_number, ins->line_number);
				enclosure_head->termination = ins;
				ins->termination = enclosure_head;
				return;
			}
			break;
		case PSEUDO_MNEM_END_FILL:
			if(current_enclosure != PSEUDO_MNEM_FILL)
			{
				fprintf(stderr, "Error in line %ld: no matching .fill found for .endfill\n", ins->line_number);
			}
			else
			{
//				fprintf(stderr, "Debug: .fill in line %ld terminated in line %ld\n", enclosure_head->line_number, ins->line_number);
				enclosure_head->termination = ins;
				ins->termination = enclosure_head;
				return;
			}
			break;
		default:
			break;
		}
	}
	if(current_enclosure != MNEM_NONE)
	{
		fprintf(stderr, "Error: unterminated %s starting in line %ld\n",
			current_enclosure == PSEUDO_MNEM_TIMES ? ".times"
			: current_enclosure == PSEUDO_MNEM_FILL ? ".fill"
			: "<undefined>", // internal error
			enclosure_head->line_number);
	}
}

const char * entry_point_name = NULL;

#define VERSION "0.1"

void print_version(void)
{
	printf("asm-" TARGET_NAME " version " VERSION "\n");
}

void print_usage(char * argv0)
{
	printf("Usage: %s [flags] <input filename>\n"
		"\t-v\tDisplay version number\n"
		"\t-h\tDisplay this help page\n"
		"\t-f<format>\tSelect output format:\n"
		"\t\tbin\tFlat binary file\n"
		"\t\thex16\tIntel HEX\n"
		"\t\thex32\t32-bit Intel HEX\n"
		"\t\tomf80\tIntel Object Module Format for 8080\n"
		"\t\tomf86\tIntel Object Module Format for 8086/386\n"
		"\t\trel\tMicrosoft REL format\n"
		"\t\tcoff\tUNIX COFF format\n"
		"\t\twin32\t32-bit Microsoft PE format\n"
		"\t\twin64\t64-bit Microsoft PE format\n"
		"\t\telf32\t32-bit ELF\n"
		"\t\telf64\t64-bit ELF\n"
		"\t-o<output filename>\tSpecify output file name\n",
		argv0);
}

extern void rel_generate(const char * module_name);

bool is_preprocessing_stage = true;

int main(int argc, char ** argv)
{
	char * input_filename = NULL;
	char * output_filename = NULL;

	output.format = FORMAT_BINARY;

	for(int i = 1; i < argc; i++)
	{
		if(argv[i][0] == '-')
		{
			char * arg;
			switch(argv[i][1])
			{
			case 'v':
				print_version();
				break;
			case 'h':
				print_usage(argv[0]);
				break;
			case 'f':
				arg = argv[i][2] ? &argv[i][2] : i + 1 < argc ? argv[++i] : NULL;
				if(arg == NULL)
				{
					fprintf(stderr, "No output format provided\n");
					exit(1);
				}
				else if(strcasecmp(arg, "bin") == 0)
				{
					output.format = FORMAT_BINARY;
				}
				else if(strcasecmp(arg, "hex") == 0)
				{
					output.format = FORMAT_HEX16;
				}
				else if(strcasecmp(arg, "hex16") == 0)
				{
					output.format = FORMAT_HEX16;
				}
				else if(strcasecmp(arg, "hex32") == 0)
				{
					output.format = FORMAT_HEX32;
				}
				else if(strcasecmp(arg, "elf") == 0)
				{
					output.format = FORMAT_ELF32;
				}
				else if(strcasecmp(arg, "elf32") == 0)
				{
					output.format = FORMAT_ELF32;
				}
				else if(strcasecmp(arg, "elf64") == 0)
				{
					output.format = FORMAT_ELF64;
				}
				else if(strcasecmp(arg, "omf80") == 0)
				{
					output.format = FORMAT_OMF80;
				}
				else if(strcasecmp(arg, "omf86") == 0)
				{
					output.format = FORMAT_OMF86;
				}
				else if(strcasecmp(arg, "omf") == 0)
				{
					output.format = FORMAT_OMF86;
				}
				else if(strcasecmp(arg, "rel") == 0)
				{
					output.format = FORMAT_REL;
				}
				else if(strcasecmp(arg, "coff") == 0)
				{
					output.format = FORMAT_COFF;
				}
				else if(strcasecmp(arg, "win32") == 0)
				{
					output.format = FORMAT_WIN32;
				}
				else if(strcasecmp(arg, "win64") == 0)
				{
					output.format = FORMAT_WIN64;
				}
				else if(strcasecmp(arg, "d") == 0)
				{
					output.format = FORMAT_DEBUG;
				}
				else
				{
					fprintf(stderr, "Unknown output format: `%s'\n", arg);
					exit(1);
				}
				break;
			case 'o':
				arg = argv[i][2] ? &argv[i][2] : i + 1 < argc ? argv[++i] : NULL;
				if(arg == NULL)
				{
					fprintf(stderr, "No output filename provided\n");
					exit(1);
				}
				else if(output_filename != NULL)
				{
					fprintf(stderr, "Duplicate output filename provided\n");
					exit(1);
				}
				else
				{
					output_filename = arg;
				}
				break;
			case 's':
				arg = argv[i][2] ? &argv[i][2] : i + 1 < argc ? argv[++i] : NULL;
				if(arg == NULL)
				{
					fprintf(stderr, "No segmentation mode provided\n");
					exit(1);
				}
				else if(strcasecmp(arg, "none") == 0)
				{
					elf32_segments = ELF32_NO_SEGMENTS;
				}
				else if(strcasecmp(arg, "vma") == 0)
				{
					elf32_segments = ELF32_VMA_SEGMENTS;
				}
				else if(strcasecmp(arg, "segelf") == 0)
				{
					elf32_segments = ELF32_SEGELF;
				}
				else if(strcasecmp(arg, "link") == 0)
				{
					elf32_segments = ELF32_RETROLINKER;
				}
				else
				{
					fprintf(stderr, "Unknown segmentation format: `%s'\n", arg);
					exit(1);
				}
				break;
			case 'D':
				{
					reference_t ref[1];
					char * nameptr = argv[i][2] ? &argv[i][2] : i + 1 < argc ? argv[++i] : NULL;
					char * equptr = strchr(nameptr, '=');
					if(equptr == NULL)
					{
						reference_set_ui(ref, 1);
						label_define(strdup(nameptr), ref);
					}
					else
					{
						integer_value_t value;
						uint_parse(value, equptr + 1, 10);
						reference_set(ref, INTVAL(value));
						int_delete(value);
						char * name = malloc(equptr - nameptr + 1);
						memcpy(name, nameptr, equptr - nameptr);
						name[equptr - nameptr] = '\0';
						label_define(name, ref);
					}
				}
				break;
			default:
				fprintf(stderr, "Unknown flag: `%s'\n", argv[i]);
				exit(1);
			}
		}
		else if(input_filename != NULL)
		{
			fprintf(stderr, "Duplicate input filename provided\n");
			exit(1);
		}
		else
		{
			input_filename = argv[i];
		}
	}

	if(output.format == FORMAT_DEBUG)
		printf("%d\n", _MNEM_TOTAL);

	if(output.format == FORMAT_OMF86 && elf32_segments != ELF32_NO_SEGMENTS)
	{
		fprintf(stderr, "Segmentation specification ignored for OMF\n");
	}
	else if(output.format != FORMAT_ELF32 && elf32_segments != ELF32_NO_SEGMENTS && elf32_segments != ELF32_RETROLINKER)
	{
		fprintf(stderr, "Segmentation not supported for this format\n");
		elf32_segments = ELF32_NO_SEGMENTS;
	}

#if TARGET_X86
	switch(output.format)
	{
	case FORMAT_BINARY:
	case FORMAT_REL:
	case FORMAT_HEX16:
	case FORMAT_OMF86:
		current_parser_state->cpu_type = CPU_8086;
		current_parser_state->bit_size = BITSIZE16;
		break;
	case FORMAT_HEX32:
	case FORMAT_ELF32:
	case FORMAT_COFF:
	case FORMAT_WIN32:
		current_parser_state->cpu_type = CPU_386;
		current_parser_state->bit_size = BITSIZE32;
		break;
	case FORMAT_DEBUG:
	case FORMAT_ELF64:
	case FORMAT_WIN64:
		current_parser_state->cpu_type = CPU_X64;
		current_parser_state->bit_size = BITSIZE64;
		break;
	case FORMAT_OMF80:
		current_parser_state->cpu_type = CPU_8080;
		break;
	}
#endif

	setup_lexer(current_parser_state);

	if(input_filename != NULL)
	{
		stdin = freopen(input_filename, "r", stdin);
		if(stdin == NULL)
		{
			fprintf(stderr, "Error: unable to open %s for reading\n", input_filename);
			exit(1);
		}
	}

	int result = yyparse();
	if(result != 0)
		return result;

	is_preprocessing_stage = false;

	instruction_t * iterator = current_parser_state->stream.first_instruction;
	make_instruction_groups(&iterator, MNEM_NONE, NULL);

	compilation_result_t cr;
	cr = precompile_instruction_stream(&current_parser_state->stream);
	if(cr == RESULT_FAILED)
		return 1;

	bool completed = false;
	while(!completed)
	{
		completed = true;
		cr = compile_instruction_stream(&current_parser_state->stream);
		if(cr == RESULT_FAILED)
			return 1;
		if(cr != RESULT_COMPLETE)
			completed = false;
	}

	if(output_filename == NULL && output.format == FORMAT_DEBUG)
	{
		output.file = stdout;
	}
	else
	{
		if(output_filename == NULL)
		{
			const char * extension;
			switch(output.format)
			{
			case FORMAT_HEX16:
			case FORMAT_HEX32:
				extension = ".hex";
				break;
			case FORMAT_REL:
				extension = ".rel";
				break;
			case FORMAT_OMF80:
			case FORMAT_OMF86:
			case FORMAT_WIN32:
			case FORMAT_WIN64:
				extension = ".obj";
				break;
			case FORMAT_COFF:
			case FORMAT_ELF32:
			case FORMAT_ELF64:
				extension = ".o";
				break;
			default:
				extension = "";
				break;
			}
			if(input_filename != NULL)
			{
				char * dot = strrchr(input_filename, '.');
				if(dot != NULL && strchr(dot, '/') != NULL)
					dot = NULL;

				if(dot == NULL)
				{
					output_filename = malloc(strlen(input_filename) + strlen(extension) + 1);
					strcpy(output_filename, input_filename);
					strcat(output_filename, extension);
				}
				else
				{
					output_filename = malloc(dot - input_filename + strlen(extension) + 1);
					memcpy(output_filename, input_filename, dot - input_filename);
					strcpy(output_filename + (dot - input_filename), extension);
				}
			}
			else
			{
				output_filename = malloc(1 + strlen(extension) + 1);
				output_filename[0] = 'a';
				strcpy(output_filename + 1, extension);
			}
		}
		if(output_format_is_textual(output.format))
			output.file = fopen(output_filename, "w");
		else
			output.file = fopen(output_filename, "wb");
	}

	cr = generate_instruction_stream(&current_parser_state->stream);

	if(cr == RESULT_FAILED)
		return 1;

	switch(output.format)
	{
	case FORMAT_DEBUG:
		{
			definition_t * current;
			for(current = globals; current != NULL; current = current->next)
			{
				static const char deftype_symbol[] = "IEC";
				printf("%s %c%c 0x", current->name, current->global ? 'G' : 'L', deftype_symbol[current->deftype]);
				uint_print_hex(stdout, current->ref.value);

				if(current->ref.var.type != VAR_NONE)
				{
					if(current->ref.var.segment_of)
					{
						printf(" seg");
					}
					if(current->ref.var.type == VAR_SECTION)
					{
						printf(" sect %ld", current->ref.var.internal.section_index);
					}
					else if(current->ref.var.type == VAR_DEFINE)
					{
						printf(" %s", current->ref.var.external->name);
					}
				}
				if(current->ref.wrt_section != WRT_DEFAULT)
					printf(" wrt %ld", current->ref.wrt_section); //output.section[current->ref.wrt_section]->name);

				switch(current->imported.mode)
				{
				case ENTRY_NONE:
					break;
				case ENTRY_BYORDINAL:
					printf(" import from %s by ordinal 0x%04X (%d)", current->imported.module, current->imported.ordinal, current->imported.ordinal);
					break;
				case ENTRY_BYNAME:
					printf(" import from %s by name", current->imported.module);
					if(current->imported.name != NULL)
						printf(" %s", current->imported.name);
					break;
				}

				switch(current->exported.mode)
				{
				case ENTRY_NONE:
					break;
				case ENTRY_BYORDINAL:
					printf(" export by ordinal 0x%04X (%d)", current->exported.ordinal, current->exported.ordinal);
					if(current->exported.name != NULL)
						printf(" with name %s", current->exported.name);
					break;
				case ENTRY_BYNAME:
					printf(" export by name");
					if(current->exported.name != NULL)
						printf(" %s", current->exported.name);
					break;
				}

				printf("\n");
			}
		}
		break;

	case FORMAT_BINARY:
		break;

	case FORMAT_HEX16:
	case FORMAT_HEX32:
		intel_hex_close();
		break;

	case FORMAT_REL:
		rel_generate(input_filename);
		break;

	case FORMAT_OMF80:
	case FORMAT_OMF86:
		omf_generate(input_filename);
		break;

	case FORMAT_ELF32:
	case FORMAT_ELF64:
		elf_generate();
		break;

	case FORMAT_COFF:
	case FORMAT_WIN32:
	case FORMAT_WIN64:
		coff_generate(input_filename);
		break;
	}

	if(output.file != stdout)
		fclose(output.file);

	return 0;
}

