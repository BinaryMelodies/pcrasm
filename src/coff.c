
#include "asm.h"
#include "coff.h"
#include "elf.h"
#include "isa.h"

#define MIN(a, b) ((a) <= (b) ? (a) : (b))

// TODO: common with ELF
static inline void fwrite16(FILE * file, uint16_t value)
{
	switch(output.format == FORMAT_COFF ? elf_default_byte_order() : ELFDATA2LSB)
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
	switch(output.format == FORMAT_COFF ? elf_default_byte_order() : ELFDATA2LSB)
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
	switch(output.format == FORMAT_COFF ? elf_default_byte_order() : ELFDATA2LSB)
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

static inline void fwrite_padded(const char * ptr, size_t maxlen, FILE * file)
{
	fwrite(ptr, 1, MIN(strlen(ptr), maxlen), file);
	for(int i = strlen(ptr); i < maxlen; i++)
		fwrite8(output.file, 0);
}


void coff_generate(const char * input_filename)
{
	size_t section_count = 0;
	for(size_t section_index = 0; section_index < output.section_count; section_index ++)
	{
		if(output.section[section_index]->format != SECTION_DATA && output.section[section_index]->format != SECTION_ZERO_DATA)
		{
			assert(false); // section indexes must map 1-to-1
			continue;
		}

		output.section[section_index]->elf_section_index = ++section_count;
	}

	size_t section_data_offset;
	section_data_offset = 0x14 + 40 * section_count;

	for(size_t section_index = 0; section_index < output.section_count; section_index ++)
	{
		if(output.section[section_index]->format != SECTION_DATA && output.section[section_index]->format != SECTION_ZERO_DATA)
			continue;

		if(output.section[section_index]->align != 0)
			section_data_offset = align_to(section_data_offset, output.section[section_index]->align);
		output.section[section_index]->file_offset = section_data_offset;
		if((output.section[section_index]->flags & SHF_PROGBITS) != 0)
		{
			section_data_offset += output.section[section_index]->data.full_size;
		}

		section_t * relocations = output.section[section_index]->data.relocations;
		if(relocations->reloc.count > 0)
		{
			if(relocations->align != 0)
				section_data_offset = align_to(section_data_offset, relocations->align);
			relocations->file_offset = section_data_offset;
			section_data_offset += coff_relocation_size() * relocations->reloc.count;
		}
	}

	// collect symbols
	section_t symtab[1];
	section_init(symtab, NULL, SECTION_SYMTAB, (section_t) { });
	size_t symtab_entry_count = 0;

	// add a file symbol with a single auxiliary entry
	section_add_symbol(symtab, NULL);
	section_add_symbol(symtab, NULL);
	symtab_entry_count += 2;

	// add symbols for each section
	for(size_t i = 0; i < output.section_count; i ++)
	{
		// add a section symbol with a single auxiliary entry
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
			output.section[i]->elf_symbol_index = section_add_symbol(symtab, section);
			section_add_symbol(symtab, NULL);
			symtab_entry_count += 2;
		}
	}

	// add local symbols

	// TODO: local common symbols do not exist
	for(definition_t * current = globals; current != NULL; current = current->next)
	{
		if(!(current->global || current->deftype == DEFTYPE_EXTERNAL))
		{
			current->elf_symbol_index = section_add_symbol(symtab, current);
			symtab_entry_count ++;
		}
	}

	// add global symbols

	for(definition_t * current = globals; current != NULL; current = current->next)
	{
		if(current->global || current->deftype == DEFTYPE_EXTERNAL)
		{
			current->elf_symbol_index = section_add_symbol(symtab, current);
			symtab_entry_count ++;
		}
	}

	// TODO: create string table

	// file header

	// magic
	fwrite16(output.file, coff_magic_number());
	// nscns
	fwrite16(output.file, section_count);
	// timdat
	fwrite32(output.file, 0);
	// symptr
	fwrite32(output.file, section_data_offset);
	// nsyms
	fwrite32(output.file, symtab_entry_count);
	// opthdr
	fwrite16(output.file, 0);
	// flags
	fwrite16(output.file, coff_header_flags());

	// section headers

	for(size_t index = 0; index < output.section_count; index++)
	{
		if(output.section[index]->format != SECTION_DATA && output.section[index]->format != SECTION_ZERO_DATA)
			continue;

		// name
		fwrite_padded(output.section[index]->name, 8, output.file); // TODO: long names

		// paddr
		fwrite32(output.file, 0);
		// vaddr
		fwrite32(output.file, 0);
		// size
		fwrite32(output.file, output.section[index]->data.full_size);
		// scnptr
		fwrite32(output.file, output.section[index]->file_offset);
		// relptr
		fwrite32(output.file, output.section[index]->data.relocations->file_offset); // TODO
		// lnnoptr
		fwrite32(output.file, 0);
		// nreloc
		fwrite16(output.file, output.section[index]->data.relocations->reloc.count);
		// nlnno
		fwrite16(output.file, 0);

		// flags
		uint32_t flags = 0;

		if((output.section[index]->flags & SHF_NOBITS) != 0)
			flags |= 0x80;
		else if((output.section[index]->flags & SHF_EXECINSTR) != 0)
			flags |= 0x20;
		else
			flags |= 0x40;

		if(output.format != FORMAT_COFF)
		{
			if((output.section[index]->flags & SHF_EXECINSTR) != 0)
				flags |= 0x20000000;
			if((output.section[index]->flags & SHF_READ) != 0)
				flags |= 0x40000000;
			if((output.section[index]->flags & SHF_WRITE) != 0)
				flags |= 0x80000000;

			switch(output.section[index]->align)
			{
			case 0:
			case 1:
				flags |= 0x00100000;
				break;
			case 2:
				flags |= 0x00200000;
				break;
			case 4:
				flags |= 0x00300000;
				break;
			case 8:
				flags |= 0x00400000;
				break;
			case 0x10:
				flags |= 0x00500000;
				break;
			case 0x20:
				flags |= 0x00600000;
				break;
			case 0x40:
				flags |= 0x00700000;
				break;
			case 0x80:
				flags |= 0x00800000;
				break;
			case 0x100:
				flags |= 0x00900000;
				break;
			case 0x200:
				flags |= 0x00A00000;
				break;
			case 0x400:
				flags |= 0x00B00000;
				break;
			case 0x800:
				flags |= 0x00C00000;
				break;
			case 0x1000:
				flags |= 0x00D00000;
				break;
			case 0x2000:
				flags |= 0x00E00000;
				break;
			}
		}

		fwrite32(output.file, flags);
	}

	// section data

	for(size_t section_index = 0; section_index < output.section_count; section_index++)
	{
		if(output.section[section_index]->format != SECTION_DATA && output.section[section_index]->format != SECTION_ZERO_DATA)
			continue;

		fseek(output.file, output.section[section_index]->file_offset, SEEK_SET);

		if(output.section[section_index]->format == SECTION_DATA && output.section[section_index]->data.full_size > 0)
		{
			fwrite(output.section[section_index]->data.first_block->buffer, 1, output.section[section_index]->data.first_block->size, output.file);
		}

		section_t * relocations = output.section[section_index]->data.relocations;
		if(relocations->reloc.count > 0)
		{
			fseek(output.file, relocations->file_offset, SEEK_SET);

			for(size_t index = 0; index < relocations->reloc.count; index++)
			{
				relocation_t rel = relocations->reloc.relocations[index];
				if(rel.wrt_section != WRT_DEFAULT)
				{
					fprintf(stderr, "WRT relocations unsupported for format, ignoring frame\n");
				}

				uint32_t sym;
				switch(rel.var.type)
				{
				case VAR_NONE:
					sym = N_ABS;
					break;
				case VAR_SECTION:
					sym = output.section[rel.var.internal.section_index]->elf_symbol_index;
					break;
				case VAR_DEFINE:
					sym = rel.var.external->elf_symbol_index;
					break;
				}

				fwrite32(output.file, rel.offset);
				fwrite32(output.file, sym);
				if(coff_relocation_size() == 16)
				{
					fwrite32(output.file, uint_get(rel.addend));
				}
				int reltype = coff_get_relocation_type(rel);
				if(reltype == -1)
					fprintf(stderr, "Invalid relocation\n");
				fwrite16(output.file, reltype == -1 ? 0 : reltype);
				if(coff_relocation_size() == 16)
				{
					fwrite16(output.file, 0);
				}
			}
		}
	}

	// symbol data

	fseek(output.file, section_data_offset, SEEK_SET);

	for(size_t symbol_index = 0; symbol_index < symtab->symtab.count; symbol_index++)
	{
		definition_t * definition = symtab->symtab.symbols[symbol_index];

		const char * name;
		uint32_t value;
		uint16_t section;
		uint8_t sclass;
		uint8_t auxnum;
		if(definition == NULL)
		{
			name = ".file\0\0\0";
			value = 0;
			section = N_DEBUG;
			sclass = C_FILE;
			auxnum = 1;
		}
		else
		{
			name = definition->name;
			switch(definition->deftype)
			{
			case DEFTYPE_EXTERNAL:
				value = 0;
				section = N_UNDEF;
				sclass = C_EXT;
				auxnum = 0;
				break;
			case DEFTYPE_COMMON:
				// TODO: only definition->global is used
				value = uint_get(definition->ref.value);
				section = N_UNDEF;
				sclass = C_EXT;
				auxnum = 0;
				break;
			case DEFTYPE_EQU:
				switch(definition->ref.var.type)
				{
				case VAR_NONE:
					value = uint_get(definition->ref.value);
					section = N_ABS;
					sclass = definition->global ? C_EXT : C_STAT; // TODO: labels
					auxnum = 0;
					break;
				case VAR_DEFINE:
					// should not appear
					value = 0;
					section = N_UNDEF;
					sclass = C_EXT;
					auxnum = 0;
					break;
				case VAR_SECTION:
					if(symbol_index == output.section[definition->ref.var.internal.section_index]->elf_symbol_index)
					{
						// the section symbol
						value = 0;
						auxnum = 1;
					}
					else
					{
						// a defined symbol
						value = uint_get(definition->ref.value);
						auxnum = 0;
					}
					section = output.section[definition->ref.var.internal.section_index]->elf_section_index;
					sclass = definition->global ? C_EXT : C_STAT; // TODO: labels
					break;
				}
			}
		}

		fwrite_padded(name, 8, output.file); // TODO: long names
		fwrite32(output.file, value);
		fwrite16(output.file, section);
		fwrite16(output.file, 0); // type
		fwrite8(output.file, sclass);
		fwrite8(output.file, auxnum);

		if(auxnum != 0)
		{
			if(definition == NULL)
			{
				fwrite_padded(input_filename, 14, output.file);
				fwrite32(output.file, 0); // padding
			}
			else
			{
				fwrite32(output.file, output.section[section - 1]->data.full_size);
				fwrite16(output.file, output.section[section - 1]->data.relocations->reloc.count);
				fwrite16(output.file, 0); // line numbers

				fwrite32(output.file, 0); // padding
				fwrite32(output.file, 0); // padding
				fwrite16(output.file, 0); // padding
			}
		}

		symbol_index += auxnum;
	}

	// TODO: string table
	fwrite32(output.file, 0); // length
}

