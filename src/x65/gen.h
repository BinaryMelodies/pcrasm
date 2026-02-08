
#ifndef CPU_ALL
# define CPU_ALL CPU_6502, CPU_LAST
#endif

#ifndef _PATTERN
# define ___PATTERN(__cpuname, __mnemonic) pattern_##__cpuname##_##__mnemonic
# define __PATTERN(__cpuname, __mnemonic) ___PATTERN(__cpuname, __mnemonic)
# define _PATTERN(__mnemonic) __PATTERN(_CPUNAME, __mnemonic)
#endif

static const instruction_format_t _PATTERN(adc)[_MODE_COUNT] =
{
	[MODE_ZPG_X_IND] = { 0x61 },
	[MODE_ZPG] = { 0x65 },
	[MODE_IMMA] = { 0x69 },
	[MODE_ABS] = { 0x6D },
	[MODE_ZPG_IND_Y] = { 0x71 },
	[MDOE_ZPG_X] = { 0x75 },
	[MODE_ABS_Y] = { 0x79 },
	[MODE_ABS_X] = { 0x7D },
#ifdef _INCLUDE_65C02
	[MODE_ZPG_IND_Z] = { 0x72 },
#endif
#ifdef _INCLUDE_65C816
	[MODE_STK] = { 0x63 },
	[MODE_LNG_IND] = { 0x67 },
	[MODE_LNG] = { 0x6F },
	[MODE_STK_IND_Y] = { 0x73 },
	[MODE_LNG_IND_Y] = { 0x77 },
	[MODE_LNG_X] = { 0x7F },
#endif
};

static const instruction_format_t _PATTERN(and)[_MODE_COUNT] =
{
	[MODE_ZPG_X_IND] = { 0x21 },
	[MODE_ZPG] = { 0x25 },
	[MODE_IMMA] = { 0x29 },
	[MODE_ABS] = { 0x2D },
	[MODE_ZPG_IND_Y] = { 0x31 },
	[MDOE_ZPG_X] = { 0x35 },
	[MODE_ABS_Y] = { 0x39 },
	[MODE_ABS_X] = { 0x3D },
#ifdef _INCLUDE_65C02
	[MODE_ZPG_IND_Z] = { 0x32 },
#endif
#ifdef _INCLUDE_65C816
	[MODE_STK] = { 0x23 },
	[MODE_LNG_IND] = { 0x27 },
	[MODE_LNG] = { 0x2F },
	[MODE_STK_IND_Y] = { 0x33 },
	[MODE_LNG_IND_Y] = { 0x37 },
	[MODE_LNG_X] = { 0x3F },
#endif
};

static const instruction_format_t _PATTERN(asl)[_MODE_COUNT] =
{
	[MODE_ZPG] = { 0x06 },
	[MODE_ABS] = { 0x0E },
	[MDOE_ZPG_X] = { 0x16 },
	[MODE_ABS_X] = { 0x1E },
	[MODE_A] = { 0x0A },
};

#ifdef _INCLUDE_65CE02
static const instruction_format_t _PATTERN(asr)[_MODE_COUNT] =
{
	[MODE_ZPG] = { 0x44 },
	[MDOE_ZPG_X] = { 0x54 },
	[MODE_A] = { 0x43 },
};
#endif

#ifdef _INCLUDE_65CE02
static const instruction_format_t _PATTERN(asw)[_MODE_COUNT] =
{
	[MODE_ABS] = { 0xCB },
};
#endif

#ifdef _INCLUDE_65CE02
static const instruction_format_t _PATTERN(aug)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x5C },
};
#endif

#ifdef _INCLUDE_WDC65C02
static const instruction_format_t _PATTERN(bbr_n)[_MODE_COUNT] =
{
	[MODE_ZPG_REL8] = { 0x0F },
};
#endif

#ifdef _INCLUDE_WDC65C02
static const instruction_format_t _PATTERN(bbs_n)[_MODE_COUNT] =
{
	[MODE_ZPG_REL8] = { 0x8F },
};
#endif

static const instruction_format_t _PATTERN(bcc)[_MODE_COUNT] =
{
	[MODE_REL8] = { 0x90 },
#ifdef _INCLUDE_65CE02
	[MODE_REL16] = { 0x93 },
#endif
};

static const instruction_format_t _PATTERN(bcs)[_MODE_COUNT] =
{
	[MODE_REL8] = { 0xB0 },
#ifdef _INCLUDE_65CE02
	[MODE_REL16] = { 0xB3 },
#endif
};

static const instruction_format_t _PATTERN(beq)[_MODE_COUNT] =
{
	[MODE_REL8] = { 0xF0 },
#ifdef _INCLUDE_65CE02
	[MODE_REL16] = { 0xF3 },
#endif
};

