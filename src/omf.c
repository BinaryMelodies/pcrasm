
#include <stdio.h>
#include "asm.h"
#include "omf.h"
#include "elf.h"

#define entry_point_name (entry_point_name == NULL ? "..start" : entry_point_name)

uint8_t current_record_type = 0;
long current_record_start;
uint16_t current_record_size;
uint8_t current_record_checksum;

static void omf_begin_record(uint8_t type)
{
	current_record_type = type;
	current_record_start = ftell(output.file);
	fwrite8(output.file, type);
	current_record_size = 0;
	fwrite16le(output.file, current_record_size);
	current_record_checksum = 0;
}

static void omf_putbyte(uint8_t byte)
{
	fwrite8(output.file, byte);
	current_record_size ++;
	current_record_checksum -= byte;
}

static void omf_end_record(void)
{
	current_record_type = 0;
	omf_putbyte(current_record_checksum);
	long end = ftell(output.file);
	fseek(output.file, current_record_start + 1, SEEK_SET);
	fwrite16le(output.file, current_record_size);
	fseek(output.file, end, SEEK_SET);
}

static void omf_putword(uint16_t word)
{
	omf_putbyte(word);
	omf_putbyte(word >> 8);
}

static void omf_putdword(uint32_t dword)
{
	omf_putbyte(dword);
	omf_putbyte(dword >> 8);
	omf_putbyte(dword >> 16);
	omf_putbyte(dword >> 24);
}

static void omf_putoffset(uint32_t value)
{
	if((current_record_type & 1) == 0)
		omf_putword(value);
	else
		omf_putdword(value);
}

static void omf_putnumber(uint32_t value)
{
	if(value <= 0x80)
	{
		omf_putbyte(value);
	}
	else if(value <= 0x10000)
	{
		omf_putbyte(0x81);
		omf_putword(value);
	}
	else if(value <= 0x1000000)
	{
		omf_putbyte(0x81);
		omf_putword(value);
		omf_putbyte(value >> 16);
	}
	else if(value <= 0x100000000)
	{
		omf_putbyte(0x81);
		omf_putdword(value);
	}
}

static void omf_putstring(const char * text)
{
	// TODO: verify that text is not longer than 255 bytes
	omf_putbyte(strlen(text));
	for(size_t i = 0; text[i] != '\0'; i++)
		omf_putbyte(text[i]);
}

static void omf_putindex(uint16_t value)
{
	if(value < 0x80)
	{
		omf_putbyte(value);
	}
	else
	{
		omf_putbyte(0x80 | (value >> 8));
		omf_putbyte(value);
	}
}

#define MAX(a, b) ((a) >= (b) ? (a) : (b))

