
#include <stdint.h>
#include "asm.h"
#include "elf.h"

#define entry_point_name (entry_point_name == NULL ? "..start" : entry_point_name)

static uint8_t rel_bit_buffer = 0;
static size_t rel_bit_buffer_size = 0;

typedef enum relative_t
{
	REL_ABS = 0,
	REL_CODE = 1,
	REL_DATA = 2,
	REL_COMMON = 3,
} relative_t;

typedef enum special_t
{
	// name
	SPEC_PUBLIC_NAME = 0x0,
	SPEC_SELECT_COMMON = 0x1,
	SPEC_MODULE_NAME = 0x2,
	// name, value
	SPEC_DEFINE_COMMON = 0x5,
	SPEC_EXTERNAL_CHAIN = 0x6,
	SPEC_DEFINE_PUBLIC = 0x7,
	// value
	SPEC_EXTERNAL_OFFSET = 0x9,
	SPEC_DATA_SIZE = 0xA,
	SPEC_SET_LOCATION = 0xB,
	SPEC_CHAIN_ADDRESS = 0xC, // TODO: what is this used for?
	SPEC_CODE_SIZE = 0xD,
	SPEC_MODULE_END = 0xE,
	SPEC_FILE_END = 0xF,
} special_t;

void rel_bit_buffer_flush(void)
{
	fwrite8(output.file, rel_bit_buffer);
	rel_bit_buffer = 0;
	rel_bit_buffer_size = 0;
}

// maximum only 8 bits allowed
void rel_writebits(size_t count, uint8_t value)
{
	if(rel_bit_buffer_size + count >= 8)
	{
		rel_bit_buffer |= value >> (rel_bit_buffer_size + count - 8);
		value &= ~(-1 << (rel_bit_buffer_size + count - 8));
		count -= 8 - rel_bit_buffer_size;
		rel_bit_buffer_flush();
	}
	if(count > 0)
	{
		rel_bit_buffer |= value << (8 - rel_bit_buffer_size - count);
		rel_bit_buffer_size += count;
	}
}

void rel_writebyte(uint8_t value)
{
	rel_writebits(8, value);
}

// this cannot be rel_writebits(16, value), as that would output the value in big endian byte order
void rel_writeword(uint16_t value)
{
	rel_writebyte(value);
	rel_writebyte(value >> 8);
}

static uint16_t last_location = 0;

void rel_putbyte(uint8_t value)
{
	rel_writebits(1, 0);
	rel_writebyte(value);
	last_location ++;
}

void rel_putword(uint8_t value)
{
	rel_putbyte(value);
	rel_putbyte(value >> 8);
}

// only REL_CODE, REL_DATA, REL_COMMON allowed
void rel_putrel(relative_t relative, uint16_t value)
{
	rel_writebits(3, 4 + relative);
	rel_writeword(value);
	last_location += 2;
}

void rel_putaddress(int relative, uint16_t address)
{
	rel_writebits(2, relative);
	rel_writeword(address);
}

void rel_putname(size_t length, const char * name)
{
	if(length > 7)
		length = 7;
	rel_writebits(3, length);
	for(size_t i = 0; i < length; i++)
		rel_writebyte(name[i]);
}

void rel_putspecial(uint8_t code)
{
	rel_writebits(7, 0x40 + code);
}

void rel_put_module_name(const char * module_name)
{
	rel_putspecial(SPEC_MODULE_NAME);
	rel_putname(strlen(module_name), module_name);
}

void rel_put_define_common(uint16_t size, const char * name)
{
	rel_putspecial(SPEC_DEFINE_COMMON);
	rel_putaddress(REL_ABS, size);
	rel_putname(strlen(name), name);
}

void rel_put_public_name(const char * name)
{
	rel_putspecial(SPEC_PUBLIC_NAME);
	rel_putname(strlen(name), name);
}