static const instruction_format_t _PATTERN(bit)[_MODE_COUNT] =
{
	[MODE_ZPG] = { 0x24 },
	[MODE_ABS] = { 0x2C },
#ifdef _INCLUDE_65C02
	[MODE_IMMA] = { 0x89 },
	[MDOE_ZPG_X] = { 0x34 },
	[MODE_ABS_X] = { 0x3C },
#endif
};

static const instruction_format_t _PATTERN(bmi)[_MODE_COUNT] =
{
	[MODE_REL8] = { 0x30 },
#ifdef _INCLUDE_65CE02
	[MODE_REL16] = { 0x33 },
#endif
};

static const instruction_format_t _PATTERN(bne)[_MODE_COUNT] =
{
	[MODE_REL8] = { 0xD0 },
#ifdef _INCLUDE_65CE02
	[MODE_REL16] = { 0xD3 },
#endif
};

static const instruction_format_t _PATTERN(bpl)[_MODE_COUNT] =
{
	[MODE_REL8] = { 0x10 },
#ifdef _INCLUDE_65CE02
	[MODE_REL16] = { 0x13 },
#endif
};

static const instruction_format_t _PATTERN(bra)[_MODE_COUNT] =
{
#ifdef _INCLUDE_65C02
	[MODE_REL8] = { 0x80 },
#endif
#ifdef _INCLUDE_65CE02
	[MODE_REL16] = { 0x83 },
#endif
};

static const instruction_format_t _PATTERN(brk)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x00 },
};

#ifdef _INCLUDE_65C816
static const instruction_format_t _PATTERN(brl)[_MODE_COUNT] =
{
	[MODE_REL16] = { 0x82 },
};
#endif

#ifdef _INCLUDE_65CE02
static const instruction_format_t _PATTERN(bsr)[_MODE_COUNT] =
{
	[MODE_REL16] = { 0x63 },
};
#endif

static const instruction_format_t _PATTERN(bvc)[_MODE_COUNT] =
{
	[MODE_REL8] = { 0x50 },
#ifdef _INCLUDE_65CE02
	[MODE_REL16] = { 0x53 },
#endif
};

static const instruction_format_t _PATTERN(bvs)[_MODE_COUNT] =
{
	[MODE_REL8] = { 0x70 },
#ifdef _INCLUDE_65CE02
	[MODE_REL16] = { 0x73 },
#endif
};

static const instruction_format_t _PATTERN(clc)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x18 },
};

static const instruction_format_t _PATTERN(cld)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0xD8 },
};

#ifdef _INCLUDE_65CE02
static const instruction_format_t _PATTERN(cle)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x02 },
};
#endif

static const instruction_format_t _PATTERN(cli)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x58 },
};

static const instruction_format_t _PATTERN(clv)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0xB8 },
};

static const instruction_format_t _PATTERN(cmp)[_MODE_COUNT] =
{
	[MODE_ZPG_X_IND] = { 0xC1 },
	[MODE_ZPG] = { 0xC5 },
	[MODE_IMMA] = { 0xC9 },
	[MODE_ABS] = { 0xCD },
	[MODE_ZPG_IND_Y] = { 0xD1 },
	[MDOE_ZPG_X] = { 0xD5 },
	[MODE_ABS_Y] = { 0xD9 },
	[MODE_ABS_X] = { 0xDD },
#ifdef _INCLUDE_65C02
	[MODE_ZPG_IND_Z] = { 0xD2 },
#endif
#ifdef _INCLUDE_65C816
	[MODE_STK] = { 0xC3 },
	[MODE_LNG_IND] = { 0xC7 },
	[MODE_LNG] = { 0xCF },
	[MODE_STK_IND_Y] = { 0xD3 },
	[MODE_LNG_IND_Y] = { 0xD7 },
	[MODE_LNG_X] = { 0xDF },
#endif
};

#ifdef _INCLUDE_65C816
static const instruction_format_t _PATTERN(cop)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x02 },
};
#endif

static const instruction_format_t _PATTERN(cpx)[_MODE_COUNT] =
{
	[MODE_ZPG] = { 0xE4 },
	[MODE_IMMX] = { 0xE0 },
	[MODE_ABS] = { 0xEC },
};

static const instruction_format_t _PATTERN(cpy)[_MODE_COUNT] =
{
	[MODE_ZPG] = { 0xC4 },
	[MODE_IMMX] = { 0xC0 },
	[MODE_ABS] = { 0xCC },
};

#ifdef _INCLUDE_65CE02
static const instruction_format_t _PATTERN(cpz)[_MODE_COUNT] =
{
	[MODE_ZPG] = { 0xD4 },
	[MODE_IMMX] = { 0xC2 },
	[MODE_ABS] = { 0xDC },
};
#endif

static const instruction_format_t _PATTERN(dec)[_MODE_COUNT] =
{
	[MODE_ZPG] = { 0xC6 },
	[MODE_ABS] = { 0xCE },
	[MDOE_ZPG_X] = { 0xD6 },
	[MODE_ABS_X] = { 0xDE },
#ifdef _INCLUDE_65C02
	[MODE_A] = { 0x3A },
#endif
};