void omf80_generate(const char * module_name)
{
	// common symbols have to be allocated their own segments

	size_t section_number = 1;

	for(size_t section_index = 0; section_index < output.section_count; section_index++)
	{
		if(output.section[section_index]->format != SECTION_DATA && output.section[section_index]->format != SECTION_ZERO_DATA)
			continue;

		// TODO: must be code/data/stack/heap
		output.section[section_index]->elf_symbol_index = section_number++;

		if(section_number == 5)
			section_number++; // reserved
	}

	size_t common_symbols = 0;

	for(definition_t * current = globals; current != NULL; current = current->next)
	{
		if(current->deftype == DEFTYPE_COMMON)
		{
			//current->elf_symbol_index = named_common_section--;
			common_symbols ++;
			if(common_symbols >= 256 - MAX(6, section_number))
			{
				fprintf(stderr, "Too many common symbols\n");
				break;
			}
		}
	}

	for(size_t symbol_index = 0; symbol_index < common_symbols; symbol_index++)
	{
		objfile_new_section(&output, NULL, SECTION_ZERO_DATA, section_attributes(SHF_NOBITS, 1));
	}

	size_t named_common_section = 254;
	for(definition_t * current = globals; current != NULL; current = current->next)
	{
		if(current->deftype == DEFTYPE_COMMON)
		{
			if(named_common_section == MAX(5, section_number - 1))
				break;
			current->elf_symbol_index = named_common_section--;
			size_t size;
			size = uint_get(current->size.value);
			output.section[current->elf_symbol_index + output.section_count - 255]->data.full_size = size;

			size = uint_get(current->ref.value);
			output.section[current->elf_symbol_index + output.section_count - 255]->align = size;
		}
	}

	omf_begin_record(OMF_THEADR80);
	omf_putstring(module_name);
	omf_putword(0); // reserved
	for(size_t section_index = 0; section_index < output.section_count; section_index++)
	{
		if(output.section[section_index]->format != SECTION_DATA && output.section[section_index]->format != SECTION_ZERO_DATA)
			continue;

		omf_putbyte(output.section[section_index]->elf_symbol_index);
		omf_putword(output.section[section_index]->data.full_size);
		if(output.section[section_index]->align <= 1)
			omf_putbyte(3);
		else
			omf_putbyte(2); // 256-byte alignment
	}
	omf_end_record();

	// EXTDEF, PUBDEF
	uint16_t external_names = 0;
	bool record_started = false;
	for(definition_t * current = globals; current != NULL; current = current->next)
	{
		if(current->deftype == DEFTYPE_EXTERNAL)
		{
			if(!record_started)
			{
				omf_begin_record(OMF_EXTDEF80);
				record_started = true;
			}
			omf_putstring(current->name);
			omf_putbyte(0); // reserved
			current->elf_symbol_index = external_names++;
		}
	}
	if(record_started)
		omf_end_record();

	// TODO: group by segments?
	for(definition_t * current = globals; current != NULL; current = current->next)
	{
		if(current->deftype == DEFTYPE_EQU && current->global)
		{
			uint8_t segment = 0;
			switch(current->ref.var.type)
			{
			case VAR_NONE:
				segment = 0; // absolute
				break;
			case VAR_SECTION:
				segment = output.section[current->ref.var.internal.section_index]->elf_symbol_index;
				break;
			case VAR_DEFINE:
				// not handling it
				// TODO
				continue;
			}
			omf_begin_record(OMF_PUBDEF80);
			omf_putbyte(segment);
			omf_putword(uint_get(current->ref.value));
			omf_putstring(current->name);
			omf_end_record();
		}
	}

	// LEDATA, relocations
	for(size_t section_index = 0; section_index < output.section_count; section_index++)
	{
		if(output.section[section_index]->format != SECTION_DATA)
			continue;

		section_t * relocations = output.section[section_index]->data.relocations;
		size_t relocation_index = 0;

		for(
			block_t * current = output.section[section_index]->data.first_block;
			current != NULL;
			current = current->next
		)
		{
			omf_begin_record(OMF_LEDATA80);
			omf_putbyte(output.section[section_index]->elf_symbol_index);
			// TODO: should we split this up?
			omf_putword(0); // offset
			for(uint16_t offset = 0; offset < current->size; offset++)
			{
				omf_putbyte(current->buffer[offset]);
			}
			omf_end_record();

			// TODO: group by type and size?
			for(; relocation_index < relocations->reloc.count
				&& relocations->reloc.relocations[relocation_index].offset < current->address + current->size; relocation_index++)
			{
				relocation_t * rel = &relocations->reloc.relocations[relocation_index];
				switch(rel->var.type)
				{
				case VAR_NONE:
					// TODO: should not happen
					break;
				case VAR_DEFINE:
					switch(rel->var.external->deftype)
					{
					case DEFTYPE_EQU:
						// TODO: should not happen
						break;
					case DEFTYPE_EXTERNAL:
						omf_begin_record(OMF_FIXUPC80); // external
						switch(rel->size)
						{
						case BITSIZE8:
							omf_putbyte(1);
							break;
						case BITSIZE16:
						default:
							omf_putbyte(3);
							break;
						// TODO: 3 for high byte
						}
						omf_putword(rel->var.external->elf_symbol_index);
						omf_putword(rel->offset);
						omf_end_record();
						break;
					case DEFTYPE_COMMON:
						// behaves like an intersegment relocation
						omf_begin_record(OMF_FIXUPB80); // intersegment
						omf_putbyte(rel->var.external->elf_symbol_index);
						switch(rel->size)
						{
						case BITSIZE8:
							omf_putbyte(1);
							break;
						case BITSIZE16:
						default:
							omf_putbyte(3);
							break;
						// TODO: 3 for high byte
						}
						omf_putword(rel->offset);
						omf_end_record();
						break;
					}
					break;
				case VAR_SECTION:
					if(rel->var.internal.section_index == section_index)
					{
						omf_begin_record(OMF_FIXUPA80); // intrasegment
						switch(rel->size)
						{
						case BITSIZE8:
							omf_putbyte(1);
							break;
						case BITSIZE16:
						default:
							omf_putbyte(3);
							break;
						// TODO: 3 for high byte
						}
						omf_putword(rel->offset);
						omf_end_record();
					}
					else
					{
						omf_begin_record(OMF_FIXUPB80); // intersegment
						omf_putbyte(output.section[rel->var.internal.section_index]->elf_symbol_index);
						switch(rel->size)
						{
						case BITSIZE8:
							omf_putbyte(1);
							break;
						case BITSIZE16:
						default:
							omf_putbyte(3);
							break;
						// TODO: 3 for high byte
						}
						omf_putword(rel->offset);
						omf_end_record();
					}
					break;
				}
			}
		}
	}

	reference_t start_symbol[1];
	label_lookup(entry_point_name, start_symbol);

	omf_begin_record(OMF_MODEND80);
	omf_putbyte(1); // main program
	if(start_symbol->var.type == VAR_SECTION)
	{
		omf_putbyte(output.section[start_symbol->var.internal.section_index]->elf_symbol_index);
	}
	else
	{
		omf_putbyte(0);
	}

	uint16_t start;
	start = uint_get(start_symbol->value);
	omf_putword(start);
	omf_end_record();

	omf_begin_record(OMF_EOF80);
	omf_end_record();
}