void rel_put_define_public(const char * name, relative_t relative, uint16_t value)
{
	rel_putspecial(SPEC_DEFINE_PUBLIC);
	rel_putaddress(relative, value);
	rel_putname(strlen(name), name);
}

void rel_put_data_size(uint16_t size)
{
	rel_putspecial(SPEC_DATA_SIZE);
	rel_putaddress(REL_ABS, size);
}

void rel_put_code_size(uint16_t size)
{
	rel_putspecial(SPEC_CODE_SIZE);
	rel_putaddress(REL_CODE, size); // M80 writes it as program relative
}

void rel_put_external_offset(uint16_t chain, relative_t relative, uint16_t offset)
{
	if(offset != 0)
	{
		rel_putspecial(SPEC_EXTERNAL_OFFSET);
		rel_putaddress(REL_ABS, offset); // TODO: REL_ABS?
	}
	if(relative == REL_ABS)
		rel_putword(chain);
	else
		rel_putrel(relative, chain);
}

void rel_put_external(const char * name, relative_t relative, uint16_t chain)
{
	rel_putspecial(SPEC_EXTERNAL_CHAIN);
	rel_putaddress(relative, chain);
	rel_putname(strlen(name), name);
}

void rel_put_select_common(const char * common_name)
{
	rel_putspecial(SPEC_SELECT_COMMON);
	rel_putname(strlen(common_name), common_name);
}

void rel_put_set_location(int relative, uint16_t address)
{
	rel_putspecial(SPEC_SET_LOCATION);
	rel_putaddress(relative, address);
}

void rel_put_module_end(int start_relative, uint16_t start_address)
{
	rel_putspecial(SPEC_MODULE_END);
	rel_putaddress(start_relative, start_address);
}

static const char * last_common = NULL;

void rel_select_common(const char * common_name)
{
	if(last_common == NULL || strcmp(last_common, common_name) != 0)
	{
		rel_put_select_common(common_name);
		last_common = common_name;
	}
}

static relative_t last_location_relative = REL_ABS;

void rel_set_location(int relative, uint16_t address, const char * common_name)
{
	if(relative == REL_COMMON)
	{
		rel_select_common(common_name);
	}

	if(relative != last_location_relative || address != last_location)
	{
		rel_put_set_location(relative, address);
		last_location_relative = relative;
		last_location = address;
	}
}