#ifdef _INCLUDE_65CE02
static const instruction_format_t _PATTERN(dew)[_MODE_COUNT] =
{
	[MODE_ABS] = { 0xC3 },
};
#endif

static const instruction_format_t _PATTERN(dex)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0xCA },
};

static const instruction_format_t _PATTERN(dey)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x88 },
};

#ifdef _INCLUDE_65CE02
static const instruction_format_t _PATTERN(dez)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x3B },
};
#endif

static const instruction_format_t _PATTERN(eor)[_MODE_COUNT] =
{
	[MODE_ZPG_X_IND] = { 0x41 },
	[MODE_ZPG] = { 0x45 },
	[MODE_IMMA] = { 0x49 },
	[MODE_ABS] = { 0x4D },
	[MODE_ZPG_IND_Y] = { 0x51 },
	[MDOE_ZPG_X] = { 0x55 },
	[MODE_ABS_Y] = { 0x59 },
	[MODE_ABS_X] = { 0x5D },
#ifdef _INCLUDE_65C02
	[MODE_ZPG_IND_Z] = { 0x52 },
#endif
#ifdef _INCLUDE_65C816
	[MODE_STK] = { 0x43 },
	[MODE_LNG_IND] = { 0x47 },
	[MODE_LNG] = { 0x4F },
	[MODE_STK_IND_Y] = { 0x53 },
	[MODE_LNG_IND_Y] = { 0x57 },
	[MODE_LNG_X] = { 0x5F },
#endif
};

static const instruction_format_t _PATTERN(inc)[_MODE_COUNT] =
{
	[MODE_ZPG] = { 0xE6 },
	[MODE_ABS] = { 0xEE },
	[MDOE_ZPG_X] = { 0xF6 },
	[MODE_ABS_X] = { 0xFE },
#ifdef _INCLUDE_65C02
	[MODE_A] = { 0x1A },
#endif
};

#ifdef _INCLUDE_65CE02
static const instruction_format_t _PATTERN(inw)[_MODE_COUNT] =
{
	[MODE_ABS] = { 0xE3 },
};
#endif

static const instruction_format_t _PATTERN(inx)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0xE8 },
};

static const instruction_format_t _PATTERN(iny)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0xC8 },
};

#ifdef _INCLUDE_65CE02
static const instruction_format_t _PATTERN(inz)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x1B },
};
#endif

#ifdef _INCLUDE_65C816
static const instruction_format_t _PATTERN(jml)[_MODE_COUNT] =
{
	[MODE_LNG] = { 0x5C },
	[MODE_ABS_LNG_IND] = { 0xDC },
};
#endif

static const instruction_format_t _PATTERN(jmp)[_MODE_COUNT] =
{
	[MODE_ABS] = { 0x4C },
	[MODE_ABS_IND] = { 0x6C },
#ifdef _INCLUDE_65C02
	[MODE_ABS_X_IND] = { 0x7C },
#endif
#ifdef _INCLUDE_65C816
	[MODE_LNG] = { 0x5C }, // alternative syntax
	[MODE_ABS_LNG_IND] = { 0xDC }, // alternative syntax
#endif
};

#ifdef _INCLUDE_65C816
static const instruction_format_t _PATTERN(jsl)[_MODE_COUNT] =
{
	[MODE_LNG] = { 0x22 },
};
#endif

static const instruction_format_t _PATTERN(jsr)[_MODE_COUNT] =
{
	[MODE_ABS] = { 0x20 },
#ifdef _INCLUDE_65CE02
	[MODE_ABS_IND] = { 0x23 },
	[MODE_ABS_X_IND] = { 0x22 },
#endif
#ifdef _INCLUDE_65C816
	[MODE_ABS_X_IND] = { 0xFC }, // alternative syntax
	[MODE_LNG] = { 0x22 }, // alternative syntax
#endif
};

static const instruction_format_t _PATTERN(lda)[_MODE_COUNT] =
{
	[MODE_ZPG_X_IND] = { 0xA1 },
	[MODE_ZPG] = { 0xA5 },
	[MODE_IMMA] = { 0xA9 },
	[MODE_ABS] = { 0xAD },
	[MODE_ZPG_IND_Y] = { 0xB1 },
	[MDOE_ZPG_X] = { 0xB5 },
	[MODE_ABS_Y] = { 0xB9 },
	[MODE_ABS_X] = { 0xBD },
#ifdef _INCLUDE_65C02
	[MODE_ZPG_IND_Z] = { 0xB2 },
#endif
#ifdef _INCLUDE_65CE02
	[MODE_STK_IND_Y] = { 0xE3 },
#endif
#ifdef _INCLUDE_65C816
	[MODE_STK] = { 0xA3 },
	[MODE_LNG_IND] = { 0xA7 },
	[MODE_LNG] = { 0xAF },
	[MODE_STK_IND_Y] = { 0xB3 },
	[MODE_LNG_IND_Y] = { 0xB7 },
	[MODE_LNG_X] = { 0xBF },
#endif
};