void omf86_generate(const char * module_name)
{
	omf_begin_record(OMF_THEADR);
	omf_putstring(module_name);
	omf_end_record();

	for(definition_t * current = globals; current != NULL; current = current->next)
	{
		if(current->imported.mode != ENTRY_NONE)
		{
			omf_begin_record(OMF_COMENT);
			omf_putbyte(0);
			omf_putbyte(0xA0);
			omf_putbyte(OMF_IMPDEF);
			omf_putbyte(current->imported.mode == ENTRY_BYORDINAL);
			omf_putstring(current->name);
			omf_putstring(current->imported.module);
			if(current->imported.mode == ENTRY_BYORDINAL)
				omf_putword(current->imported.ordinal);
			else
				omf_putstring(current->imported.name ? current->imported.name : "");
			omf_end_record();
		}

		if(current->exported.mode != ENTRY_NONE)
		{
			omf_begin_record(OMF_COMENT);
			omf_putbyte(0);
			omf_putbyte(0xA0);
			omf_putbyte(OMF_EXPDEF);
			uint8_t flags = 0x00;
			if(current->exported.mode == ENTRY_BYORDINAL)
			{
				flags |= OMF_EXPDEF_ORDINAL;
			}
			flags |= (current->exported.attributes & (OMF_EXPDEF_RESIDENT_NAME | OMF_EXPDEF_NODATA)) | (current->exported.parameter_count & 0x1F);
			omf_putbyte(flags);
			omf_putstring(current->exported.name ? current->exported.name : current->name);
			omf_putstring(current->exported.name ? current->name : "");
			if(current->exported.mode == ENTRY_BYORDINAL)
				omf_putword(current->exported.ordinal);
			omf_end_record();
		}
	}

	omf_begin_record(OMF_LNAMES);

	size_t name_index = 0;

	omf_putstring("");
	++name_index;

	for(size_t section_index = 0; section_index < output.section_count; section_index++)
	{
		if(output.section[section_index]->format != SECTION_DATA && output.section[section_index]->format != SECTION_ZERO_DATA)
			continue;

		omf_putstring(output.section[section_index]->name);
		output.section[section_index]->elf_string_offset = ++name_index;
		// TODO: class and overlay name
	}

	omf_end_record();

	size_t segment_index = 0;

	for(size_t section_index = 0; section_index < output.section_count; section_index++)
	{
		if(output.section[section_index]->format != SECTION_DATA && output.section[section_index]->format != SECTION_ZERO_DATA)
			continue;

		uint8_t bits = 0x00;
		if(output.section[section_index]->data.full_size <= 0x10000)
		{
			omf_begin_record(OMF_SEGDEF16);
			if(output.section[section_index]->data.full_size == 0x10000)
				bits |= 0x02;
		}
		else
		{
			omf_begin_record(OMF_SEGDEF32);
			if(output.section[section_index]->data.full_size == 0x100000000)
				bits |= 0x02;
		}

		if((output.section[section_index]->flags & SHF_USE32) != 0)
		{
			bits |= 0x01;
		}

		if((output.section[section_index]->flags & SHF_PRIVATE) != 0)
		{
			bits |= 0x00;
		}
		else if((output.section[section_index]->flags & SHF_PUBLIC) != 0)
		{
			bits |= 0x08;
		}
		else if((output.section[section_index]->flags & SHF_STACK) != 0)
		{
			bits |= 0x14;
		}
		else if((output.section[section_index]->flags & SHF_COMMON) != 0)
		{
			bits |= 0x18;
		}

		if(output.section[section_index]->align == 1)
		{
			bits |= 0x20;
		}
		else if(output.section[section_index]->align == 2)
		{
			bits |= 0x40;
		}
		else if(output.section[section_index]->align == 4) // TODO: old versions don't support this
		{
			bits |= 0xA0;
		}
		else if(output.section[section_index]->align <= 16)
		{
			bits |= 0x60;
		}
		else if(output.section[section_index]->align <= 4096) // TODO: might be 256
		{
			bits |= 0x80;
		}

		omf_putbyte(bits);

		omf_putoffset(output.section[section_index]->data.full_size);

		omf_putindex(output.section[section_index]->elf_string_offset);
		omf_putindex(1);
		omf_putindex(1);

		omf_end_record();

		output.section[section_index]->elf_symbol_index = ++segment_index;
	}

	// TODO: GRPDEF

	size_t external_index = 0;

	definition_t * start_symbol = NULL;

	for(definition_t * current = globals; current != NULL; current = current->next)
	{
		if(strcmp(current->name, entry_point_name) == 0)
		{
			start_symbol = current;
			continue;
		}

		switch(current->deftype)
		{
		case DEFTYPE_EXTERNAL:
			omf_begin_record(OMF_EXTDEF);
			omf_putstring(current->name);
			omf_putindex(0); // type
			omf_end_record();
			current->elf_symbol_index = ++external_index;
			break;
		case DEFTYPE_COMMON:
			omf_begin_record(OMF_COMDEF);
			omf_putstring(current->name);
			omf_putindex(0); // type
			uint32_t size = uint_get(current->size.value);
			if(!current->isfar)
			{
				omf_putbyte(0x62); // near
				omf_putnumber(size);
			}
			else
			{
				omf_putbyte(0x61); // far
				uint32_t count = uint_get(current->count.value);
				omf_putnumber(count);
				omf_putnumber(size / count);
			}
			omf_end_record();
			current->elf_symbol_index = ++external_index;
			break;
		case DEFTYPE_EQU:
			if(current->global)
			{
				// TODO: group by section?

				uint32_t value = uint_get(current->ref.value);
				if(value < 0x10000)
				{
					omf_begin_record(OMF_PUBDEF16);
				}
				else
				{
					omf_begin_record(OMF_PUBDEF32);
				}

				omf_putindex(0); // group
				if(current->ref.var.type == VAR_SECTION)
				{
					omf_putindex(output.section[current->ref.var.internal.section_index]->elf_symbol_index);
				}
				else
				{
					omf_putindex(0);
					omf_putword(0); // base frame
				}
				omf_putstring(current->name);
				omf_putoffset(value);
				omf_putindex(0); // type

				omf_end_record();
			}
			break;
		}
	}

	for(size_t section_index = 0; section_index < output.section_count; section_index++)
	{
		if(output.section[section_index]->format != SECTION_DATA)
			continue;

		section_t * relocations = output.section[section_index]->data.relocations;
		size_t relocation_index = 0;

		for(
			block_t * current = output.section[section_index]->data.first_block;
			current != NULL;
			current = current->next
		)
		{
			//for(uint32_t offset = 0; offset < output.section[section_index]->data.full_size; offset += 1024)
			for(uint32_t offset = 0; offset < current->size; offset += 1024)
			{
				// TODO: make sure relocations do not get cut up into two LEDATA records
				if(current->address + offset < 0x10000)
				{
					omf_begin_record(OMF_LEDATA16);
				}
				else
				{
					omf_begin_record(OMF_LEDATA32);
				}
				omf_putindex(output.section[section_index]->elf_symbol_index);
				omf_putoffset(current->address + offset);
				//for(uint16_t i = 0; i < 1024 && offset + i < output.section[section_index]->data.full_size; i++)
				for(uint16_t i = 0; i < 1024 && offset + i < current->size; i++)
				{
					omf_putbyte(current->buffer[offset + i]);
				}
				omf_end_record();

				bool fixups_started = false;

				for(; relocation_index < relocations->reloc.count
					&& relocations->reloc.relocations[relocation_index].offset < current->address + current->size
					&& relocations->reloc.relocations[relocation_index].offset < current->address + offset + 1024;
					relocation_index++)
				{
					relocation_t relocation = relocations->reloc.relocations[relocation_index];

					if(!fixups_started)
					{
						omf_begin_record(OMF_FIXUPP16); // TODO: OMF_FIXUPP32?
						fixups_started = true;
					}

					uint16_t word = 0x8000;
					word |= relocation.offset - (current->address + offset);

					switch(relocation.size)
					{
					case 1:
						break;
					case 2:
						if(relocation.var.segment_of)
							word |= 0x0800;
						else
							word |= 0x0400;
						break;
					case 4:
						word |= 0x2400;
						break;
					}

					if(!relocation.pc_relative)
						word |= 0x4000;

					// big endian word
					omf_putbyte(word >> 8);
					omf_putbyte(word);

					uint8_t data = 0;
					uint16_t frame;
					if(relocation.wrt_section == WRT_DEFAULT)
					{
						frame = -1;
						data |= 0x50;
					}
					else if(relocation.wrt_section == WRT_NONE)
					{
						// 8089 self-relative
						frame = -1;
						data |= 0x60;
					}
					else
					{
						frame = output.section[relocation.wrt_section]->elf_symbol_index;
						data |= 0x00;
					}
					uint16_t target;
					if(relocation.var.type == VAR_SECTION)
					{
						// SEGDEF
						target = output.section[relocation.var.internal.section_index]->elf_symbol_index;
						data |= 0x04;
					}
					else if(relocation.var.type == VAR_DEFINE)
					{
						// EXTDEF
						target = relocation.var.external->elf_symbol_index;
						data |= 0x06;
					}
					else
					{
						fprintf(stderr, "Absolute addresses not supported\n");
						// TODO: probably not a real relocation
					}

					omf_putbyte(data);
					if(frame != (uint16_t)-1)
						omf_putindex(frame);
					if(target != (uint16_t)-1)
						omf_putindex(target); // TODO
				}

				if(fixups_started)
				{
					omf_end_record();
				}
			}
		}
	}

	uint8_t mod_type = 0x80; // main not overlay
	if(start_symbol != NULL)
		mod_type |= 0x41;

	uint32_t start = 0;
	if(start_symbol != NULL)
	{
		start = uint_get(start_symbol->ref.value);
	}

	if(start_symbol == NULL || start < 0x10000)
	{
		omf_begin_record(OMF_MODEND16);
	}
	else
	{
		omf_begin_record(OMF_MODEND32);
	}

	omf_putbyte(mod_type);
	if(start_symbol != NULL)
	{
		omf_putbyte(0x50);
		omf_putindex(output.section[start_symbol->ref.var.internal.section_index]->elf_symbol_index);
		omf_putoffset(start);
	}

	omf_end_record();
}

void omf_generate(const char * module_name)
{
	switch(output.format)
	{
	case FORMAT_OMF80:
		omf80_generate(module_name);
		break;
	case FORMAT_OMF86:
		omf86_generate(module_name);
		break;
	default:
		break;
	}
}

