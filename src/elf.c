
#include <assert.h>
#include <stdio.h>
#include "asm.h"
#include "elf.h"
#include "isa.h"

elf32_segments_t elf32_segments = ELF32_NO_SEGMENTS;

static inline void fwrite16(FILE * file, uint16_t value)
{
	switch(elf_default_byte_order())
	{
	case ELFDATA2LSB:
		fwrite16le(file, value);
		break;
	case ELFDATA2MSB:
		fwrite16be(file, value);
		break;
	default:
		assert(false);
	}
}

static inline void fwrite32(FILE * file, uint32_t value)
{
	switch(elf_default_byte_order())
	{
	case ELFDATA2LSB:
		fwrite32le(file, value);
		break;
	case ELFDATA2MSB:
		fwrite32be(file, value);
		break;
	default:
		assert(false);
	}
}

static inline void fwrite64(FILE * file, uint64_t value)
{
	switch(elf_default_byte_order())
	{
	case ELFDATA2LSB:
		fwrite64le(file, value);
		break;
	case ELFDATA2MSB:
		fwrite64be(file, value);
		break;
	default:
		assert(false);
	}
}

uint64_t elf_section_get_size(section_t * section)
{
	switch(section->format)
	{
	case SECTION_DATA:
	case SECTION_ZERO_DATA:
		return section->data.full_size;
	case SECTION_RELOC:
		return (section->reloc.count + section->reloc.extra_count) * (elf_backend_uses_rela() ? 3 : 2) * (output.format != FORMAT_ELF64 ? 4 : 8);
	case SECTION_SYMTAB:
		return section->symtab.count * (output.format != FORMAT_ELF64 ? 16 : 24);
	case SECTION_STRTAB:
		return section->strtab.size;
	}
	assert(false);
}

typedef struct elf_file_t
{
	size_t section_count;
	section_t ** sections;
} elf_file_t;

static size_t elf_file_add_section(elf_file_t * file, section_t * section)
{
	file->sections = file->sections == NULL ? malloc((file->section_count + 1) * sizeof(section_t *)) : realloc(file->sections, (file->section_count + 1) * sizeof(section_t *));
	file->sections[file->section_count] = section;
	if(section != NULL)
		section->elf_section_index = file->section_count;
	file->section_count ++;
	return file->section_count - 1;
}

static size_t elf_file_new_section(elf_file_t * file, const char * name, section_format_t format, section_t _default)
{
	section_t * section;
	section = malloc(sizeof(section_t));
	section_init(
		section,
		strdup(name),
		format,
		_default);
	return elf_file_add_section(file, section);
}

/*static size_t elf_file_locate_section(elf_file_t * file, const char * name, section_format_t format, section_t _default)
{
	for(size_t section_index = 0; section_index < file->section_count; section_index++)
	{
		section_t * section = file->sections[section_index];
		if(strcmp(section->name, name) == 0)
			return section_index;
	}
	return elf_file_new_section(file, name, format, _default);
}*/