static const instruction_format_t _PATTERN(ldx)[_MODE_COUNT] =
{
	[MODE_ZPG] = { 0xA6 },
	[MODE_IMMX] = { 0xA2 },
	[MODE_ABS] = { 0xAE },
	[MODE_ABS_Y] = { 0xBE },
	[MODE_ZPG_Y] = { 0xB6 },
};

static const instruction_format_t _PATTERN(ldy)[_MODE_COUNT] =
{
	[MODE_ZPG] = { 0xA4 },
	[MODE_IMMX] = { 0xA0 },
	[MODE_ABS] = { 0xAC },
	[MDOE_ZPG_X] = { 0xB4 },
	[MODE_ABS_X] = { 0xBC },
};

#ifdef _INCLUDE_65CE02
static const instruction_format_t _PATTERN(ldz)[_MODE_COUNT] =
{
	[MODE_IMMX] = { 0xA3 },
	[MODE_ABS] = { 0xAB },
	[MODE_ABS_X] = { 0xBB },
};
#endif

static const instruction_format_t _PATTERN(lsr)[_MODE_COUNT] =
{
	[MODE_ZPG] = { 0x46 },
	[MODE_ABS] = { 0x4E },
	[MDOE_ZPG_X] = { 0x56 },
	[MODE_ABS_X] = { 0x5E },
	[MODE_A] = { 0x4A },
};

#ifdef _INCLUDE_65C816
static const instruction_format_t _PATTERN(mvn)[_MODE_COUNT] =
{
	[MODE_IMM_IMM] = { 0x54 },
};
#endif

#ifdef _INCLUDE_65C816
static const instruction_format_t _PATTERN(mvp)[_MODE_COUNT] =
{
	[MODE_IMM_IMM] = { 0x44 },
};
#endif

#ifdef _INCLUDE_65CE02
static const instruction_format_t _PATTERN(neg)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x42 },
};
#endif

static const instruction_format_t _PATTERN(nop)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0xEA },
};

static const instruction_format_t _PATTERN(ora)[_MODE_COUNT] =
{
	[MODE_ZPG_X_IND] = { 0x01 },
	[MODE_ZPG] = { 0x05 },
	[MODE_IMMA] = { 0x09 },
	[MODE_ABS] = { 0x0D },
	[MODE_ZPG_IND_Y] = { 0x11 },
	[MDOE_ZPG_X] = { 0x15 },
	[MODE_ABS_Y] = { 0x19 },
	[MODE_ABS_X] = { 0x1D },
#ifdef _INCLUDE_65C02
	[MODE_ZPG_IND_Z] = { 0x12 },
#endif
#ifdef _INCLUDE_65C816
	[MODE_STK] = { 0x03 },
	[MODE_LNG_IND] = { 0x07 },
	[MODE_LNG] = { 0x0F },
	[MODE_STK_IND_Y] = { 0x13 },
	[MODE_LNG_IND_Y] = { 0x17 },
	[MODE_LNG_X] = { 0x1F },
#endif
};

#ifdef _INCLUDE_65C816
static const instruction_format_t _PATTERN(pea)[_MODE_COUNT] =
{
	[MODE_ABS] = { 0x54 },
};
#endif

#ifdef _INCLUDE_65C816
static const instruction_format_t _PATTERN(pei)[_MODE_COUNT] =
{
	[MODE_ZPG] = { 0xD4 },
};
#endif

#ifdef _INCLUDE_65C816
static const instruction_format_t _PATTERN(per)[_MODE_COUNT] =
{
	[MODE_REL16] = { 0x62 },
};
#endif

static const instruction_format_t _PATTERN(pha)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x48 },
};

#ifdef _INCLUDE_65C816
static const instruction_format_t _PATTERN(phb)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x8B },
};
#endif

#ifdef _INCLUDE_65C816
static const instruction_format_t _PATTERN(phd)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x0B },
};
#endif

#ifdef _INCLUDE_65C816
static const instruction_format_t _PATTERN(phk)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x4B },
};
#endif

static const instruction_format_t _PATTERN(php)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x08 },
};

#ifdef _INCLUDE_65CE02
static const instruction_format_t _PATTERN(phw)[_MODE_COUNT] =
{
	[MODE_ABS] = { 0xFC },
	[MODE_IMM16] = { 0xF4 },
};
#endif

#ifdef _INCLUDE_65C02
static const instruction_format_t _PATTERN(phx)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0xDA },
};
#endif

#ifdef _INCLUDE_65C02
static const instruction_format_t _PATTERN(phy)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x5A },
};
#endif