void rel_generate(const char * module_name)
{
	rel_put_module_name(module_name);

	for(definition_t * current = globals; current != NULL; current = current->next)
	{
		if(current->deftype == DEFTYPE_COMMON)
		{
			rel_put_define_common(uint_get(current->size.value), current->name);
		}
	}

	definition_t * start_symbol = NULL;
	for(definition_t * current = globals; current != NULL; current = current->next)
	{
		if(current->deftype == DEFTYPE_EQU && current->global)
		{
			if(strcmp(current->name, entry_point_name) == 0)
			{
				start_symbol = current;
				continue;
			}
			rel_put_public_name(current->name);
		}
	}

	uint32_t data_size = 0;
	uint32_t code_size = 0;
	for(size_t section_index = 0; section_index < output.section_count; section_index++)
	{
		if(output.section[section_index]->format != SECTION_DATA && output.section[section_index]->format != SECTION_ZERO_DATA)
			continue;

		if((output.section[section_index]->flags & SHF_EXECINSTR) == 0)
		{
			data_size += output.section[section_index]->data.full_size;
		}
		else
		{
			code_size += output.section[section_index]->data.full_size;
		}
	}

	rel_put_data_size(data_size);
	rel_put_code_size(code_size);

	last_location_relative = REL_ABS;
	last_location = 0;
	for(size_t section_index = 0; section_index < output.section_count; section_index++)
	{
		if(output.section[section_index]->format != SECTION_DATA && output.section[section_index]->format != SECTION_ZERO_DATA)
			continue;

		relative_t rel;
		if((output.section[section_index]->flags & SHF_EXECINSTR) == 0)
		{
			rel = REL_DATA;
		}
		else
		{
			rel = REL_CODE;
		}
		// TODO: abs, maybe common?

		section_t * relocations = output.section[section_index]->data.relocations;
		size_t relocation_index = 0;

		for(
			block_t * current = output.section[section_index]->data.first_block;
			current != NULL;
			current = current->next
		)
		{
			rel_set_location(rel, current->address, NULL);

			for(size_t offset = 0; offset < current->size; offset++)
			{
				if(relocation_index < relocations->reloc.count && relocations->reloc.relocations[relocation_index].offset == last_location)
				{
					relocation_t relocation = relocations->reloc.relocations[relocation_index];
					relocation_index ++;

					if(relocation.size == 2 && !relocation.pc_relative)
					{
						if(relocation.var.type == VAR_SECTION)
						{
							if((output.section[relocation.var.internal.section_index]->flags & SHF_EXECINSTR) == 0)
							{
								rel_putrel(REL_DATA, uint_get(relocation.addend));
							}
							else
							{
								rel_putrel(REL_CODE, uint_get(relocation.addend));
							}
						}
						else if(relocation.var.type == VAR_DEFINE)
						{
							switch(relocation.var.external->deftype)
							{
							case DEFTYPE_EXTERNAL:
								uint16_t chain = last_location;
								rel_put_external_offset(
									relocation.var.external->elf_symbol_index,
									relocation.var.external->elf_symbol_index >> 16,
									uint_get(relocation.addend));
								relocation.var.external->elf_symbol_index = (chain & 0xFFFF) | ((uint32_t)rel << 16);
								break;
							case DEFTYPE_COMMON:
								rel_select_common(relocation.var.external->name);
								rel_putrel(REL_COMMON, uint_get(relocation.addend));
								break;
							default:
								break;
							}
						}
						else
						{
							fprintf(stderr, "Oh no\n");
							// TODO: probably not a real relocation
						}

						offset += relocation.size;
						continue;
					}
				}
				// TODO: multiple blocks
				rel_putbyte(current->buffer[offset]);
			}
		}
	}

	for(definition_t * current = globals; current != NULL; current = current->next)
	{
		if(current->deftype == DEFTYPE_EXTERNAL && current->global)
		{
			rel_put_external(current->name, current->elf_symbol_index >> 16, current->elf_symbol_index);
		}
	}

	for(definition_t * current = globals; current != NULL; current = current->next)
	{
		if(current->deftype == DEFTYPE_EQU && current->global && current->ref.var.type == VAR_SECTION)
		{
			if(current == start_symbol)
				continue;

			relative_t relative;
			switch(current->ref.var.type)
			{
			case VAR_NONE:
				relative = REL_ABS;
				break;
			case VAR_SECTION:
				if((output.section[current->ref.var.internal.section_index]->flags & SHF_EXECINSTR) == 0)
					relative = REL_DATA;
				else
					relative = REL_CODE;
				break;
			default:
				// TODO: maybe common relative?
				continue;
			}
			rel_put_define_public(current->name, relative, uint_get(current->ref.value));
		}
	}

	relative_t start_relative = REL_ABS;
	uint16_t start_address = 0;
	if(start_symbol != NULL)
	{
		switch(start_symbol->ref.var.type)
		{
		case VAR_NONE:
			start_relative = REL_ABS;
			break;
		case VAR_SECTION:
			if((output.section[start_symbol->ref.var.internal.section_index]->flags & SHF_EXECINSTR) == 0)
				start_relative = REL_DATA;
			else
				start_relative = REL_CODE;
			break;
		default:
			// TODO: maybe common relative?
			goto no_start_symbol;
		}
		start_address = uint_get(start_symbol->ref.value);
	}
no_start_symbol:

	rel_put_module_end(start_relative, start_address);
	rel_bit_buffer_flush();
	rel_putspecial(SPEC_FILE_END);
	rel_bit_buffer_flush();
}

