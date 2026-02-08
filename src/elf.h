#ifndef _ELF_H
#define _ELF_H

#define SHF_WRITE 0x001
#define SHF_ALLOC 0x002
#define SHF_EXECINSTR 0x004
#define SHF_ELF_BITS 0xFFFFFFFF
// not actual ELF bit
#define SHF_READ 0x100000000L
#define SHF_NOBITS 0x200000000L
#define SHF_PROGBITS 0x400000000L
#define SHF_RELOC 0x800000000L
#define SHF_SYMTAB 0x1000000000L
#define SHF_STRTAB 0x2000000000L
#define SHF_PRIVATE 0x4000000000L
#define SHF_PUBLIC 0x8000000000L
#define SHF_COMMON 0x10000000000L
#define SHF_STACK 0x20000000000L
#define SHF_USE16 0x40000000000L
#define SHF_USE32 0x80000000000L

#define SHT_PROGBITS 1
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_RELA 4
#define SHT_NOTE 7
#define SHT_NOBITS 8
#define SHT_REL 9

#define SHN_UNDEF 0
#define SHN_ABS 0xFFF1
#define SHN_COMMON 0xFFF2

#define STB_LOCAL 0
#define STB_GLOBAL 1

#define STT_NOTYPE 0
#define STT_SECTION 3

#define ELFCLASS32 1
#define ELFCLASS64 2

#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

#define EV_CURRENT 1

#define ET_REL 1

#define EM_NONE 0
#define EM_386 3
#define EM_68K 4
#define EM_X86_64 62
#define EM_68HC11 70
#define EM_Z80 220
// various types of 65x chips
#define EM_MCS6502 254
#define EM_65816 257
#define EM_MOS 6502

#define R_386_8       22
#define R_386_16      20
#define R_386_32      1
#define R_386_PC8     23
#define R_386_PC16    21
#define R_386_PC32    2

#define R_386_SEG16   45
#define R_386_SUB16   46
#define R_386_SUB32   47
#define R_386_OZSEG16 80

#define R_X86_64_8    14
#define R_X86_64_16   12
#define R_X86_64_32   10
#define R_X86_64_64   1
#define R_X86_64_PC8  15
#define R_X86_64_PC16 13
#define R_X86_64_PC32 2
#define R_X86_64_PC64 24

#define R_68K_8    3
#define R_68K_16   2
#define R_68K_32   1
#define R_68K_PC8  6
#define R_68K_PC16 5
#define R_68K_PC32 4

// https://fossies.org/linux/binutils/include/elf/z80.h
#define R_Z80_8       1
#define R_Z80_8_DIS   2
#define R_Z80_16      4
#define R_Z80_24      5
#define R_Z80_32      6
#define R_Z80_8_PCREL 3

// https://llvm-mos.org/wiki/ELF_specification
#define R_MOS_IMM8        1
#define R_MOS_ADDR8       2
#define R_MOS_IMM16       16
#define R_MOS_ADDR16      3
#define R_MOS_ADDR24      7
#define R_MOS_PCREL_8     6
#define R_MOS_PCREL_16    12

#define R_MOS_ADDR16_HI   5
#define R_MOS_ADDR24_BANK 8

// https://fossies.org/linux/binutils/include/elf/m68hc11.h
#define R_M68HC11_8        1
#define R_M68HC11_LO8      3
#define R_M68HC11_HI8      2
#define R_M68HC11_16       5
#define R_M68HC11_24       11
#define R_M68HC11_32       6
#define R_M68HC11_PCREL_8  4
#define R_M68HC11_PCREL_16 8

typedef enum elf32_segments_t
{
	// segmentation not supported
	ELF32_NO_SEGMENTS,
	// LMA ≠ VMA scheme (OZSEG16)
	ELF32_VMA_SEGMENTS,
	// segelf scheme (SEG16)
	ELF32_SEGELF,
	// RetroLinker escapes
	ELF32_RETROLINKER,
} elf32_segments_t;

extern elf32_segments_t elf32_segments;

void elf_generate(void);

#endif // _ELF_H