#ifdef _INCLUDE_65CE02
static const instruction_format_t _PATTERN(phz)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0xDB },
};
#endif

static const instruction_format_t _PATTERN(pla)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x68 },
};

#ifdef _INCLUDE_65C816
static const instruction_format_t _PATTERN(plb)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0xAB },
};
#endif

#ifdef _INCLUDE_65C816
static const instruction_format_t _PATTERN(pld)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x2B },
};
#endif

static const instruction_format_t _PATTERN(plp)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x28 },
};

#ifdef _INCLUDE_65C02
static const instruction_format_t _PATTERN(plx)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0xFA },
};
#endif

#ifdef _INCLUDE_65C02
static const instruction_format_t _PATTERN(ply)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x7A },
};
#endif

#ifdef _INCLUDE_65CE02
static const instruction_format_t _PATTERN(plz)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0xFB },
};
#endif

#ifdef _INCLUDE_65C816
static const instruction_format_t _PATTERN(rep)[_MODE_COUNT] =
{
	[MODE_IMM8] = { 0xC2 },
};
#endif

#ifdef _INCLUDE_WDC65C02
static const instruction_format_t _PATTERN(rmb_n)[_MODE_COUNT] =
{
	[MODE_ZPG] = { 0x07 },
};
#endif

static const instruction_format_t _PATTERN(rol)[_MODE_COUNT] =
{
	[MODE_ZPG] = { 0x26 },
	[MODE_ABS] = { 0x2E },
	[MDOE_ZPG_X] = { 0x36 },
	[MODE_ABS_X] = { 0x3E },
	[MODE_A] = { 0x2A },
};

static const instruction_format_t _PATTERN(ror)[_MODE_COUNT] =
{
	[MODE_ZPG] = { 0x66 },
	[MODE_ABS] = { 0x6E },
	[MDOE_ZPG_X] = { 0x76 },
	[MODE_ABS_X] = { 0x7E },
	[MODE_A] = { 0x6A },
};

#ifdef _INCLUDE_65CE02
static const instruction_format_t _PATTERN(row)[_MODE_COUNT] =
{
	[MODE_ABS] = { 0xEB },
};
#endif

static const instruction_format_t _PATTERN(rti)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x40 },
};

#ifdef _INCLUDE_65C816
static const instruction_format_t _PATTERN(rtl)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x6B },
};
#endif

static const instruction_format_t _PATTERN(rts)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x60 },
#ifdef _INCLUDE_65CE02
	[MODE_IMM8] = { 0x62 },
#endif
};

static const instruction_format_t _PATTERN(sbc)[_MODE_COUNT] =
{
	[MODE_ZPG_X_IND] = { 0xE1 },
	[MODE_ZPG] = { 0xE5 },
	[MODE_IMMA] = { 0xE9 },
	[MODE_ABS] = { 0xED },
	[MODE_ZPG_IND_Y] = { 0xF1 },
	[MDOE_ZPG_X] = { 0xF5 },
	[MODE_ABS_Y] = { 0xF9 },
	[MODE_ABS_X] = { 0xFD },
#ifdef _INCLUDE_65C02
	[MODE_ZPG_IND_Z] = { 0xF2 },
#endif
#ifdef _INCLUDE_65C816
	[MODE_STK] = { 0xE3 },
	[MODE_LNG_IND] = { 0xE7 },
	[MODE_LNG] = { 0xEF },
	[MODE_STK_IND_Y] = { 0xF3 },
	[MODE_LNG_IND_Y] = { 0xF7 },
	[MODE_LNG_X] = { 0xFF },
#endif
};

static const instruction_format_t _PATTERN(sec)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x38 },
};

static const instruction_format_t _PATTERN(sed)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0xF8 },
};

#ifdef _INCLUDE_65CE02
static const instruction_format_t _PATTERN(see)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x03 },
};
#endif

static const instruction_format_t _PATTERN(sei)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x78 },
};

#ifdef _INCLUDE_65C816
static const instruction_format_t _PATTERN(sep)[_MODE_COUNT] =
{
	[MODE_IMM8] = { 0xE2 },
};
#endif

#ifdef _INCLUDE_WDC65C02
static const instruction_format_t _PATTERN(smb_n)[_MODE_COUNT] =
{
	[MODE_ZPG] = { 0x87 },
};
#endif