void elf_generate(void)
{
	elf_file_t elffile[1] = { { } };

	elf_file_add_section(elffile, NULL);

	size_t section_count = output.section_count;
	for(size_t section_index = 0; section_index < section_count; section_index ++)
	{
		if(output.section[section_index]->format != SECTION_DATA && output.section[section_index]->format != SECTION_ZERO_DATA)
			continue;

		elf_file_add_section(elffile, output.section[section_index]);
		if(output.section[section_index]->data.relocations->reloc.count != 0)
		{
			elf_file_add_section(elffile, output.section[section_index]->data.relocations);
		}

		// add new sections for the segment bases
		if(elf32_segments == ELF32_SEGELF)
		{
			if(output.section[section_index]->format != SECTION_DATA && output.section[section_index]->format != SECTION_ZERO_DATA)
				continue;

			if((output.section[section_index]->flags & (SHF_PROGBITS | SHF_NOBITS)) != 0)
			{
				char * segment_name = malloc(strlen(output.section[section_index]->name) + 2);
				strcpy(segment_name, output.section[section_index]->name);
				strcat(segment_name, "!");
				section_t attributes;
				attributes.flags = output.section[section_index]->flags;
				attributes.align = output.section[section_index]->align;
				size_t segment_section = objfile_locate_section(&output, segment_name, output.section[section_index]->format, attributes);
				output.section[section_index]->segment_section = segment_section;
				elf_file_add_section(elffile, output.section[segment_section]);
			}
		}
	}

	// Symbol table

	size_t symtab = elf_file_new_section(elffile, ".symtab", SECTION_SYMTAB, section_attributes(SHF_SYMTAB, output.format != FORMAT_ELF64 ? 4 : 8));

	section_add_symbol(elffile->sections[symtab], NULL);

#if TARGET_X65
	// add mapping symbols, these should come at the very beginning

	for(size_t i = 0; i < output.section_count; i++)
	{
		if(output.section[i]->format == SECTION_DATA || output.section[i]->format == SECTION_ZERO_DATA)
		{
			bitsize_t abits = BITSIZE8, xbits = BITSIZE8;
			for(
				instruction_t * ins = output.section[i]->data.first_instruction;
				ins != NULL;
				ins = ins->next)
			{
				if(abits != ins->abits)
				{
					definition_t * mapping_symbol = malloc(sizeof(definition_t));
					memset(mapping_symbol, 0, sizeof(definition_t));
					mapping_symbol->name = strdup(ins->abits != BITSIZE16 ? "$mh" : "$ml");
					reference_set_ui(&mapping_symbol->ref, ins->code_offset);
					reference_clear(&mapping_symbol->size);
					reference_clear(&mapping_symbol->count);
					mapping_symbol->ref.var.type = VAR_SECTION;
					mapping_symbol->ref.var.internal.section_index = i;
					section_add_symbol(elffile->sections[symtab], mapping_symbol);

					abits = ins->abits;
				}

				if(xbits != ins->xbits)
				{
					definition_t * mapping_symbol = malloc(sizeof(definition_t));
					memset(mapping_symbol, 0, sizeof(definition_t));
					mapping_symbol->name = strdup(ins->xbits != BITSIZE16 ? "$xh" : "$xl");
					reference_set_ui(&mapping_symbol->ref, ins->code_offset);
					reference_clear(&mapping_symbol->size);
					reference_clear(&mapping_symbol->count);
					mapping_symbol->ref.var.type = VAR_SECTION;
					mapping_symbol->ref.var.internal.section_index = i;
					section_add_symbol(elffile->sections[symtab], mapping_symbol);

					xbits = ins->xbits;
				}
			}
		}
	}
#endif

	// add symbols for each section

	for(size_t i = 0; i < output.section_count; i++)
	{
		if(output.section[i]->format == SECTION_DATA || output.section[i]->format == SECTION_ZERO_DATA)
		{
			definition_t * section = malloc(sizeof(definition_t));
			memset(section, 0, sizeof(definition_t));
			section->name = output.section[i]->name;
			reference_clear(&section->ref);
			reference_clear(&section->size);
			reference_clear(&section->count);
			section->ref.var.type = VAR_SECTION;
			section->ref.var.internal.section_index = i;
			output.section[i]->elf_symbol_index = section_add_symbol(elffile->sections[symtab], section);
		}
	}

	// add local symbols

	for(definition_t * current = globals; current != NULL; current = current->next)
	{
		if(!(current->global || current->deftype == DEFTYPE_EXTERNAL))
		{
			current->elf_symbol_index = section_add_symbol(elffile->sections[symtab], current);

			// add segment symbols, one for each local symbol

			if(elf32_segments == ELF32_SEGELF)
			{
				char * segment_name = malloc(strlen(current->name) + 2);
				strcpy(segment_name, current->name);
				strcat(segment_name, "!");

				definition_t * symbol = definition_create(segment_name);
				symbol->global = current->global;
				symbol->deftype = current->deftype;
				symbol->ref.var = current->ref.var;
				if(symbol->ref.var.type == VAR_SECTION)
				{
					symbol->ref.var.internal.section_index = output.section[current->ref.var.internal.section_index]->segment_section;
				}
				current->elf_segelf_symbol_index = section_add_symbol(elffile->sections[symtab], symbol);
			}
		}
	}

	// add RetroLinker escaped symbols to refer to extended relocations

	if(elf32_segments == ELF32_RETROLINKER)
	{
		for(size_t section_index = 0; section_index < output.section_count; section_index++)
		{
			if(output.section[section_index]->format != SECTION_RELOC)
				continue;

			for(size_t relocation_index = 0; relocation_index < output.section[section_index]->reloc.count; relocation_index++)
			{
				relocation_t * rel = &output.section[section_index]->reloc.relocations[relocation_index];

				if(rel->var.segment_of)
				{
					const char * symname = NULL;
					switch(rel->var.type)
					{
					case VAR_NONE:
						assert(false);
						break;
					case VAR_SECTION:
						symname = output.section[rel->var.internal.section_index]->name;
						break;
					case VAR_DEFINE:
						symname = rel->var.external->name;
						break;
					}

					char * segment_name = malloc(strlen(symname) + 9);
					strcpy(segment_name, "$$SEGOF$");
					strcat(segment_name, symname);

					definition_t * symbol = definition_create(segment_name);
					rel->retrolinker_symbol_index = section_add_symbol(elffile->sections[symtab], symbol);
				}
				else if(rel->wrt_section != WRT_DEFAULT)
				{
					const char * symname = NULL;
					switch(rel->var.type)
					{
					case VAR_NONE:
						assert(false);
						break;
					case VAR_SECTION:
						symname = output.section[rel->var.internal.section_index]->name;
						break;
					case VAR_DEFINE:
						symname = rel->var.external->name;
						break;
					}
					const char * segname = output.section[rel->wrt_section]->name;

					char * segment_name = malloc(strlen(symname) + strlen(segname) + 11);
					strcpy(segment_name, "$$WRTSEG$");
					strcat(segment_name, symname);
					strcat(segment_name, "$");
					strcat(segment_name, segname);

					definition_t * symbol = definition_create(segment_name);
					rel->retrolinker_symbol_index = section_add_symbol(elffile->sections[symtab], symbol);
				}
			}
		}
	}

	// add global symbols

	size_t symtab_locals = elffile->sections[symtab]->symtab.count;

	for(definition_t * current = globals; current != NULL; current = current->next)
	{
		if(current->global || current->deftype == DEFTYPE_EXTERNAL)
		{
			current->elf_symbol_index = section_add_symbol(elffile->sections[symtab], current);

			// add segment symbols, one for each global symbol

			if(elf32_segments == ELF32_SEGELF)
			{
				char * segment_name = malloc(strlen(current->name) + 2);
				strcpy(segment_name, current->name);
				strcat(segment_name, "!");

				definition_t * symbol = definition_create(segment_name);
				symbol->global = current->global;
				symbol->deftype = current->deftype;
				symbol->ref.var = current->ref.var;
				if(symbol->ref.var.type == VAR_SECTION)
				{
					symbol->ref.var.internal.section_index = output.section[current->ref.var.internal.section_index]->segment_section;
				}
				current->elf_segelf_symbol_index = section_add_symbol(elffile->sections[symtab], symbol);
			}
		}
	}

	// String table
	size_t strtab = elf_file_new_section(elffile, ".strtab", SECTION_STRTAB, section_attributes(SHF_STRTAB, 1));

	section_add_string(elffile->sections[strtab], "");

	for(size_t index = 1; index < elffile->sections[symtab]->symtab.count; index++)
	{
		elffile->sections[symtab]->symtab.symbols[index]->elf_string_offset =
			section_add_string(elffile->sections[strtab], elffile->sections[symtab]->symtab.symbols[index]->name);
	}

	// Section header string table
	size_t shstrtab = elf_file_new_section(elffile, ".shstrtab", SECTION_STRTAB, section_attributes(SHF_STRTAB, 1));

	section_add_string(elffile->sections[shstrtab], "");

	for(size_t index = 1; index < elffile->section_count; index++)
	{
		elffile->sections[index]->elf_string_offset =
			section_add_string(elffile->sections[shstrtab], elffile->sections[index]->name);
	}

	// calculate offsets

	size_t section_data_offset;
	if(output.format != FORMAT_ELF64)
	{
		section_data_offset = 52;
	}
	else
	{
		section_data_offset = 64;
	}

	for(size_t index = 1; index < elffile->section_count; index++)
	{
		if(index == shstrtab || elffile->sections[index]->format == SECTION_RELOC)
			continue;
		if(elffile->sections[index]->align != 0)
			section_data_offset = align_to(section_data_offset, elffile->sections[index]->align);
		elffile->sections[index]->file_offset = section_data_offset;
		if((elffile->sections[index]->flags & SHF_NOBITS) == 0)
			section_data_offset += elf_section_get_size(elffile->sections[index]);
	}
	for(size_t index = 1; index < elffile->section_count; index++)
	{
		if(elffile->sections[index]->format != SECTION_RELOC)
			continue;
		if(elffile->sections[index]->align != 0)
			section_data_offset = align_to(section_data_offset, elffile->sections[index]->align);
		elffile->sections[index]->file_offset = section_data_offset;
		section_data_offset += elf_section_get_size(elffile->sections[index]);
	}
	elffile->sections[shstrtab]->file_offset = section_data_offset;
	section_data_offset += elf_section_get_size(elffile->sections[shstrtab]);

	// header

	fwrite("\x7F" "ELF", 4, 1, output.file);
	if(output.format != FORMAT_ELF64)
		fwrite8(output.file, ELFCLASS32);
	else
		fwrite8(output.file, ELFCLASS64);
	fwrite8(output.file, elf_default_byte_order());
	fwrite8(output.file, EV_CURRENT);
	fseek(output.file, 16L, SEEK_SET);
	// type
	fwrite16(output.file, ET_REL);
	// machine
	fwrite16(output.file, elf_machine_type());
	// version
	fwrite32(output.file, EV_CURRENT);
	if(output.format != FORMAT_ELF64)
	{
		// entry
		fwrite32(output.file, output.entry);
		// phoff
		fwrite32(output.file, 0);
		// shoff
		fwrite32(output.file, section_data_offset);
	}
	else
	{
		// entry
		fwrite64(output.file, output.entry);
		// phoff
		fwrite64(output.file, 0);
		// shoff
		fwrite64(output.file, section_data_offset);
	}
	// flags
	fwrite32(output.file, 0); // TODO
	// ehsize
	if(output.format != FORMAT_ELF64)
	{
		fwrite16(output.file, 52);
	}
	else
	{
		fwrite16(output.file, 64);
	}
	// phentsize
	fwrite16(output.file, 0);
	// phnum
	fwrite16(output.file, 0);
	// shentsize
	if(output.format != FORMAT_ELF64)
	{
		fwrite16(output.file, 40);
	}
	else
	{
		fwrite16(output.file, 64);
	}
	// shnum
	fwrite16(output.file, elffile->section_count);
	// shstrndx
	fwrite16(output.file, shstrtab);

	static const uint8_t zeroes[64] = { };

	for(size_t section_index = 1; section_index < elffile->section_count; section_index++)
	{
		if((elffile->sections[section_index]->flags & SHF_NOBITS) != 0)
			continue;

		fseek(output.file, elffile->sections[section_index]->file_offset, SEEK_SET);

		switch(elffile->sections[section_index]->format)
		{
		case SECTION_DATA:
			if(elffile->sections[section_index]->data.full_size > 0)
				fwrite(elffile->sections[section_index]->data.first_block->buffer, 1, elffile->sections[section_index]->data.first_block->size, output.file);
			break;
		case SECTION_ZERO_DATA:
			break;
		case SECTION_STRTAB:
			fwrite(elffile->sections[section_index]->strtab.buffer, 1, elffile->sections[section_index]->strtab.size, output.file);
			break;
		case SECTION_SYMTAB:
			for(size_t symbol_index = 0; symbol_index < elffile->sections[section_index]->symtab.count; symbol_index++)
			{
				definition_t * definition = elffile->sections[section_index]->symtab.symbols[symbol_index];

				if(definition == NULL)
				{
					if(output.format != FORMAT_ELF64)
					{
						fwrite(zeroes, 16, 1, output.file);
					}
					else
					{
						fwrite(zeroes, 24, 1, output.file);
					}
					continue;
				}

				// name
				fwrite32(output.file, definition->elf_string_offset);

				uint64_t value;
				uint16_t shndx;
				uint8_t info;
				uint64_t size = 0;

				size = uint_get(definition->size.value);

				switch(definition->deftype)
				{
				case DEFTYPE_EXTERNAL:
					value = 0;
					shndx = SHN_UNDEF;
					info = (STB_GLOBAL << 4) | STT_NOTYPE;
					break;
				case DEFTYPE_COMMON:
					value = uint_get(definition->ref.value);
					if(definition->global)
					{
						info = STB_GLOBAL << 4;
					}
					else
					{
						info = STB_LOCAL << 4;
					}

					shndx = SHN_COMMON;
					info |= STT_NOTYPE;
					break;
				case DEFTYPE_EQU:
					value = uint_get(definition->ref.value);
					if(definition->global)
					{
						info = STB_GLOBAL << 4;
					}
					else
					{
						info = STB_LOCAL << 4;
					}

					switch(definition->ref.var.type)
					{
					case VAR_NONE:
						shndx = SHN_ABS;
						info |= STT_NOTYPE;
						break;
					case VAR_DEFINE:
						// should not appear
						shndx = SHN_UNDEF;
						info |= STT_NOTYPE;
						break;
					case VAR_SECTION:
						shndx = output.section[definition->ref.var.internal.section_index]->elf_section_index;
						if(symbol_index == output.section[definition->ref.var.internal.section_index]->elf_symbol_index)
						{
							// this is the symbol corresponding to the section
							info |= STT_SECTION;
						}
						else
						{
							info |= STT_NOTYPE;
						}
						break;
					}
				}

				if(output.format != FORMAT_ELF64)
				{
					// value
					fwrite32(output.file, value);
					// size
					fwrite32(output.file, size);
				}
				// info
				fwrite8(output.file, info);
				// other
				fwrite8(output.file, 0);
				// shndx
				fwrite16(output.file, shndx);
				if(output.format == FORMAT_ELF64)
				{
					// value
					fwrite64(output.file, value);
					// size
					fwrite64(output.file, size);
				}
			}
			break;
		case SECTION_RELOC:
			for(size_t index = 0; index < elffile->sections[section_index]->reloc.count; index++)
			{
				relocation_t rel = elffile->sections[section_index]->reloc.relocations[index];

				if((elf32_segments == ELF32_NO_SEGMENTS || elf32_segments == ELF32_VMA_SEGMENTS) && rel.wrt_section != WRT_DEFAULT)
				{
					fprintf(stderr, "WRT relocations unsupported for format, ignoring frame\n");
				}

				if(elf32_segments == ELF32_NO_SEGMENTS && rel.var.segment_of)
				{
					fprintf(stderr, "SEG relocations unsupported for format\n");
				}

				uint32_t segsym = -1;
				if(elf32_segments == ELF32_SEGELF && rel.wrt_section != WRT_DEFAULT)
				{
					segsym = output.section[output.section[rel.wrt_section]->segment_section]->elf_symbol_index;
				}

				uint32_t sym;
				if(elf32_segments == ELF32_RETROLINKER && rel.retrolinker_symbol_index != 0)
					sym = rel.retrolinker_symbol_index;
				else switch(rel.var.type)
				{
				case VAR_NONE:
					sym = 0;
					break;
				case VAR_SECTION:
					if(elf32_segments == ELF32_SEGELF && rel.wrt_section == WRT_DEFAULT)
						segsym = output.section[output.section[rel.var.internal.section_index]->segment_section]->elf_symbol_index;
					if(elf32_segments == ELF32_SEGELF && rel.var.segment_of)
						sym = segsym;
					else
						sym = output.section[rel.var.internal.section_index]->elf_symbol_index;
					break;
				case VAR_DEFINE:
					if(elf32_segments == ELF32_SEGELF && rel.wrt_section == WRT_DEFAULT)
						segsym = rel.var.external->elf_segelf_symbol_index;
					if(elf32_segments == ELF32_SEGELF && rel.var.segment_of)
						sym = segsym;
					else
						sym = rel.var.external->elf_symbol_index;
					break;
				}

				if(output.format != FORMAT_ELF64)
				{
					// offset
					fwrite32(output.file, rel.offset);
					// info
					uint32_t info = (sym << 8) | elf_get_relocation_type(rel);
					fwrite32(output.file, info);
					// addend
					if(elf_backend_uses_rela())
					{
						fwrite32(output.file, uint_get(rel.addend));
					}

					if(elf32_segments == ELF32_SEGELF && !rel.var.segment_of && (rel.size == 2 || rel.size == 4))
					{
						fwrite32(output.file, rel.offset);
						uint32_t info = (segsym << 8) | (rel.size != 4 ? R_386_SUB16 : R_386_SUB32);
						fwrite32(output.file, info);
					}
				}
				else
				{
					// offset
					fwrite64(output.file, rel.offset);
					// info
					uint64_t info = ((uint64_t)sym << 32) | elf_get_relocation_type(rel);
					fwrite64(output.file, info);
					// addend
					if(elf_backend_uses_rela())
					{
						fwrite64(output.file, uint_get(rel.addend));
					}
				}
			}
			break;
		}
	}

	fseek(output.file, section_data_offset, SEEK_SET);

	// null section
	if(output.format != FORMAT_ELF64)
	{
		fwrite(zeroes, 40, 1, output.file);
	}
	else
	{
		fwrite(zeroes, 64, 1, output.file);
	}
	// remaining sections
	for(size_t index = 1; index < elffile->section_count; index++)
	{
		// name
		fwrite32(output.file, elffile->sections[index]->elf_string_offset);
		// type
		uint32_t sh_type;
		if((elffile->sections[index]->flags & SHF_STRTAB) != 0)
		{
			sh_type = SHT_STRTAB;
		}
		else if((elffile->sections[index]->flags & SHF_SYMTAB) != 0)
		{
			sh_type = SHT_SYMTAB;
		}
		else if((elffile->sections[index]->flags & SHF_RELOC) != 0)
		{
			sh_type = elf_backend_uses_rela() ? SHT_RELA : SHT_REL;
		}
		else if((elffile->sections[index]->flags & SHF_NOBITS) != 0)
		{
			sh_type = SHT_NOBITS;
		}
		else if((elffile->sections[index]->flags & SHF_PROGBITS) != 0)
		{
			sh_type = SHT_PROGBITS;
		}
		else
		{
			sh_type = SHT_NOTE; // TODO
		}
		fwrite32(output.file, sh_type);
		if(output.format != FORMAT_ELF64)
		{
			// flags
			fwrite32(output.file, elffile->sections[index]->flags & SHF_ELF_BITS);
			// addr
			fwrite32(output.file, 0);
			// offset
			fwrite32(output.file, elffile->sections[index]->file_offset);
			// size
			fwrite32(output.file, elf_section_get_size(elffile->sections[index]));
		}
		else
		{
			// flags
			fwrite64(output.file, elffile->sections[index]->flags & SHF_ELF_BITS);
			// addr
			fwrite64(output.file, 0);
			// offset
			fwrite64(output.file, elffile->sections[index]->file_offset);
			// size
			fwrite64(output.file, elf_section_get_size(elffile->sections[index]));
		}
		uint32_t link = 0, info = 0, entsize = 0;
		switch(elffile->sections[index]->format)
		{
		case SECTION_DATA:
		case SECTION_ZERO_DATA:
			break;
		case SECTION_STRTAB:
			break;
		case SECTION_SYMTAB:
			link = strtab;
			info = symtab_locals;
			entsize = (output.format != FORMAT_ELF64 ? 16 : 24);
			break;
		case SECTION_RELOC:
			link = symtab;
			info = index - 1; // section to which it applies, which precedes the reloc section
			entsize = (elf_backend_uses_rela() ? 3 : 2) * (output.format != FORMAT_ELF64 ? 4 : 8);
			break;
		}
		// link
		fwrite32(output.file, link);
		// info
		fwrite32(output.file, info);
		if(output.format != FORMAT_ELF64)
		{
			// addralign
			fwrite32(output.file, elffile->sections[index]->align);
			// entsize
			fwrite32(output.file, entsize);
		}
		else
		{
			// addralign
			fwrite64(output.file, elffile->sections[index]->align);
			// entsize
			fwrite64(output.file, entsize);
		}
	}
}