static const instruction_format_t _PATTERN(sta)[_MODE_COUNT] =
{
	[MODE_ZPG_X_IND] = { 0x81 },
	[MODE_ZPG] = { 0x85 },
	[MODE_ABS] = { 0x8D },
	[MODE_ZPG_IND_Y] = { 0x91 },
	[MDOE_ZPG_X] = { 0x95 },
	[MODE_ABS_Y] = { 0x99 },
	[MODE_ABS_X] = { 0x9D },
#ifdef _INCLUDE_65C02
	[MODE_ZPG_IND_Z] = { 0x92 },
#endif
#ifdef _INCLUDE_65CE02
	[MODE_STK_IND_Y] = { 0x82 },
#endif
#ifdef _INCLUDE_65C816
	[MODE_STK] = { 0x83 },
	[MODE_LNG_IND] = { 0x87 },
	[MODE_LNG] = { 0x8F },
	[MODE_STK_IND_Y] = { 0x93 },
	[MODE_LNG_IND_Y] = { 0x97 },
	[MODE_LNG_X] = { 0x9F },
#endif
};

static const instruction_format_t _PATTERN(stp)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0xDB },
};

static const instruction_format_t _PATTERN(stx)[_MODE_COUNT] =
{
	[MODE_ZPG] = { 0x86 },
	[MODE_ABS] = { 0x8E },
#ifdef _INCLUDE_65CE02
	[MODE_ABS_Y] = { 0x9B },
#endif
	[MODE_ZPG_Y] = { 0x96 },
};

static const instruction_format_t _PATTERN(sty)[_MODE_COUNT] =
{
	[MODE_ZPG] = { 0x84 },
	[MODE_ABS] = { 0x8C },
	[MDOE_ZPG_X] = { 0x94 },
#ifdef _INCLUDE_65CE02
	[MODE_ABS_X] = { 0x8B },
#endif
};

#ifdef _INCLUDE_65C02
static const instruction_format_t _PATTERN(stz)[_MODE_COUNT] =
{
	[MODE_ZPG] = { 0x64 },
	[MODE_ABS] = { 0x9C },
	[MDOE_ZPG_X] = { 0x74 },
	[MODE_ABS_X] = { 0x9E },
};
#endif

#ifdef _INCLUDE_65CE02
static const instruction_format_t _PATTERN(tab)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x5B },
};
#endif

static const instruction_format_t _PATTERN(tax)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0xAA },
};

static const instruction_format_t _PATTERN(tay)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0xA8 },
};

#ifdef _INCLUDE_65CE02
static const instruction_format_t _PATTERN(taz)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x4B },
};
#endif

#ifdef _INCLUDE_65CE02
static const instruction_format_t _PATTERN(tba)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x7B },
};
#endif

#ifdef _INCLUDE_65C816
static const instruction_format_t _PATTERN(tcd)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x5B },
};
#endif

#ifdef _INCLUDE_65C816
static const instruction_format_t _PATTERN(tcs)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x1B },
};
#endif

#ifdef _INCLUDE_65C816
static const instruction_format_t _PATTERN(tdc)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x7B },
};
#endif

#ifdef _INCLUDE_65C02
static const instruction_format_t _PATTERN(trb)[_MODE_COUNT] =
{
	[MODE_ZPG] = { 0x14 },
	[MODE_ABS] = { 0x1C },
};
#endif

#ifdef _INCLUDE_65C02
static const instruction_format_t _PATTERN(tsb)[_MODE_COUNT] =
{
	[MODE_ZPG] = { 0x04 },
	[MODE_ABS] = { 0x0C },
};
#endif

#ifdef _INCLUDE_65C816
static const instruction_format_t _PATTERN(tsc)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x3B },
};
#endif

static const instruction_format_t _PATTERN(tsx)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0xBA },
};

#ifdef _INCLUDE_65CE02
static const instruction_format_t _PATTERN(tsy)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x0B },
};
#endif

static const instruction_format_t _PATTERN(txa)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x8A },
};

static const instruction_format_t _PATTERN(txs)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x9A },
};

#ifdef _INCLUDE_65C816
static const instruction_format_t _PATTERN(txy)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x9B },
};
#endif

static const instruction_format_t _PATTERN(tya)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x98 },
};

#ifdef _INCLUDE_65CE02
static const instruction_format_t _PATTERN(tys)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x2B },
};
#endif

#ifdef _INCLUDE_65C816
static const instruction_format_t _PATTERN(tyx)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0xBB },
};
#endif

#ifdef _INCLUDE_65CE02
static const instruction_format_t _PATTERN(tza)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x6B },
};
#endif

#ifdef _INCLUDE_65C816
static const instruction_format_t _PATTERN(wai)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0xCB },
};
#endif

#ifdef _INCLUDE_65C816
static const instruction_format_t _PATTERN(wdm)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0x42 },
};
#endif

#ifdef _INCLUDE_65C816
static const instruction_format_t _PATTERN(xba)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0xEB },
};
#endif

#ifdef _INCLUDE_65C816
static const instruction_format_t _PATTERN(xce)[_MODE_COUNT] =
{
	[MODE_IMPL] = { 0xFB },
};
#endif

#define ___PATTERNS(__cpuname) x65_##__cpuname##_patterns
#define __PATTERNS(__cpuname) ___PATTERNS(__cpuname)
#define _PATTERNS __PATTERNS(_CPUNAME)

static const instruction_format_t * _PATTERNS[] =
{
	[MNEM_ADC] = _PATTERN(adc),
	[MNEM_AND] = _PATTERN(and),
	[MNEM_ASL] = _PATTERN(asl),
#ifdef _INCLUDE_65CE02
	[MNEM_ASR] = _PATTERN(asr),
	[MNEM_ASW] = _PATTERN(asw),
	[MNEM_AUG] = _PATTERN(aug),
#endif
#ifdef _INCLUDE_WDC65C02
	[MNEM_BBR] = _PATTERN(bbr_n),
	[MNEM_BBS] = _PATTERN(bbs_n),
#endif
	[MNEM_BCC] = _PATTERN(bcc),
	[MNEM_BCS] = _PATTERN(bcs),
	[MNEM_BEQ] = _PATTERN(beq),
	[MNEM_BIT] = _PATTERN(bit),
	[MNEM_BMI] = _PATTERN(bmi),
	[MNEM_BNE] = _PATTERN(bne),
	[MNEM_BPL] = _PATTERN(bpl),
#if defined _INCLUDE_65C02 || defined _INCLUDE_65CE02
	[MNEM_BRA] = _PATTERN(bra),
#endif
	[MNEM_BRK] = _PATTERN(brk),
#ifdef _INCLUDE_65C816
	[MNEM_BRL] = _PATTERN(brl),
#endif
#ifdef _INCLUDE_65CE02
	[MNEM_BSR] = _PATTERN(bsr),
#endif
	[MNEM_BVC] = _PATTERN(bvc),
	[MNEM_BVS] = _PATTERN(bvs),
	[MNEM_CLC] = _PATTERN(clc),
	[MNEM_CLD] = _PATTERN(cld),
#ifdef _INCLUDE_65CE02
	[MNEM_CLE] = _PATTERN(cle),
#endif
	[MNEM_CLI] = _PATTERN(cli),
	[MNEM_CLV] = _PATTERN(clv),
	[MNEM_CMP] = _PATTERN(cmp),
#ifdef _INCLUDE_65C816
	[MNEM_COP] = _PATTERN(cop),
#endif
	[MNEM_CPX] = _PATTERN(cpx),
	[MNEM_CPY] = _PATTERN(cpy),
#ifdef _INCLUDE_65CE02
	[MNEM_CPZ] = _PATTERN(cpz),
#endif
	[MNEM_DEC] = _PATTERN(dec),
#ifdef _INCLUDE_65CE02
	[MNEM_DEW] = _PATTERN(dew),
#endif
	[MNEM_DEX] = _PATTERN(dex),
	[MNEM_DEY] = _PATTERN(dey),
#ifdef _INCLUDE_65CE02
	[MNEM_DEZ] = _PATTERN(dez),
#endif
	[MNEM_EOR] = _PATTERN(eor),
	[MNEM_INC] = _PATTERN(inc),
#ifdef _INCLUDE_65CE02
	[MNEM_INW] = _PATTERN(inw),
#endif
	[MNEM_INX] = _PATTERN(inx),
	[MNEM_INY] = _PATTERN(iny),
#ifdef _INCLUDE_65CE02
	[MNEM_INZ] = _PATTERN(inz),
#endif
#ifdef _INCLUDE_65C816
	[MNEM_JML] = _PATTERN(jml),
#endif
	[MNEM_JMP] = _PATTERN(jmp),
#ifdef _INCLUDE_65C816
	[MNEM_JSL] = _PATTERN(jsl),
#endif
	[MNEM_JSR] = _PATTERN(jsr),
	[MNEM_LDA] = _PATTERN(lda),
	[MNEM_LDX] = _PATTERN(ldx),
	[MNEM_LDY] = _PATTERN(ldy),
#ifdef _INCLUDE_65CE02
	[MNEM_LDZ] = _PATTERN(ldz),
#endif
	[MNEM_LSR] = _PATTERN(lsr),
#ifdef _INCLUDE_65C816
	[MNEM_MVN] = _PATTERN(mvn),
	[MNEM_MVP] = _PATTERN(mvp),
#endif
#ifdef _INCLUDE_65CE02
	[MNEM_NEG] = _PATTERN(neg),
#endif
	[MNEM_NOP] = _PATTERN(nop),
	[MNEM_ORA] = _PATTERN(ora),
#ifdef _INCLUDE_65C816
	[MNEM_PEA] = _PATTERN(pea),
	[MNEM_PEI] = _PATTERN(pei),
	[MNEM_PER] = _PATTERN(per),
#endif
	[MNEM_PHA] = _PATTERN(pha),
#ifdef _INCLUDE_65C816
	[MNEM_PHB] = _PATTERN(phb),
	[MNEM_PHD] = _PATTERN(phd),
	[MNEM_PHK] = _PATTERN(phk),
#endif
	[MNEM_PHP] = _PATTERN(php),
#ifdef _INCLUDE_65CE02
	[MNEM_PHW] = _PATTERN(phw),
#endif
#ifdef _INCLUDE_65C02
	[MNEM_PHX] = _PATTERN(phx),
	[MNEM_PHY] = _PATTERN(phy),
#endif
#ifdef _INCLUDE_65CE02
	[MNEM_PHZ] = _PATTERN(phz),
#endif
	[MNEM_PLA] = _PATTERN(pla),
#ifdef _INCLUDE_65C816
	[MNEM_PLB] = _PATTERN(plb),
	[MNEM_PLD] = _PATTERN(pld),
#endif
	[MNEM_PLP] = _PATTERN(plp),
#ifdef _INCLUDE_65C02
	[MNEM_PLX] = _PATTERN(plx),
	[MNEM_PLY] = _PATTERN(ply),
#endif
#ifdef _INCLUDE_65CE02
	[MNEM_PLZ] = _PATTERN(plz),
#endif
#ifdef _INCLUDE_65C816
	[MNEM_REP] = _PATTERN(rep),
#endif
#ifdef _INCLUDE_WDC65C02
	[MNEM_RMB] = _PATTERN(rmb_n),
#endif
	[MNEM_ROL] = _PATTERN(rol),
	[MNEM_ROR] = _PATTERN(ror),
#ifdef _INCLUDE_65CE02
	[MNEM_ROW] = _PATTERN(row),
#endif
	[MNEM_RTI] = _PATTERN(rti),
#ifdef _INCLUDE_65C816
	[MNEM_RTL] = _PATTERN(rtl),
#endif
	[MNEM_RTS] = _PATTERN(rts),
	[MNEM_SBC] = _PATTERN(sbc),
	[MNEM_SEC] = _PATTERN(sec),
	[MNEM_SED] = _PATTERN(sed),
#ifdef _INCLUDE_65CE02
	[MNEM_SEE] = _PATTERN(see),
#endif
	[MNEM_SEI] = _PATTERN(sei),
#ifdef _INCLUDE_65C816
	[MNEM_SEP] = _PATTERN(sep),
#endif
#ifdef _INCLUDE_WDC65C02
	[MNEM_SMB] = _PATTERN(smb_n),
#endif
	[MNEM_STA] = _PATTERN(sta),
	[MNEM_STP] = _PATTERN(stp),
	[MNEM_STX] = _PATTERN(stx),
	[MNEM_STY] = _PATTERN(sty),
#ifdef _INCLUDE_65C02
	[MNEM_STZ] = _PATTERN(stz),
#endif
#ifdef _INCLUDE_65CE02
	[MNEM_TAB] = _PATTERN(tab),
#endif
	[MNEM_TAX] = _PATTERN(tax),
	[MNEM_TAY] = _PATTERN(tay),
#ifdef _INCLUDE_65CE02
	[MNEM_TAZ] = _PATTERN(taz),
#endif
#ifdef _INCLUDE_65CE02
	[MNEM_TBA] = _PATTERN(tba),
#endif
#ifdef _INCLUDE_65C816
	[MNEM_TCD] = _PATTERN(tcd),
	[MNEM_TCS] = _PATTERN(tcs),
	[MNEM_TDC] = _PATTERN(tdc),
#endif
#ifdef _INCLUDE_65C02
	[MNEM_TRB] = _PATTERN(trb),
	[MNEM_TSB] = _PATTERN(tsb),
#endif
#ifdef _INCLUDE_65C816
	[MNEM_TSC] = _PATTERN(tsc),
#endif
	[MNEM_TSX] = _PATTERN(tsx),
#ifdef _INCLUDE_65CE02
	[MNEM_TSY] = _PATTERN(tsy),
#endif
	[MNEM_TXA] = _PATTERN(txa),
	[MNEM_TXS] = _PATTERN(txs),
#ifdef _INCLUDE_65C816
	[MNEM_TXY] = _PATTERN(txy),
#endif
	[MNEM_TYA] = _PATTERN(tya),
#ifdef _INCLUDE_65CE02
	[MNEM_TYS] = _PATTERN(tys),
#endif
#ifdef _INCLUDE_65C816
	[MNEM_TYX] = _PATTERN(tyx),
#endif
#ifdef _INCLUDE_65CE02
	[MNEM_TZA] = _PATTERN(tza),
#endif
#ifdef _INCLUDE_65C816
	[MNEM_WAI] = _PATTERN(wai),
#endif
#ifdef _INCLUDE_65C816
	[MNEM_WDM] = _PATTERN(wdm),
	[MNEM_XBA] = _PATTERN(xba),
	[MNEM_XCE] = _PATTERN(xce),
#endif
};

#undef _CPUNAME
#undef _PATTERNS
#undef __PATTERNS
#undef ___PATTERNS

