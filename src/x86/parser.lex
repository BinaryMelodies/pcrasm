
%include "../parser.lex"

%{
#include <assert.h>

#include "../../../src/asm.h"
#include "../../../src/x86/isa.h"
#include "parser.tab.h"

static const char _reg8ord[] = { 0, 3, 1, 2 };
#define reg8ord(letter) (_reg8ord[(letter) - 'a'])
#define reg16ord(letter1, letter2) ((letter1) == 'd' ? 7 : (letter1) == 'b' ? 5 : (letter2) == 'p' ? 4 : 6)

static const char _segord[] = { 1, 3, 0, 4, 5 };
#define segord(letter) ((letter) == 's' ? 2 : _segord[(letter) - 'c'])
#define necseg0ord(letter) ((letter) == 'p' ? 1 : 2)
static const char _necseg1ord[] = { 3, 0, 7, 6 };
#define necseg1ord(letter) (_necseg1ord[(letter) - '0'])

static int parse_cond(const char * cond);
static int parse_cond_nec(const char * cond);
static int parse_cond_x80(const char * cond);
static int parse_cond_x87(const char * cond); // for FCMOVcc
%}

COND	(a|ae|b|be|c|e|g|ge|l|le|na|nae|nb|nbe|nc|ne|ng|nge|nl|nle|no|np|ns|nz|o|p|pe|po|s|z)
NEC_COND	(c|e|ge|gt|h|l|le|lt|n|nc|ne|nh|nl|nv|nz|p|pe|po|v|z)
X80_COND	(nz|z|nc|c|po|pe|p|m)

%s INTEL NEC I8080 Z80 I8089
%%

".far"|"far"	{ return KWD_FAR; }
".seg"|"seg"	{ return KWD_SEG; }
".wrt"|"wrt"	{ return KWD_WRT; }

".word16"	{ yylval.i = _DATA_LE(BITSIZE16); return TOK_DATA; }
".word32"	{ yylval.i = _DATA_LE(BITSIZE32); return TOK_DATA; }
".word64"	{ yylval.i = _DATA_LE(BITSIZE64); return TOK_DATA; }

".byte"|"byte"|"db"	{ yylval.i = BITSIZE8; return TOK_DATA; }
".word"|"word"	{ yylval.i = _DATA_LE(BITSIZE16); return TOK_DATA; }
<INTEL,I8080,Z80,I8089>"dw"	{ yylval.i = _DATA_LE(BITSIZE16); return TOK_DATA; }
<NEC>"dw"	{ return TOK_DW; }
".dword"|"dword"|"dd"	{ yylval.i = _DATA_LE(BITSIZE32); return TOK_DATA; }
".long"|"long"	{ yylval.i = _DATA_LE(BITSIZE32); return TOK_DATA; }
".qword"|"qword"|"dq"	{ yylval.i = _DATA_LE(BITSIZE64); return TOK_DATA; }
".quad"|"quad"	{ yylval.i = _DATA_LE(BITSIZE64); return TOK_DATA; }
".tword"|"tword"|"dt"	{ yylval.i = _DATA_LE(BITSIZE80); return TOK_DATA; }

(".")?"rel"	{ return KWD_REL; }

".8088"|".8086"	{ yylval.i = CPU_8086; return TOK_ARCH; }
".80188"|".80186"|".188"|".186"	{ yylval.i = CPU_186; return TOK_ARCH; }
".v20"|".v30"	{ yylval.i = CPU_V30; return TOK_ARCH; }
".9002"	{ yylval.i = CPU_9002; return TOK_ARCH; }
".v33"|".v53"	{ yylval.i = CPU_V33; return TOK_ARCH; }
".v25"	{ yylval.i = CPU_V25; return TOK_ARCH; }
".v55"	{ yylval.i = CPU_V55; return TOK_ARCH; }
".80286"|".286"	{ yylval.i = CPU_286; return TOK_ARCH; }
".80386"|".386"	{ yylval.i = CPU_386B1; return TOK_ARCH; }
".80386b0"|".386b0"	{ yylval.i = CPU_386; return TOK_ARCH; }
".80486"|".486"	{ yylval.i = CPU_486B; return TOK_ARCH; }
".80486a"|".486a"	{ yylval.i = CPU_486; return TOK_ARCH; }
".80586"|".586"	{ yylval.i = CPU_586; return TOK_ARCH; }
".ia64"	{ yylval.i = CPU_IA64; return TOK_ARCH; }
".amd64"|".intel64"|".x86_64"|".x64"	{ yylval.i = CPU_X64; return TOK_ARCH; }
".cyrix"	{ yylval.i = CPU_CYRIX; return TOK_ARCH; }
".mx"	{ yylval.i = CPU_MX; return TOK_ARCH; }
".gx"	{ yylval.i = CPU_GX; return TOK_ARCH; }
".gx2"	{ yylval.i = CPU_GX2; return TOK_ARCH; }
".8080"	{ yylval.i = CPU_8080; return TOK_ARCH; }
".z80"	{ yylval.i = CPU_Z80; return TOK_ARCH; }
<INTEL,NEC>".no87"	{ yylval.i = FPU_NONE; return TOK_FPU; }
<INTEL,NEC>".8087"	{ yylval.i = FPU_8087; return TOK_FPU; }
<INTEL,NEC>".287"	{ yylval.i = FPU_287; return TOK_FPU; }
<INTEL,NEC>".387"	{ yylval.i = FPU_387; return TOK_FPU; }
".8089"	{ yylval.i = CPU_8089; return TOK_ARCH; }

<INTEL>".a16"	{ yylval.i = 2; return TOK_ASIZE; }
<INTEL>".a32"	{ yylval.i = 4; return TOK_ASIZE; }
<INTEL>".a64"	{ yylval.i = 8; return TOK_ASIZE; }

<INTEL,NEC,I8080,Z80>".code16"	{ yylval.i = BITSIZE16; return TOK_BITS; }
<INTEL>".code32"	{ yylval.i = BITSIZE32; return TOK_BITS; }
<INTEL>".code64"	{ yylval.i = BITSIZE64; return TOK_BITS; }
<INTEL,NEC,I8080,Z80>".bits"	{ return KWD_BITS; }

<INTEL>cr([12]?[0-9]|3[01])	{ yylval.i = strtol(yytext + 2, NULL, 10); return TOK_CREG; }

<NEC>cy	{ return TOK_CYSYMBOL; /* NEC */ }

<NEC>dir	{ return TOK_DIRSYMBOL; /* NEC */ }

<INTEL>dr([12]?[0-9]|3[01])	{ yylval.i = strtol(yytext + 2, NULL, 10); return TOK_DREG; }

<INTEL,NEC>(a|c|d|b)l	{ yylval.i = _OPD(OPD_GPRB, reg8ord(yytext[0])); return TOK_GPR; }
<INTEL,NEC>(a|c|d|b)h	{ yylval.i = _OPD(OPD_GPRH, reg8ord(yytext[0]) + 4); return TOK_GPR; }
<NEC>(a|c|b)w	{ yylval.i = _OPD(OPD_GPRW, reg8ord(yytext[0])); return TOK_GPR; /* NEC */ }
<INTEL>(a|c|d|b)x	{ yylval.i = _OPD(OPD_GPRW, reg8ord(yytext[0])); return TOK_GPR; }
<INTEL>e(a|c|d|b)x	{ yylval.i = _OPD(OPD_GPRD, reg8ord(yytext[1])); return TOK_GPR; }
<INTEL>r(a|c|d|b)x	{ yylval.i = _OPD(OPD_GPRQ, reg8ord(yytext[1])); return TOK_GPR; }
<INTEL>(sp|bp|si|di)l	{ yylval.i = _OPD(OPD_GPRB, reg16ord(yytext[0], yytext[1])); return TOK_GPR; }
<INTEL>(sp|bp|si|di)	{ yylval.i = _OPD(OPD_GPRW, reg16ord(yytext[0], yytext[1])); return TOK_GPR; }
<INTEL>e(sp|bp|si|di)	{ yylval.i = _OPD(OPD_GPRD, reg16ord(yytext[1], yytext[1])); return TOK_GPR; }
<INTEL>r(sp|bp|si|di)	{ yylval.i = _OPD(OPD_GPRQ, reg16ord(yytext[1], yytext[1])); return TOK_GPR; }
<NEC>(ix|iy)	{ yylval.i = _OPD(OPD_GPRW, yytext[1] - 'x' + 6); return TOK_GPR; /* NEC */ }
<INTEL>(r[89]|r[12][0-9]|r3[01])b	{ yylval.i = _OPD(OPD_GPRB, strtol(yytext + 1, NULL, 10)); return TOK_GPR; }
<INTEL>(r[89]|r[12][0-9]|r3[01])w	{ yylval.i = _OPD(OPD_GPRW, strtol(yytext + 1, NULL, 10)); return TOK_GPR; }
<INTEL>(r[89]|r[12][0-9]|r3[01])d	{ yylval.i = _OPD(OPD_GPRD, strtol(yytext + 1, NULL, 10)); return TOK_GPR; }
<INTEL>(r[89]|r[12][0-9]|r3[01])	{ yylval.i = _OPD(OPD_GPRQ, strtol(yytext + 1, NULL, 10)); return TOK_GPR; }

<INTEL>eip	{ yylval.i = _OPD(OPD_GPRD, 0); return TOK_IPREG; }
<INTEL>rip	{ yylval.i = _OPD(OPD_GPRQ, 0); return TOK_IPREG; }

<NEC>iram	{ return TOK_IRAM; /* NEC */ }

<INTEL>lock	{ return TOK_LOCK; }
<NEC>buslock	{ return TOK_LOCK; /* NEC */ }

<INTEL>mm[0-7]	{ yylval.i = yytext[2] - '0'; return TOK_MMREG; }

<INTEL>"aaa"	{ yylval.i = MNEM_AAA; return TOK_MNEMONIC; }
<INTEL>"aad"	{ yylval.i = MNEM_AAD; return TOK_MNEMONIC; }
<INTEL>"aam"	{ yylval.i = MNEM_AAM; return TOK_MNEMONIC; }
<INTEL>"aas"	{ yylval.i = MNEM_AAS; return TOK_MNEMONIC; }
<INTEL>"adc"	{ yylval.i = MNEM_ADC; return TOK_MNEMONIC; }
<NEC>"add4s"	{ yylval.i = MNEM_ADD4S; return TOK_MNEMONIC; }
<NEC>"addc"	{ yylval.i = MNEM_ADC; return TOK_MNEMONIC; }
<NEC,INTEL>"add"	{ yylval.i = MNEM_ADD; return TOK_MNEMONIC; }
<NEC>"adj4a"	{ yylval.i = MNEM_DAA; return TOK_MNEMONIC; }
<NEC>"adj4s"	{ yylval.i = MNEM_DAS; return TOK_MNEMONIC; }
<NEC>"adjba"	{ yylval.i = MNEM_AAA; return TOK_MNEMONIC; }
<NEC>"adjbs"	{ yylval.i = MNEM_AAS; return TOK_MNEMONIC; }
<NEC,INTEL>"and"	{ yylval.i = MNEM_AND; return TOK_MNEMONIC; }
<INTEL>"arpl"	{ yylval.i = MNEM_ARPL; return TOK_MNEMONIC; /* 286 */ }
<NEC>"b"{NEC_COND}	{ yylval.i = _COND_MNEM(parse_cond_nec(yytext + 1), MNEM_J_CC); return TOK_MNEMONIC; }
<INTEL>"bound"	{ yylval.i = MNEM_BOUND; return TOK_MNEMONIC; /* 186 */ }
<NEC>"bcwz"	{ yylval.i = MNEM_JCXZ; return TOK_MNEMONIC; }
<NEC>"br"	{ yylval.i = MNEM_JMP; return TOK_MNEMONIC; }
<NEC>"brk"	{ yylval.i = MNEM_BRK; return TOK_MNEMONIC; }
<NEC>"brkem"	{ yylval.i = MNEM_BRKEM; return TOK_MNEMONIC; /* v20/v30 */ }
<NEC>"brkem2"|"brkfem"	{ yylval.i = MNEM_BRKEM2; return TOK_MNEMONIC; /* µpd9002 */ }
<NEC>"brkv"	{ yylval.i = MNEM_INTO; return TOK_MNEMONIC; }
<NEC>"brkxa"	{ yylval.i = MNEM_BRKXA; return TOK_MNEMONIC; /* v33/v53 */ }
<INTEL>"bsf"	{ yylval.i = MNEM_BSF; return TOK_MNEMONIC; /* 386 */ }
<INTEL>"bsr"	{ yylval.i = MNEM_BSR; return TOK_MNEMONIC; /* 386 */ }
<INTEL>"bswap"	{ yylval.i = MNEM_BSWAP; return TOK_MNEMONIC; /* 486 */ }
<INTEL>"bt"	{ yylval.i = MNEM_BT; return TOK_MNEMONIC; /* 386 */ }
<INTEL>"btc"	{ yylval.i = MNEM_BTC; return TOK_MNEMONIC; /* 386 */ }
<INTEL>"btr"	{ yylval.i = MNEM_BTR; return TOK_MNEMONIC; /* 386 */ }
<INTEL>"bts"	{ yylval.i = MNEM_BTS; return TOK_MNEMONIC; /* 386 */ }
<NEC,INTEL>"call"	{ yylval.i = MNEM_CALL; return TOK_MNEMONIC; }
<INTEL>"cbw"	{ yylval.i = MNEM_CBW; return TOK_MNEMONIC; }
<INTEL>"cdq"	{ yylval.i = MNEM_CDQ; return TOK_MNEMONIC; /* 386 */ }
<INTEL>"cdqe"	{ yylval.i = MNEM_CDQE; return TOK_MNEMONIC; /* x64 */ }
<NEC>"chkind"	{ yylval.i = MNEM_BOUND; return TOK_MNEMONIC; }
<INTEL>"clc"	{ yylval.i = MNEM_CLC; return TOK_MNEMONIC; }
<INTEL>"cld"	{ yylval.i = MNEM_CLD; return TOK_MNEMONIC; }
<INTEL>"cli"	{ yylval.i = MNEM_CLI; return TOK_MNEMONIC; }
<NEC>"clr1"	{ yylval.i = MNEM_CLR1; return TOK_MNEMONIC; }
<INTEL>"clts"	{ yylval.i = MNEM_CLTS; return TOK_MNEMONIC; /* 286 */ }
<INTEL>"cmc"	{ yylval.i = MNEM_CMC; return TOK_MNEMONIC; }
<INTEL>"cmov"{COND}	{ yylval.i = _COND_MNEM(parse_cond(yytext + 4), MNEM_CMOV_CC); return TOK_MNEMONIC; /* 586 */ }
<NEC,INTEL>"cmp"	{ yylval.i = MNEM_CMP; return TOK_MNEMONIC; }
<NEC>"cmp4s"	{ yylval.i = MNEM_CMP4S; return TOK_MNEMONIC; }
<NEC>"cmpbk"	{ yylval.i = MNEM_CMPS; return TOK_MNEMONIC; }
<NEC>"cmpbkb"	{ yylval.i = MNEM_CMPSB; return TOK_MNEMONIC; }
<NEC>"cmpbkw"	{ yylval.i = MNEM_CMPSW; return TOK_MNEMONIC; }
<NEC>"cmpm"	{ yylval.i = MNEM_SCAS; return TOK_MNEMONIC; }
<NEC>"cmpmb"	{ yylval.i = MNEM_SCASB; return TOK_MNEMONIC; }
<NEC>"cmpmw"	{ yylval.i = MNEM_SCASW; return TOK_MNEMONIC; }
<INTEL>"cmps"	{ yylval.i = MNEM_CMPS; return TOK_MNEMONIC; }
<INTEL>"cmpsb"	{ yylval.i = MNEM_CMPSB; return TOK_MNEMONIC; }
<INTEL>"cmpsw"	{ yylval.i = MNEM_CMPSW; return TOK_MNEMONIC; }
<INTEL>"cmpsd"	{ yylval.i = MNEM_CMPSD; return TOK_MNEMONIC; }
<INTEL>"cmpsq"	{ yylval.i = MNEM_CMPSQ; return TOK_MNEMONIC; }
<INTEL>"cmpxchg"	{ yylval.i = MNEM_CMPXCHG; return TOK_MNEMONIC; /* 486 */ }
<INTEL>"cmpxchg8b"	{ yylval.i = MNEM_CMPXCHG8B; return TOK_MNEMONIC; /* 586 */ }
<INTEL>"cmpxchg16b"	{ yylval.i = MNEM_CMPXCHG16B; return TOK_MNEMONIC; /* x64 */ }
<INTEL>"cpuid"	{ yylval.i = MNEM_CPUID; return TOK_MNEMONIC; /* 586 */ }
<INTEL>"cqo"	{ yylval.i = MNEM_CQO; return TOK_MNEMONIC; /* x64 */ }
<NEC>"cvtbd"	{ yylval.i = MNEM_AAM; return TOK_MNEMONIC; }
<NEC>"cvtbw"	{ yylval.i = MNEM_CBW; return TOK_MNEMONIC; }
<NEC>"cvtdb"	{ yylval.i = MNEM_AAD; return TOK_MNEMONIC; }
<NEC>"cvtwl"	{ yylval.i = MNEM_CWD; return TOK_MNEMONIC; }
<INTEL>"cwd"	{ yylval.i = MNEM_CWD; return TOK_MNEMONIC; }
<INTEL>"cwde"	{ yylval.i = MNEM_CWDE; return TOK_MNEMONIC; /* 386 */ }
<INTEL>"daa"	{ yylval.i = MNEM_DAA; return TOK_MNEMONIC; }
<INTEL>"das"	{ yylval.i = MNEM_DAS; return TOK_MNEMONIC; }
<NEC>"dbnz"	{ yylval.i = MNEM_LOOP; return TOK_MNEMONIC; }
<NEC>"dbnze"	{ yylval.i = MNEM_LOOPE; return TOK_MNEMONIC; }
<NEC>"dbnzne"	{ yylval.i = MNEM_LOOPNE; return TOK_MNEMONIC; }
<NEC,INTEL>"dec"	{ yylval.i = MNEM_DEC; return TOK_MNEMONIC; }
<NEC>"di"	{ yylval.i = MNEM_CLI; return TOK_MNEMONIC; }
<NEC>"dispose"	{ yylval.i = MNEM_LEAVE; return TOK_MNEMONIC; }
<INTEL>"div"	{ yylval.i = MNEM_DIV; return TOK_MNEMONIC; }
<NEC>"div"	{ yylval.i = MNEM_IDIV; return TOK_MNEMONIC; }
<NEC>"divu"	{ yylval.i = MNEM_DIV; return TOK_MNEMONIC; }
<NEC>"ei"	{ yylval.i = MNEM_STI; return TOK_MNEMONIC; }
<INTEL>"enter"	{ yylval.i = MNEM_ENTER; return TOK_MNEMONIC; /* 186 */ }
<NEC>"ext"	{ yylval.i = MNEM_EXT; return TOK_MNEMONIC; }
<NEC>"halt"	{ yylval.i = MNEM_HLT; return TOK_MNEMONIC; }
<INTEL>"hlt"	{ yylval.i = MNEM_HLT; return TOK_MNEMONIC; }
<INTEL>"idiv"	{ yylval.i = MNEM_IDIV; return TOK_MNEMONIC; }
<INTEL>"imul"	{ yylval.i = MNEM_IMUL; return TOK_MNEMONIC; }
<NEC,INTEL>"in"	{ yylval.i = MNEM_IN; return TOK_MNEMONIC; }
<NEC,INTEL>"inc"	{ yylval.i = MNEM_INC; return TOK_MNEMONIC; }
<NEC>"inm"	{ yylval.i = MNEM_INS; return TOK_MNEMONIC; }
<NEC>"inmb"	{ yylval.i = MNEM_INSB; return TOK_MNEMONIC; }
<NEC>"inmw"	{ yylval.i = MNEM_INSW; return TOK_MNEMONIC; }
<INTEL>"ins"	{ yylval.i = MNEM_INS; return TOK_MNEMONIC; /* 186 */ }
<INTEL>"insb"	{ yylval.i = MNEM_INSB; return TOK_MNEMONIC; /* 186 */ }
<INTEL>"insw"	{ yylval.i = MNEM_INSW; return TOK_MNEMONIC; /* 186 */ }
<INTEL>"insd"	{ yylval.i = MNEM_INSD; return TOK_MNEMONIC; /* 186 */ }
<NEC>"ins"	{ yylval.i = MNEM_INS_NEC; return TOK_MNEMONIC; }
<INTEL>"int"	{ yylval.i = MNEM_INT; return TOK_MNEMONIC; }
<INTEL>"int1"|"int01"|"icebp"	{ yylval.i = MNEM_INT1; return TOK_MNEMONIC; /* 386 */ }
<INTEL>"int3"	{ yylval.i = MNEM_INT3; return TOK_MNEMONIC; }
<INTEL>"into"	{ yylval.i = MNEM_INTO; return TOK_MNEMONIC; }
<INTEL>"invd"	{ yylval.i = MNEM_INVD; return TOK_MNEMONIC; /* 486 */ }
<INTEL>"invlpg"	{ yylval.i = MNEM_INVLPG; return TOK_MNEMONIC; /* 486 */ }
<INTEL>"iret"w?	{ yylval.i = MNEM_IRET; return TOK_MNEMONIC; }
<INTEL>"iretd"	{ yylval.i = MNEM_IRETD; return TOK_MNEMONIC; }
<INTEL>"iretq"	{ yylval.i = MNEM_IRETQ; return TOK_MNEMONIC; }
<INTEL>"j"{COND}	{ yylval.i = _COND_MNEM(parse_cond(yytext + 1), MNEM_J_CC); return TOK_MNEMONIC; }
<INTEL>"jcxz"	{ yylval.i = MNEM_JCXZ; return TOK_MNEMONIC; }
<INTEL>"jecxz"	{ yylval.i = MNEM_JECXZ; return TOK_MNEMONIC; }
<INTEL>"jrcxz"	{ yylval.i = MNEM_JRCXZ; return TOK_MNEMONIC; }
<INTEL>"jmp"	{ yylval.i = MNEM_JMP; return TOK_MNEMONIC; }
<INTEL>"lahf"	{ yylval.i = MNEM_LAHF; return TOK_MNEMONIC; }
<INTEL>"lar"	{ yylval.i = MNEM_LAR; return TOK_MNEMONIC; /* 286 */ }
<NEC>"ldea"	{ yylval.i = MNEM_LDEA; return TOK_MNEMONIC; }
<INTEL>"lds"	{ yylval.i = MNEM_LDS; return TOK_MNEMONIC; }
<INTEL>"lea"	{ yylval.i = MNEM_LEA; return TOK_MNEMONIC; }
<INTEL>"leave"	{ yylval.i = MNEM_LEAVE; return TOK_MNEMONIC; /* 186 */ }
<INTEL>"les"	{ yylval.i = MNEM_LES; return TOK_MNEMONIC; }
<NEC>"ldm"	{ yylval.i = MNEM_LODS; return TOK_MNEMONIC; }
<NEC>"ldmb"	{ yylval.i = MNEM_LODSB; return TOK_MNEMONIC; }
<NEC>"ldmw"	{ yylval.i = MNEM_LODSW; return TOK_MNEMONIC; }
<INTEL>"lfs"	{ yylval.i = MNEM_LFS; return TOK_MNEMONIC; /* 386 */ }
<INTEL>"lgdt"	{ yylval.i = MNEM_LGDT; return TOK_MNEMONIC; /* 286 */ }
<INTEL>"lgs"	{ yylval.i = MNEM_LGS; return TOK_MNEMONIC; /* 386 */ }
<INTEL>"lidt"	{ yylval.i = MNEM_LIDT; return TOK_MNEMONIC; /* 286 */ }
<INTEL>"lldt"	{ yylval.i = MNEM_LLDT; return TOK_MNEMONIC; /* 286 */ }
<INTEL>"lmsw"	{ yylval.i = MNEM_LMSW; return TOK_MNEMONIC; /* 286 */ }
<INTEL>"lods"	{ yylval.i = MNEM_LODS; return TOK_MNEMONIC; }
<INTEL>"lodsb"	{ yylval.i = MNEM_LODSB; return TOK_MNEMONIC; }
<INTEL>"lodsw"	{ yylval.i = MNEM_LODSW; return TOK_MNEMONIC; }
<INTEL>"lodsd"	{ yylval.i = MNEM_LODSD; return TOK_MNEMONIC; }
<INTEL>"lodsq"	{ yylval.i = MNEM_LODSQ; return TOK_MNEMONIC; }
<INTEL>"loop"	{ yylval.i = MNEM_LOOP; return TOK_MNEMONIC; }
<INTEL>"loop"[ez]	{ yylval.i = MNEM_LOOPE; return TOK_MNEMONIC; }
<INTEL>"loopn"[ez]	{ yylval.i = MNEM_LOOPNE; return TOK_MNEMONIC; }
<INTEL>"lsl"	{ yylval.i = MNEM_LSL; return TOK_MNEMONIC; /* 286 */ }
<INTEL>"lss"	{ yylval.i = MNEM_LSS; return TOK_MNEMONIC; /* 386 */ }
<INTEL>"ltr"	{ yylval.i = MNEM_LTR; return TOK_MNEMONIC; /* 286 */ }
<INTEL>"lzcnt"	{ yylval.i = MNEM_LZCNT; return TOK_MNEMONIC; /* later */ }
<INTEL>"mov"	{ yylval.i = MNEM_MOV; return TOK_MNEMONIC; }
<NEC>"mov"	{ yylval.i = MNEM_MOV_NEC; return TOK_MNEMONIC; }
<NEC>"movbk"	{ yylval.i = MNEM_MOVS; return TOK_MNEMONIC; }
<NEC>"movbkb"	{ yylval.i = MNEM_MOVSB; return TOK_MNEMONIC; }
<NEC>"movbkw"	{ yylval.i = MNEM_MOVSW; return TOK_MNEMONIC; }
<INTEL>"movs"	{ yylval.i = MNEM_MOVS; return TOK_MNEMONIC; }
<INTEL>"movsb"	{ yylval.i = MNEM_MOVSB; return TOK_MNEMONIC; }
<INTEL>"movsw"	{ yylval.i = MNEM_MOVSW; return TOK_MNEMONIC; }
<INTEL>"movsd"	{ yylval.i = MNEM_MOVSD; return TOK_MNEMONIC; }
<INTEL>"movsq"	{ yylval.i = MNEM_MOVSQ; return TOK_MNEMONIC; }
<INTEL>"movsx"	{ yylval.i = MNEM_MOVSX; return TOK_MNEMONIC; /* 386 */ }
<INTEL>"movsxd"	{ yylval.i = MNEM_MOVSXD; return TOK_MNEMONIC; /* x64 */ }
<INTEL>"movzx"	{ yylval.i = MNEM_MOVZX; return TOK_MNEMONIC; /* 386 */ }
<INTEL>"mul"	{ yylval.i = MNEM_MUL; return TOK_MNEMONIC; }
<NEC>"mul"	{ yylval.i = MNEM_MUL_NEC; return TOK_MNEMONIC; }
<NEC>"mulu"	{ yylval.i = MNEM_MULU; return TOK_MNEMONIC; }
<NEC,INTEL>"neg"	{ yylval.i = MNEM_NEG; return TOK_MNEMONIC; }
<NEC,INTEL>"nop"	{ yylval.i = MNEM_NOP; return TOK_MNEMONIC; }
<INTEL>"nopl"	{ yylval.i = MNEM_NOPL; return TOK_MNEMONIC; /* 586 */ }
<NEC,INTEL>"not"	{ yylval.i = MNEM_NOT; return TOK_MNEMONIC; }
<NEC>"not1"	{ yylval.i = MNEM_NOT1; return TOK_MNEMONIC; }
<NEC,INTEL>"or"	{ yylval.i = MNEM_OR; return TOK_MNEMONIC; }
<NEC,INTEL>"out"	{ yylval.i = MNEM_OUT; return TOK_MNEMONIC; }
<NEC>"outm"	{ yylval.i = MNEM_OUTS; return TOK_MNEMONIC; }
<NEC>"outmb"	{ yylval.i = MNEM_OUTSB; return TOK_MNEMONIC; }
<NEC>"outmw"	{ yylval.i = MNEM_OUTSW; return TOK_MNEMONIC; }
<INTEL>"outs"	{ yylval.i = MNEM_OUTS; return TOK_MNEMONIC; /* 186 */ }
<INTEL>"outsb"	{ yylval.i = MNEM_OUTSB; return TOK_MNEMONIC; /* 186 */ }
<INTEL>"outsw"	{ yylval.i = MNEM_OUTSW; return TOK_MNEMONIC; /* 186 */ }
<INTEL>"outsd"	{ yylval.i = MNEM_OUTSD; return TOK_MNEMONIC; /* 186 */ }
<NEC>"poll"	{ yylval.i = MNEM_WAIT; return TOK_MNEMONIC; }
<NEC,INTEL>"pop"	{ yylval.i = MNEM_POP; return TOK_MNEMONIC; }
<INTEL>"popa"w?	{ yylval.i = MNEM_POPA; return TOK_MNEMONIC; /* 186 */ }
<INTEL>"popad"	{ yylval.i = MNEM_POPAD; return TOK_MNEMONIC; /* 186 */ }
<INTEL>"popcnt"	{ yylval.i = MNEM_POPCNT; return TOK_MNEMONIC; /* later */ }
<INTEL>"popf"w?	{ yylval.i = MNEM_POPF; return TOK_MNEMONIC; }
<INTEL>"popfd"	{ yylval.i = MNEM_POPFD; return TOK_MNEMONIC; }
<INTEL>"popfq"	{ yylval.i = MNEM_POPFQ; return TOK_MNEMONIC; }
<NEC>"prepare"	{ yylval.i = MNEM_ENTER; return TOK_MNEMONIC; }
<NEC,INTEL>"push"	{ yylval.i = MNEM_PUSH; return TOK_MNEMONIC; }
<INTEL>"pusha"w?	{ yylval.i = MNEM_PUSHA; return TOK_MNEMONIC; /* 186 */ }
<INTEL>"pushad"	{ yylval.i = MNEM_PUSHAD; return TOK_MNEMONIC; /* 186 */ }
<INTEL>"pushf"w?	{ yylval.i = MNEM_POPF; return TOK_MNEMONIC; }
<INTEL>"pushfd"	{ yylval.i = MNEM_POPFD; return TOK_MNEMONIC; }
<INTEL>"pushfq"	{ yylval.i = MNEM_POPFQ; return TOK_MNEMONIC; }
<INTEL>"rcl"	{ yylval.i = MNEM_RCL; return TOK_MNEMONIC; }
<INTEL>"rcr"	{ yylval.i = MNEM_RCR; return TOK_MNEMONIC; }
<INTEL>"rdfsbase"	{ yylval.i = MNEM_RDFSBASE; return TOK_MNEMONIC; /* later */ }
<INTEL>"rdgsbase"	{ yylval.i = MNEM_RDGSBASE; return TOK_MNEMONIC; /* later */ }
<INTEL>"rdmsr"	{ yylval.i = MNEM_RDMSR; return TOK_MNEMONIC; /* 586 */ }
<INTEL>"rdpid"	{ yylval.i = MNEM_RDPID; return TOK_MNEMONIC; /* later */ }
<INTEL>"rdpmc"	{ yylval.i = MNEM_RDPMC; return TOK_MNEMONIC; /* 586 */ }
<INTEL>"rdtsc"	{ yylval.i = MNEM_RDTSC; return TOK_MNEMONIC; /* 586 */ }
<INTEL>"rdtscp"	{ yylval.i = MNEM_RDTSCP; return TOK_MNEMONIC; /* later */ }
<NEC,INTEL>"ret"n?	{ yylval.i = MNEM_RET; return TOK_MNEMONIC; }
<NEC,INTEL>"retf"	{ yylval.i = MNEM_RETF; return TOK_MNEMONIC; }
<INTEL>"retw"n?	{ yylval.i = MNEM_RETW; return TOK_MNEMONIC; }
<INTEL>"retfw"	{ yylval.i = MNEM_RETFW; return TOK_MNEMONIC; }
<INTEL>"retd"n?	{ yylval.i = MNEM_RETD; return TOK_MNEMONIC; }
<INTEL>"retfd"	{ yylval.i = MNEM_RETFD; return TOK_MNEMONIC; }
<INTEL>"retq"n?	{ yylval.i = MNEM_RETQ; return TOK_MNEMONIC; }
<INTEL>"retfq"	{ yylval.i = MNEM_RETFQ; return TOK_MNEMONIC; }
<NEC>"reti"	{ yylval.i = MNEM_RETI; return TOK_MNEMONIC; }
<NEC>"retxa"	{ yylval.i = MNEM_RETXA; return TOK_MNEMONIC; /* v33/v53 */ }
<NEC,INTEL>"rol"	{ yylval.i = MNEM_ROL; return TOK_MNEMONIC; }
<NEC>"rol4"	{ yylval.i = MNEM_ROL4; return TOK_MNEMONIC; }
<NEC>"rolc"	{ yylval.i = MNEM_RCL; return TOK_MNEMONIC; }
<NEC,INTEL>"ror"	{ yylval.i = MNEM_ROR; return TOK_MNEMONIC; }
<NEC>"ror4"	{ yylval.i = MNEM_ROR4; return TOK_MNEMONIC; }
<NEC>"rorc"	{ yylval.i = MNEM_RCR; return TOK_MNEMONIC; }
<INTEL>"rsm"	{ yylval.i = MNEM_RSM; return TOK_MNEMONIC; /* 586 */ }
<INTEL>"sahf"	{ yylval.i = MNEM_SAHF; return TOK_MNEMONIC; }
<INTEL>"sal"	{ yylval.i = MNEM_SAL; return TOK_MNEMONIC; }
<INTEL>"sar"	{ yylval.i = MNEM_SAR; return TOK_MNEMONIC; }
<INTEL>"sbb"	{ yylval.i = MNEM_SBB; return TOK_MNEMONIC; }
<INTEL>"scas"	{ yylval.i = MNEM_SCAS; return TOK_MNEMONIC; }
<INTEL>"scasb"	{ yylval.i = MNEM_SCASB; return TOK_MNEMONIC; }
<INTEL>"scasw"	{ yylval.i = MNEM_SCASW; return TOK_MNEMONIC; }
<INTEL>"scasd"	{ yylval.i = MNEM_SCASD; return TOK_MNEMONIC; }
<INTEL>"scasq"	{ yylval.i = MNEM_SCASQ; return TOK_MNEMONIC; }
<NEC>"set1"	{ yylval.i = MNEM_SET1; return TOK_MNEMONIC; }
<INTEL>"set"{COND}	{ yylval.i = _COND_MNEM(parse_cond(yytext + 3), MNEM_SET_CC); return TOK_MNEMONIC; /* 386 */ }
<INTEL>"sgdt"	{ yylval.i = MNEM_SGDT; return TOK_MNEMONIC; /* 286 */ }
<NEC,INTEL>"shl"	{ yylval.i = MNEM_SHL; return TOK_MNEMONIC; }
<INTEL>"shld"	{ yylval.i = MNEM_SHLD; return TOK_MNEMONIC; /* 386 */ }
<NEC,INTEL>"shr"	{ yylval.i = MNEM_SHR; return TOK_MNEMONIC; }
<NEC>"shra"	{ yylval.i = MNEM_SAR; return TOK_MNEMONIC; }
<INTEL>"shrd"	{ yylval.i = MNEM_SHRD; return TOK_MNEMONIC; /* 386 */ }
<INTEL>"sidt"	{ yylval.i = MNEM_SIDT; return TOK_MNEMONIC; /* 286 */ }
<INTEL>"sldt"	{ yylval.i = MNEM_SLDT; return TOK_MNEMONIC; /* 286 */ }
<INTEL>"smsw"	{ yylval.i = MNEM_SMSW; return TOK_MNEMONIC; /* 286 */ }
<INTEL>"stc"	{ yylval.i = MNEM_STC; return TOK_MNEMONIC; }
<INTEL>"std"	{ yylval.i = MNEM_STD; return TOK_MNEMONIC; }
<INTEL>"sti"	{ yylval.i = MNEM_STI; return TOK_MNEMONIC; }
<NEC>"stm"	{ yylval.i = MNEM_STOS; return TOK_MNEMONIC; }
<NEC>"stmb"	{ yylval.i = MNEM_STOSB; return TOK_MNEMONIC; }
<NEC>"stmw"	{ yylval.i = MNEM_STOSW; return TOK_MNEMONIC; }
<INTEL>"stos"	{ yylval.i = MNEM_STOS; return TOK_MNEMONIC; }
<INTEL>"stosb"	{ yylval.i = MNEM_STOSB; return TOK_MNEMONIC; }
<INTEL>"stosw"	{ yylval.i = MNEM_STOSW; return TOK_MNEMONIC; }
<INTEL>"stosd"	{ yylval.i = MNEM_STOSD; return TOK_MNEMONIC; }
<INTEL>"stosq"	{ yylval.i = MNEM_STOSQ; return TOK_MNEMONIC; }
<INTEL>"str"	{ yylval.i = MNEM_STR; return TOK_MNEMONIC; /* 286 */ }
<NEC,INTEL>"sub"	{ yylval.i = MNEM_SUB; return TOK_MNEMONIC; }
<NEC>"sub4s"	{ yylval.i = MNEM_SUB4S; return TOK_MNEMONIC; }
<NEC>"subc"	{ yylval.i = MNEM_SBB; return TOK_MNEMONIC; }
<INTEL>"swapgs"	{ yylval.i = MNEM_SWAPGS; return TOK_MNEMONIC; /* x64 */ }
<INTEL>"syscall"	{ yylval.i = MNEM_SYSCALL; return TOK_MNEMONIC; /* 586 */ }
<INTEL>"sysenter"	{ yylval.i = MNEM_SYSENTER; return TOK_MNEMONIC; /* 586 */ }
<INTEL>"sysexit"	{ yylval.i = MNEM_SYSEXIT; return TOK_MNEMONIC; /* 586 */ }
<INTEL>"sysret"	{ yylval.i = MNEM_SYSRET; return TOK_MNEMONIC; /* 586 */ }
<NEC,INTEL>"test"	{ yylval.i = MNEM_TEST; return TOK_MNEMONIC; }
<NEC>"test1"	{ yylval.i = MNEM_TEST1; return TOK_MNEMONIC; }
<NEC>"trans"b?	{ yylval.i = MNEM_XLAT; return TOK_MNEMONIC; }
<INTEL>"tzcnt"	{ yylval.i = MNEM_TZCNT; return TOK_MNEMONIC; /* later */ }
<INTEL>"ud0"|"oio"	{ yylval.i = MNEM_UD0; return TOK_MNEMONIC; /* 286 */ }
<INTEL>"ud1"|"ud2b"	{ yylval.i = MNEM_UD1; return TOK_MNEMONIC; /* 286 */ }
<INTEL>"ud2"a?	{ yylval.i = MNEM_UD2; return TOK_MNEMONIC; /* 286 */ }
<INTEL>"verr"	{ yylval.i = MNEM_VERR; return TOK_MNEMONIC; /* 286 */ }
<INTEL>"verw"	{ yylval.i = MNEM_VERW; return TOK_MNEMONIC; /* 286 */ }
<INTEL>f?"wait"	{ yylval.i = MNEM_WAIT; return TOK_MNEMONIC; }
<INTEL>"wbinvd"	{ yylval.i = MNEM_WBINVD; return TOK_MNEMONIC; /* 486 */ }
<INTEL>"wrfsbase"	{ yylval.i = MNEM_WRFSBASE; return TOK_MNEMONIC; /* later */ }
<INTEL>"wrgsbase"	{ yylval.i = MNEM_WRGSBASE; return TOK_MNEMONIC; /* later */ }
<INTEL>"wrmsr"	{ yylval.i = MNEM_WRMSR; return TOK_MNEMONIC; /* 586 */ }
<INTEL>"xadd"	{ yylval.i = MNEM_XADD; return TOK_MNEMONIC; /* 486 */ }
<NEC>"xch"	{ yylval.i = MNEM_XCHG; return TOK_MNEMONIC; }
<INTEL>"xchg"	{ yylval.i = MNEM_XCHG; return TOK_MNEMONIC; }
<INTEL>"xlat"b?	{ yylval.i = MNEM_XLAT; return TOK_MNEMONIC; }
<NEC,INTEL>"xor"	{ yylval.i = MNEM_XOR; return TOK_MNEMONIC; }

<INTEL>"ibts"	{ yylval.i = MNEM_IBTS; return TOK_MNEMONIC; /* 386 */ }
<INTEL>"jmpe"	{ yylval.i = MNEM_JMPE; return TOK_MNEMONIC; /* ia64 */ }
<INTEL>"loadall"	{ yylval.i = MNEM_LOADALL; return TOK_MNEMONIC; /* 286 */ }
<INTEL>"loadall286"	{ yylval.i = MNEM_LOADALL286; return TOK_MNEMONIC; /* 286 */ }
<INTEL>"loadall386"|"loadalld"	{ yylval.i = MNEM_LOADALL386; return TOK_MNEMONIC; /* 386 */ }
<INTEL>"salc"|"setalc"	{ yylval.i = MNEM_SALC; return TOK_MNEMONIC; }
<INTEL>"setmo"	{ yylval.i = MNEM_SETMO; return TOK_MNEMONIC; /* 8086 */ }
<INTEL>"setmoc"	{ yylval.i = MNEM_SETMOC; return TOK_MNEMONIC; /* 8086 */ }
<INTEL>"storeall"	{ yylval.i = MNEM_STOREALL; return TOK_MNEMONIC; /* 286 */ }
<INTEL>"umov"	{ yylval.i = MNEM_UMOV; return TOK_MNEMONIC; /* 386 */ }
<INTEL>"xbts"	{ yylval.i = MNEM_XBTS; return TOK_MNEMONIC; /* 386 */ }
<INTEL>"umpf"	{ return TOK_286_UMOV_PREFIX; /* 286 */ }

<NEC>"brkcs"	{ yylval.i = MNEM_BRKCS; return TOK_MNEMONIC; /* v25 */ }
<NEC>"brkn"	{ yylval.i = MNEM_BRKN; return TOK_MNEMONIC; /* v25sg */ }
<NEC>"brks"	{ yylval.i = MNEM_BRKS; return TOK_MNEMONIC; /* v25sg */ }
<NEC>"bsch"	{ yylval.i = MNEM_BSCH; return TOK_MNEMONIC; /* v55 */ }
<NEC>"btclrl"	{ yylval.i = MNEM_BTCLRL; return TOK_MNEMONIC; /* v55 */ }
<NEC>"btclr"	{ yylval.i = MNEM_BTCLR; return TOK_MNEMONIC; /* v25 */ }
<NEC>"fint"	{ yylval.i = MNEM_FINT; return TOK_MNEMONIC; /* v25 */ }
<NEC>"movspa"	{ yylval.i = MNEM_MOVSPA; return TOK_MNEMONIC; /* v25 */ }
<NEC>"movspb"	{ yylval.i = MNEM_MOVSPB; return TOK_MNEMONIC; /* v25 */ }
<NEC>"qhout"	{ yylval.i = MNEM_QHOUT; return TOK_MNEMONIC; /* v55 */ }
<NEC>"qout"	{ yylval.i = MNEM_QOUT; return TOK_MNEMONIC; /* v55 */ }
<NEC>"qtin"	{ yylval.i = MNEM_QTIN; return TOK_MNEMONIC; /* v55 */ }
<NEC>"retrbi"	{ yylval.i = MNEM_RETRBI; return TOK_MNEMONIC; /* v25 */ }
<NEC>"rstwdt"	{ yylval.i = MNEM_RSTWDT; return TOK_MNEMONIC; /* v55 */ }
<NEC>"stop"	{ yylval.i = MNEM_STOP; return TOK_MNEMONIC; /* v25 */ }
<NEC>"tsksw"	{ yylval.i = MNEM_TSKSW; return TOK_MNEMONIC; /* v25 */ }

<NEC>"idle"	{ yylval.i = MNEM_IDLE; return TOK_MNEMONIC; /* v25sc */ }
<NEC>"albit"	{ yylval.i = MNEM_ALBIT; return TOK_MNEMONIC; /* v25pi */ }
<NEC>"cnvtrp"	{ yylval.i = MNEM_CNVTRP; return TOK_MNEMONIC; /* v25pi */ }
<NEC>"coltrp"	{ yylval.i = MNEM_COLTRP; return TOK_MNEMONIC; /* v25pi */ }
<NEC>"getbit"	{ yylval.i = MNEM_GETBIT; return TOK_MNEMONIC; /* v25pi */ }
<NEC>"mhdec"	{ yylval.i = MNEM_MHDEC; return TOK_MNEMONIC; /* v25pi */ }
<NEC>"mhenc"	{ yylval.i = MNEM_MHENC; return TOK_MNEMONIC; /* v25pi */ }
<NEC>"mrdec"	{ yylval.i = MNEM_MRDEC; return TOK_MNEMONIC; /* v25pi */ }
<NEC>"mrenc"	{ yylval.i = MNEM_MRENC; return TOK_MNEMONIC; /* v25pi */ }
<NEC>"scheol"	{ yylval.i = MNEM_SCHEOL; return TOK_MNEMONIC; /* v25pi */ }

<INTEL>"rsdc"	{ yylval.i = MNEM_RSDC; return TOK_MNEMONIC; /* Cyrix */ }
<INTEL>"rsldt"	{ yylval.i = MNEM_RSLDT; return TOK_MNEMONIC; /* Cyrix */ }
<INTEL>"rsts"	{ yylval.i = MNEM_RSTS; return TOK_MNEMONIC; /* Cyrix */ }
<INTEL>"smint"	{ yylval.i = MNEM_SMINT; return TOK_MNEMONIC; /* Cyrix */ }
<INTEL>"svdc"	{ yylval.i = MNEM_SVDC; return TOK_MNEMONIC; /* Cyrix */ }
<INTEL>"svldt"	{ yylval.i = MNEM_SVLDT; return TOK_MNEMONIC; /* Cyrix */ }
<INTEL>"svts"	{ yylval.i = MNEM_SVTS; return TOK_MNEMONIC; /* Cyrix */ }
<INTEL>"rdshr"	{ yylval.i = MNEM_RDSHR; return TOK_MNEMONIC; /* Cyrix 6x86MX */ }
<INTEL>"wrshr"	{ yylval.i = MNEM_WRSHR; return TOK_MNEMONIC; /* Cyrix 6x86MX */ }
<INTEL>"bb0_reset"	{ yylval.i = MNEM_BB0_RESET; return TOK_MNEMONIC; /* MediaGX */ }
<INTEL>"bb1_reset"	{ yylval.i = MNEM_BB1_RESET; return TOK_MNEMONIC; /* MediaGX */ }
<INTEL>"cpu_read"	{ yylval.i = MNEM_CPU_READ; return TOK_MNEMONIC; /* MediaGX */ }
<INTEL>"cpu_write"	{ yylval.i = MNEM_CPU_WRITE; return TOK_MNEMONIC; /* MediaGX */ }
<INTEL>"dmint"	{ yylval.i = MNEM_DMINT; return TOK_MNEMONIC; /* Geode GX2 */ }
<INTEL>"rdm"	{ yylval.i = MNEM_RDM; return TOK_MNEMONIC; /* Geode GX2 */ }

<INTEL,NEC>f2xm1	{ yylval.i = MNEM_F2XM1; return TOK_MNEMONIC; }
<INTEL,NEC>fabs	{ yylval.i = MNEM_FABS; return TOK_MNEMONIC; }
<INTEL,NEC>faddp	{ yylval.i = MNEM_FADDP; return TOK_MNEMONIC; }
<INTEL,NEC>fadd	{ yylval.i = MNEM_FADD; return TOK_MNEMONIC; }
<INTEL,NEC>fbld	{ yylval.i = MNEM_FBLD; return TOK_MNEMONIC; }
<INTEL,NEC>fbstp	{ yylval.i = MNEM_FBSTP; return TOK_MNEMONIC; }
<INTEL,NEC>fchs	{ yylval.i = MNEM_FCHS; return TOK_MNEMONIC; }
<INTEL,NEC>fclex	{ yylval.i = MNEM_FCLEX; return TOK_MNEMONIC; }
<INTEL,NEC>fcompp	{ yylval.i = MNEM_FCOMPP; return TOK_MNEMONIC; }
<INTEL,NEC>fcomp	{ yylval.i = MNEM_FCOMP; return TOK_MNEMONIC; }
<INTEL,NEC>fcom	{ yylval.i = MNEM_FCOM; return TOK_MNEMONIC; }
<INTEL,NEC>fdecstp	{ yylval.i = MNEM_FDECSTP; return TOK_MNEMONIC; }
<INTEL,NEC>fdisi	{ yylval.i = MNEM_FDISI; return TOK_MNEMONIC; }
<INTEL,NEC>fdivp	{ yylval.i = MNEM_FDIVP; return TOK_MNEMONIC; }
<INTEL,NEC>fdivrp	{ yylval.i = MNEM_FDIVRP; return TOK_MNEMONIC; }
<INTEL,NEC>fdivr	{ yylval.i = MNEM_FDIVR; return TOK_MNEMONIC; }
<INTEL,NEC>fdiv	{ yylval.i = MNEM_FDIV; return TOK_MNEMONIC; }
<INTEL,NEC>feni	{ yylval.i = MNEM_FENI; return TOK_MNEMONIC; }
<INTEL,NEC>ffreep	{ yylval.i = MNEM_FFREEP; return TOK_MNEMONIC; }
<INTEL,NEC>ffree	{ yylval.i = MNEM_FFREE; return TOK_MNEMONIC; }
<INTEL,NEC>fiadd	{ yylval.i = MNEM_FIADD; return TOK_MNEMONIC; }
<INTEL,NEC>ficomp	{ yylval.i = MNEM_FICOMP; return TOK_MNEMONIC; }
<INTEL,NEC>ficom	{ yylval.i = MNEM_FICOM; return TOK_MNEMONIC; }
<INTEL,NEC>fidivr	{ yylval.i = MNEM_FIDIVR; return TOK_MNEMONIC; }
<INTEL,NEC>fidiv	{ yylval.i = MNEM_FIDIV; return TOK_MNEMONIC; }
<INTEL,NEC>fild	{ yylval.i = MNEM_FILD; return TOK_MNEMONIC; }
<INTEL,NEC>fimul	{ yylval.i = MNEM_FIMUL; return TOK_MNEMONIC; }
<INTEL,NEC>fincstp	{ yylval.i = MNEM_FINCSTP; return TOK_MNEMONIC; }
<INTEL,NEC>finit	{ yylval.i = MNEM_FINIT; return TOK_MNEMONIC; }
<INTEL,NEC>fistp	{ yylval.i = MNEM_FISTP; return TOK_MNEMONIC; }
<INTEL,NEC>fist	{ yylval.i = MNEM_FIST; return TOK_MNEMONIC; }
<INTEL,NEC>fisubr	{ yylval.i = MNEM_FISUBR; return TOK_MNEMONIC; }
<INTEL,NEC>fisub	{ yylval.i = MNEM_FISUB; return TOK_MNEMONIC; }
<INTEL,NEC>fld1	{ yylval.i = MNEM_FLD1; return TOK_MNEMONIC; }
<INTEL,NEC>fldcw	{ yylval.i = MNEM_FLDCW; return TOK_MNEMONIC; }
<INTEL,NEC>fldenv	{ yylval.i = MNEM_FLDENV; return TOK_MNEMONIC; }
<INTEL,NEC>fldl2e	{ yylval.i = MNEM_FLDL2E; return TOK_MNEMONIC; }
<INTEL,NEC>fldl2t	{ yylval.i = MNEM_FLDL2T; return TOK_MNEMONIC; }
<INTEL,NEC>fldlg2	{ yylval.i = MNEM_FLDLG2; return TOK_MNEMONIC; }
<INTEL,NEC>fldln2	{ yylval.i = MNEM_FLDLN2; return TOK_MNEMONIC; }
<INTEL,NEC>fldpi	{ yylval.i = MNEM_FLDPI; return TOK_MNEMONIC; }
<INTEL,NEC>fld	{ yylval.i = MNEM_FLD; return TOK_MNEMONIC; }
<INTEL,NEC>fldz	{ yylval.i = MNEM_FLDZ; return TOK_MNEMONIC; }
<INTEL,NEC>fmulp	{ yylval.i = MNEM_FMULP; return TOK_MNEMONIC; }
<INTEL,NEC>fmul	{ yylval.i = MNEM_FMUL; return TOK_MNEMONIC; }
<INTEL,NEC>fnclex	{ yylval.i = MNEM_FNCLEX; return TOK_MNEMONIC; }
<INTEL,NEC>fndisi	{ yylval.i = MNEM_FNDISI; return TOK_MNEMONIC; }
<INTEL,NEC>fneni	{ yylval.i = MNEM_FNENI; return TOK_MNEMONIC; }
<INTEL,NEC>fninit	{ yylval.i = MNEM_FNINIT; return TOK_MNEMONIC; }
<INTEL,NEC>fnop	{ yylval.i = MNEM_FNOP; return TOK_MNEMONIC; }
<INTEL,NEC>fnsave	{ yylval.i = MNEM_FNSAVE; return TOK_MNEMONIC; }
<INTEL,NEC>fnstcw	{ yylval.i = MNEM_FNSTCW; return TOK_MNEMONIC; }
<INTEL,NEC>fnstenv	{ yylval.i = MNEM_FNSTENV; return TOK_MNEMONIC; }
<INTEL,NEC>fnstsw	{ yylval.i = MNEM_FNSTSW; return TOK_MNEMONIC; }
<INTEL,NEC>fpatan	{ yylval.i = MNEM_FPATAN; return TOK_MNEMONIC; }
<INTEL,NEC>fprem	{ yylval.i = MNEM_FPREM; return TOK_MNEMONIC; }
<INTEL,NEC>fptan	{ yylval.i = MNEM_FPTAN; return TOK_MNEMONIC; }
<INTEL,NEC>frndint	{ yylval.i = MNEM_FRNDINT; return TOK_MNEMONIC; }
<INTEL,NEC>frstor	{ yylval.i = MNEM_FRSTOR; return TOK_MNEMONIC; }
<INTEL,NEC>fsave	{ yylval.i = MNEM_FSAVE; return TOK_MNEMONIC; }
<INTEL,NEC>fscale	{ yylval.i = MNEM_FSCALE; return TOK_MNEMONIC; }
<INTEL,NEC>fsqrt	{ yylval.i = MNEM_FSQRT; return TOK_MNEMONIC; }
<INTEL,NEC>fstcw	{ yylval.i = MNEM_FSTCW; return TOK_MNEMONIC; }
<INTEL,NEC>fstenv	{ yylval.i = MNEM_FSTENV; return TOK_MNEMONIC; }
<INTEL,NEC>fstpnce	{ yylval.i = MNEM_FSTPNCE; return TOK_MNEMONIC; }
<INTEL,NEC>fstp	{ yylval.i = MNEM_FSTP; return TOK_MNEMONIC; }
<INTEL,NEC>fstsw	{ yylval.i = MNEM_FSTSW; return TOK_MNEMONIC; }
<INTEL,NEC>fst	{ yylval.i = MNEM_FST; return TOK_MNEMONIC; }
<INTEL,NEC>fsubp	{ yylval.i = MNEM_FSUBP; return TOK_MNEMONIC; }
<INTEL,NEC>fsubrp	{ yylval.i = MNEM_FSUBRP; return TOK_MNEMONIC; }
<INTEL,NEC>fsubr	{ yylval.i = MNEM_FSUBR; return TOK_MNEMONIC; }
<INTEL,NEC>fsub	{ yylval.i = MNEM_FSUB; return TOK_MNEMONIC; }
<INTEL,NEC>ftst	{ yylval.i = MNEM_FTST; return TOK_MNEMONIC; }
<INTEL,NEC>fxam	{ yylval.i = MNEM_FXAM; return TOK_MNEMONIC; }
<INTEL,NEC>fxch	{ yylval.i = MNEM_FXCH; return TOK_MNEMONIC; }
<INTEL,NEC>fxtract	{ yylval.i = MNEM_FXTRACT; return TOK_MNEMONIC; }
<INTEL,NEC>fyl2xp1	{ yylval.i = MNEM_FYL2XP1; return TOK_MNEMONIC; }
<INTEL,NEC>fyl2x	{ yylval.i = MNEM_FYL2X; return TOK_MNEMONIC; }

<INTEL,NEC>fnsetpm	{ yylval.i = MNEM_FNSETPM; return TOK_MNEMONIC; }
<INTEL,NEC>fsetpm	{ yylval.i = MNEM_FSETPM; return TOK_MNEMONIC; }

<INTEL,NEC>fcos	{ yylval.i = MNEM_FCOS; return TOK_MNEMONIC; }
<INTEL,NEC>fprem1	{ yylval.i = MNEM_FPREM1; return TOK_MNEMONIC; }
<INTEL,NEC>fsincos	{ yylval.i = MNEM_FSINCOS; return TOK_MNEMONIC; }
<INTEL,NEC>fsin	{ yylval.i = MNEM_FSIN; return TOK_MNEMONIC; }
<INTEL,NEC>fucompp	{ yylval.i = MNEM_FUCOMPP; return TOK_MNEMONIC; }
<INTEL,NEC>fucomp	{ yylval.i = MNEM_FUCOMP; return TOK_MNEMONIC; }
<INTEL,NEC>fucom	{ yylval.i = MNEM_FUCOM; return TOK_MNEMONIC; }

<INTEL,NEC>fcmov(b|e|be|u|nb|ne|nbe|nu)	{ yylval.i = _COND_MNEM(parse_cond_x87(yytext + 5), MNEM_FCMOV_CC); return TOK_MNEMONIC; }
<INTEL,NEC>fcomip	{ yylval.i = MNEM_FCOMIP; return TOK_MNEMONIC; }
<INTEL,NEC>fcomi	{ yylval.i = MNEM_FCOMI; return TOK_MNEMONIC; }
<INTEL,NEC>fucomip	{ yylval.i = MNEM_FUCOMIP; return TOK_MNEMONIC; }
<INTEL,NEC>fucomi	{ yylval.i = MNEM_FUCOMI; return TOK_MNEMONIC; }

<INTEL,NEC>frstpm	{ yylval.i = MNEM_FRSTPM; return TOK_MNEMONIC; }
<INTEL,NEC>fnstdw	{ yylval.i = MNEM_FNSTDW; return TOK_MNEMONIC; }
<INTEL,NEC>fnstsg	{ yylval.i = MNEM_FNSTSG; return TOK_MNEMONIC; }
<INTEL,NEC>fstdw	{ yylval.i = MNEM_FSTDW; return TOK_MNEMONIC; }
<INTEL,NEC>fstsg	{ yylval.i = MNEM_FSTSG; return TOK_MNEMONIC; }

<INTEL,NEC>fisttp	{ yylval.i = MNEM_FISTTP; return TOK_MNEMONIC; }

<INTEL>".o16"	{ yylval.i = BITSIZE16; return TOK_OSIZE; }
<INTEL>".o32"	{ yylval.i = BITSIZE32; return TOK_OSIZE; }
<INTEL>".o64"	{ yylval.i = BITSIZE64; return TOK_OSIZE; }

<NEC>psw	{ return TOK_PSWREG; /* NEC */ }

<INTEL,NEC>rep	{ yylval.i = PREF_REP; return TOK_REP; }
<INTEL,NEC>rep[ze]	{ yylval.i = PREF_REPE; return TOK_REP; }
<INTEL,NEC>repn[ze]	{ yylval.i = PREF_REPNE; return TOK_REP; }
<NEC>repc	{ yylval.i = PREF_REPC; return TOK_REP; /* NEC */ }
<NEC>repnc	{ yylval.i = PREF_REPNC; return TOK_REP; /* NEC */ }

<NEC>r	{ return TOK_RSYMBOL; /* NEC */ }

<INTEL>[ecsdfg]s	{ yylval.i = segord(yytext[0]); return TOK_SEG; }
<NEC>[ps]s	{ yylval.i = necseg0ord(yytext[0]); return TOK_SEG; /* NEC */ }
<NEC>ds[0-3]	{ yylval.i = necseg1ord(yytext[2]); return TOK_SEG; /* NEC */ }

<INTEL,NEC>st	{ yylval.i = 0; return TOK_STREG; }
<INTEL,NEC>st[0-7]	{ yylval.i = yytext[2] - '0'; return TOK_STREG; }
<INTEL,NEC>st\([0-7]\)	{ yylval.i = yytext[3] - '0'; return TOK_STREG; }

<INTEL>tr[0-7]	{ yylval.i = yytext[2] - '0'; return TOK_TREG; }

<INTEL>xmm([12]?[0-9]|3[01])	{ yylval.i = strtol(yytext + 3, NULL, 10); return TOK_XMMREG; }
<INTEL>ymm([12]?[0-9]|3[01])	{ yylval.i = strtol(yytext + 3, NULL, 10); return TOK_YMMREG; }
<INTEL>zmm([12]?[0-9]|3[01])	{ yylval.i = strtol(yytext + 3, NULL, 10); return TOK_ZMMREG; }

<I8080,Z80>a	{ yylval.i = X80_A; return TOK_X80_REG; }
<I8080,Z80>b	{ yylval.i = X80_B; return TOK_X80_MEM; }
<I8080>c	{ yylval.i = X80_C; return TOK_X80_REG; }
<Z80>c	{ yylval.i = X80_C; return TOK_X80_MEM; }
<I8080,Z80>d	{ yylval.i = X80_D; return TOK_X80_REG; }
<I8080,Z80>e	{ yylval.i = X80_E; return TOK_X80_REG; }
<I8080,Z80>h	{ yylval.i = X80_H; return TOK_X80_REG; }
<I8080,Z80>l	{ yylval.i = X80_L; return TOK_X80_REG; }
<I8080>m	{ yylval.i = X80_REG_M; return TOK_X80_REG; }
<Z80>ixh	{ yylval.i = X80_IXH; return TOK_X80_REG; }
<Z80>ixl	{ yylval.i = X80_IXL; return TOK_X80_REG; }
<Z80>iyh	{ yylval.i = X80_IYH; return TOK_X80_REG; }
<Z80>iyl	{ yylval.i = X80_IYL; return TOK_X80_REG; }
<Z80>i	{ yylval.i = X80_I; return TOK_X80_REG; }
<Z80>r	{ yylval.i = X80_R; return TOK_X80_REG; }

<Z80>bc	{ yylval.i = X80_BC; return TOK_X80_MEM; }
<Z80>de	{ yylval.i = X80_DE; return TOK_X80_MEM; }
<Z80>hl	{ yylval.i = X80_HL; return TOK_X80_MEM; }
<Z80>ix	{ yylval.i = X80_IX; return TOK_X80_IDX; }
<Z80>iy	{ yylval.i = X80_IY; return TOK_X80_IDX; }
<I8080,Z80>sp	{ yylval.i = X80_SP; return TOK_X80_MEM; }
<I8080>psw	{ yylval.i = X80_PSW; return TOK_X80_REG; }
<Z80>af	{ yylval.i = X80_AF; return TOK_X80_REG; }
<Z80>"af'"	{ yylval.i = X80_AF2; return TOK_X80_REG; }

<Z80>nz	{ yylval.i = X80_NZ; return TOK_X80_REG; }
<Z80>z	{ yylval.i = X80_Z; return TOK_X80_REG; }
<Z80>nc	{ yylval.i = X80_NC; return TOK_X80_REG; }
<Z80>po	{ yylval.i = X80_PO; return TOK_X80_REG; }
<Z80>pe	{ yylval.i = X80_PE; return TOK_X80_REG; }
<Z80>p	{ yylval.i = X80_P; return TOK_X80_REG; }
<Z80>m	{ yylval.i = X80_CND_M; return TOK_X80_REG; }

<I8080>aci	{ yylval.i = MNEM_I8080_ACI; return TOK_I8080_MNEM; }
<I8080>adc	{ yylval.i = MNEM_I8080_ADC; return TOK_I8080_MNEM; }
<I8080>add	{ yylval.i = MNEM_I8080_ADD; return TOK_I8080_MNEM; }
<I8080>adi	{ yylval.i = MNEM_I8080_ADI; return TOK_I8080_MNEM; }
<I8080>ana	{ yylval.i = MNEM_I8080_ANA; return TOK_I8080_MNEM; }
<I8080>ani	{ yylval.i = MNEM_I8080_ANI; return TOK_I8080_MNEM; }
<I8080>call	{ yylval.i = MNEM_I8080_CALL; return TOK_I8080_MNEM; }
<I8080>c{X80_COND}	{ yylval.i = _COND_MNEM(parse_cond_x80(yytext + 1), MNEM_I8080_C_CC); return TOK_I8080_MNEM; }
<I8080>cma	{ yylval.i = MNEM_I8080_CMA; return TOK_I8080_MNEM; }
<I8080>cmc	{ yylval.i = MNEM_I8080_CMC; return TOK_I8080_MNEM; }
<I8080>cmp	{ yylval.i = MNEM_I8080_CMP; return TOK_I8080_MNEM; }
<I8080>cpi	{ yylval.i = MNEM_I8080_CPI; return TOK_I8080_MNEM; }
<I8080>daa	{ yylval.i = MNEM_I8080_DAA; return TOK_I8080_MNEM; }
<I8080>dad	{ yylval.i = MNEM_I8080_DAD; return TOK_I8080_MNEM; }
<I8080>dcr	{ yylval.i = MNEM_I8080_DCR; return TOK_I8080_MNEM; }
<I8080>dcx	{ yylval.i = MNEM_I8080_DCX; return TOK_I8080_MNEM; }
<I8080>di	{ yylval.i = MNEM_I8080_DI; return TOK_I8080_MNEM; }
<I8080>ei	{ yylval.i = MNEM_I8080_EI; return TOK_I8080_MNEM; }
<I8080>hlt	{ yylval.i = MNEM_I8080_HLT; return TOK_I8080_MNEM; }
<I8080>in	{ yylval.i = MNEM_I8080_IN; return TOK_I8080_MNEM; }
<I8080>inr	{ yylval.i = MNEM_I8080_INR; return TOK_I8080_MNEM; }
<I8080>inx	{ yylval.i = MNEM_I8080_INX; return TOK_I8080_MNEM; }
<I8080>j{X80_COND}	{ yylval.i = _COND_MNEM(parse_cond_x80(yytext + 1), MNEM_I8080_J_CC); return TOK_I8080_MNEM; }
<I8080>jmp	{ yylval.i = MNEM_I8080_JMP; return TOK_I8080_MNEM; }
<I8080>lda	{ yylval.i = MNEM_I8080_LDA; return TOK_I8080_MNEM; }
<I8080>ldax	{ yylval.i = MNEM_I8080_LDAX; return TOK_I8080_MNEM; }
<I8080>lhld	{ yylval.i = MNEM_I8080_LHLD; return TOK_I8080_MNEM; }
<I8080>lxi	{ yylval.i = MNEM_I8080_LXI; return TOK_I8080_MNEM; }
<I8080>mov	{ yylval.i = MNEM_I8080_MOV; return TOK_I8080_MNEM; }
<I8080>mvi	{ yylval.i = MNEM_I8080_MVI; return TOK_I8080_MNEM; }
<I8080>nop	{ yylval.i = MNEM_I8080_NOP; return TOK_I8080_MNEM; }
<I8080>ora	{ yylval.i = MNEM_I8080_ORA; return TOK_I8080_MNEM; }
<I8080>ori	{ yylval.i = MNEM_I8080_ORI; return TOK_I8080_MNEM; }
<I8080>out	{ yylval.i = MNEM_I8080_OUT; return TOK_I8080_MNEM; }
<I8080>pchl	{ yylval.i = MNEM_I8080_PCHL; return TOK_I8080_MNEM; }
<I8080>pop	{ yylval.i = MNEM_I8080_POP; return TOK_I8080_MNEM; }
<I8080>push	{ yylval.i = MNEM_I8080_PUSH; return TOK_I8080_MNEM; }
<I8080>ral	{ yylval.i = MNEM_I8080_RAL; return TOK_I8080_MNEM; }
<I8080>rar	{ yylval.i = MNEM_I8080_RAR; return TOK_I8080_MNEM; }
<I8080>r{X80_COND}	{ yylval.i = _COND_MNEM(parse_cond_x80(yytext + 1), MNEM_I8080_R_CC); return TOK_I8080_MNEM; }
<I8080>ret	{ yylval.i = MNEM_I8080_RET; return TOK_I8080_MNEM; }
<I8080>rlc	{ yylval.i = MNEM_I8080_RLC; return TOK_I8080_MNEM; }
<I8080>rrc	{ yylval.i = MNEM_I8080_RRC; return TOK_I8080_MNEM; }
<I8080>rst	{ yylval.i = MNEM_I8080_RST; return TOK_I8080_MNEM; }
<I8080>sbb	{ yylval.i = MNEM_I8080_SBB; return TOK_I8080_MNEM; }
<I8080>sbi	{ yylval.i = MNEM_I8080_SBI; return TOK_I8080_MNEM; }
<I8080>shld	{ yylval.i = MNEM_I8080_SHLD; return TOK_I8080_MNEM; }
<I8080>sphl	{ yylval.i = MNEM_I8080_SPHL; return TOK_I8080_MNEM; }
<I8080>sta	{ yylval.i = MNEM_I8080_STA; return TOK_I8080_MNEM; }
<I8080>stax	{ yylval.i = MNEM_I8080_STAX; return TOK_I8080_MNEM; }
<I8080>stc	{ yylval.i = MNEM_I8080_STC; return TOK_I8080_MNEM; }
<I8080>sub	{ yylval.i = MNEM_I8080_SUB; return TOK_I8080_MNEM; }
<I8080>sui	{ yylval.i = MNEM_I8080_SUI; return TOK_I8080_MNEM; }
<I8080>xchg	{ yylval.i = MNEM_I8080_XCHG; return TOK_I8080_MNEM; }
<I8080>xra	{ yylval.i = MNEM_I8080_XRA; return TOK_I8080_MNEM; }
<I8080>xri	{ yylval.i = MNEM_I8080_XRI; return TOK_I8080_MNEM; }
<I8080>xthl	{ yylval.i = MNEM_I8080_XTHL; return TOK_I8080_MNEM; }

<Z80>adc	{ yylval.i = MNEM_Z80_ADC; return TOK_Z80_MNEM; }
<Z80>add	{ yylval.i = MNEM_Z80_ADD; return TOK_Z80_MNEM; }
<Z80>and	{ yylval.i = MNEM_Z80_AND; return TOK_Z80_MNEM; }
<Z80>bit	{ yylval.i = MNEM_Z80_BIT; return TOK_Z80_MNEM; }
<Z80>call	{ yylval.i = MNEM_Z80_CALL; return TOK_Z80_MNEM; }
<Z80>ccf	{ yylval.i = MNEM_Z80_CCF; return TOK_Z80_MNEM; }
<Z80>cp	{ yylval.i = MNEM_Z80_CP; return TOK_Z80_MNEM; }
<Z80>cpd	{ yylval.i = MNEM_Z80_CPD; return TOK_Z80_MNEM; }
<Z80>cpdr	{ yylval.i = MNEM_Z80_CPDR; return TOK_Z80_MNEM; }
<Z80>cpi	{ yylval.i = MNEM_Z80_CPI; return TOK_Z80_MNEM; }
<Z80>cpir	{ yylval.i = MNEM_Z80_CPIR; return TOK_Z80_MNEM; }
<Z80>cpl	{ yylval.i = MNEM_Z80_CPL; return TOK_Z80_MNEM; }
<Z80>daa	{ yylval.i = MNEM_Z80_DAA; return TOK_Z80_MNEM; }
<Z80>dec	{ yylval.i = MNEM_Z80_DEC; return TOK_Z80_MNEM; }
<Z80>di	{ yylval.i = MNEM_Z80_DI; return TOK_Z80_MNEM; }
<Z80>djnz	{ yylval.i = MNEM_Z80_DJNZ; return TOK_Z80_MNEM; }
<Z80>ei	{ yylval.i = MNEM_Z80_EI; return TOK_Z80_MNEM; }
<Z80>ex	{ yylval.i = MNEM_Z80_EX; return TOK_Z80_MNEM; }
<Z80>exx	{ yylval.i = MNEM_Z80_EXX; return TOK_Z80_MNEM; }
<Z80>halt	{ yylval.i = MNEM_Z80_HALT; return TOK_Z80_MNEM; }
<Z80>im	{ yylval.i = MNEM_Z80_IM; return TOK_Z80_MNEM; }
<Z80>in	{ yylval.i = MNEM_Z80_IN; return TOK_Z80_MNEM; }
<Z80>inc	{ yylval.i = MNEM_Z80_INC; return TOK_Z80_MNEM; }
<Z80>ind	{ yylval.i = MNEM_Z80_IND; return TOK_Z80_MNEM; }
<Z80>indr	{ yylval.i = MNEM_Z80_INDR; return TOK_Z80_MNEM; }
<Z80>ini	{ yylval.i = MNEM_Z80_INI; return TOK_Z80_MNEM; }
<Z80>inir	{ yylval.i = MNEM_Z80_INIR; return TOK_Z80_MNEM; }
<Z80>jp	{ yylval.i = MNEM_Z80_JP; return TOK_Z80_MNEM; }
<Z80>jr	{ yylval.i = MNEM_Z80_JR; return TOK_Z80_MNEM; }
<Z80>ld	{ yylval.i = MNEM_Z80_LD; return TOK_Z80_MNEM; }
<Z80>ldd	{ yylval.i = MNEM_Z80_LDD; return TOK_Z80_MNEM; }
<Z80>lddr	{ yylval.i = MNEM_Z80_LDDR; return TOK_Z80_MNEM; }
<Z80>ldi	{ yylval.i = MNEM_Z80_LDI; return TOK_Z80_MNEM; }
<Z80>ldir	{ yylval.i = MNEM_Z80_LDIR; return TOK_Z80_MNEM; }
<Z80>neg	{ yylval.i = MNEM_Z80_NEG; return TOK_Z80_MNEM; }
<Z80>nop	{ yylval.i = MNEM_Z80_NOP; return TOK_Z80_MNEM; }
<Z80>or	{ yylval.i = MNEM_Z80_OR; return TOK_Z80_MNEM; }
<Z80>otdr	{ yylval.i = MNEM_Z80_OTDR; return TOK_Z80_MNEM; }
<Z80>otir	{ yylval.i = MNEM_Z80_OTIR; return TOK_Z80_MNEM; }
<Z80>out	{ yylval.i = MNEM_Z80_OUT; return TOK_Z80_MNEM; }
<Z80>outd	{ yylval.i = MNEM_Z80_OUTD; return TOK_Z80_MNEM; }
<Z80>outi	{ yylval.i = MNEM_Z80_OUTI; return TOK_Z80_MNEM; }
<Z80>pop	{ yylval.i = MNEM_Z80_POP; return TOK_Z80_MNEM; }
<Z80>push	{ yylval.i = MNEM_Z80_PUSH; return TOK_Z80_MNEM; }
<Z80>res	{ yylval.i = MNEM_Z80_RES; return TOK_Z80_MNEM; }
<Z80>ret	{ yylval.i = MNEM_Z80_RET; return TOK_Z80_MNEM; }
<Z80>reti	{ yylval.i = MNEM_Z80_RETI; return TOK_Z80_MNEM; }
<Z80>retn	{ yylval.i = MNEM_Z80_RETN; return TOK_Z80_MNEM; }
<Z80>rl	{ yylval.i = MNEM_Z80_RL; return TOK_Z80_MNEM; }
<Z80>rla	{ yylval.i = MNEM_Z80_RLA; return TOK_Z80_MNEM; }
<Z80>rlc	{ yylval.i = MNEM_Z80_RLC; return TOK_Z80_MNEM; }
<Z80>rlca	{ yylval.i = MNEM_Z80_RLCA; return TOK_Z80_MNEM; }
<Z80>rld	{ yylval.i = MNEM_Z80_RLD; return TOK_Z80_MNEM; }
<Z80>rr	{ yylval.i = MNEM_Z80_RR; return TOK_Z80_MNEM; }
<Z80>rra	{ yylval.i = MNEM_Z80_RRA; return TOK_Z80_MNEM; }
<Z80>rrc	{ yylval.i = MNEM_Z80_RRC; return TOK_Z80_MNEM; }
<Z80>rrca	{ yylval.i = MNEM_Z80_RRCA; return TOK_Z80_MNEM; }
<Z80>rrd	{ yylval.i = MNEM_Z80_RRD; return TOK_Z80_MNEM; }
<Z80>rst	{ yylval.i = MNEM_Z80_RST; return TOK_Z80_MNEM; }
<Z80>sbc	{ yylval.i = MNEM_Z80_SBC; return TOK_Z80_MNEM; }
<Z80>scf	{ yylval.i = MNEM_Z80_SCF; return TOK_Z80_MNEM; }
<Z80>set	{ yylval.i = MNEM_Z80_SET; return TOK_Z80_MNEM; }
<Z80>sla	{ yylval.i = MNEM_Z80_SLA; return TOK_Z80_MNEM; }
<Z80>sl[1li]	{ yylval.i = MNEM_Z80_SLL; return TOK_Z80_MNEM; }
<Z80>sra	{ yylval.i = MNEM_Z80_SRA; return TOK_Z80_MNEM; }
<Z80>srl	{ yylval.i = MNEM_Z80_SRL; return TOK_Z80_MNEM; }
<Z80>sub	{ yylval.i = MNEM_Z80_SUB; return TOK_Z80_MNEM; }
<Z80>xor	{ yylval.i = MNEM_Z80_XOR; return TOK_Z80_MNEM; }

<I8080>calln	{ yylval.i = MNEM_Z80_CALLN; return TOK_I8080_MNEM; }
<Z80>calln	{ yylval.i = MNEM_Z80_CALLN; return TOK_Z80_MNEM; }
<I8080>retem	{ yylval.i = MNEM_Z80_RETEM; return TOK_I8080_MNEM; }
<Z80>retem	{ yylval.i = MNEM_Z80_RETEM; return TOK_Z80_MNEM; }

<I8089>ga	{ yylval.i = 0; return TOK_I8089_PTRREG; }
<I8089>gb	{ yylval.i = 1; return TOK_I8089_PTRREG; }
<I8089>gc	{ yylval.i = 2; return TOK_I8089_PTRREG; }
<I8089>bc	{ yylval.i = 3; return TOK_I8089_VALREG; }
<I8089>pp	{ yylval.i = 3; return TOK_I8089_PPREG; }
<I8089>tp	{ yylval.i = 4; return TOK_I8089_VALREG; }
<I8089>ix	{ yylval.i = 5; return TOK_I8089_IXREG; }
<I8089>cc	{ yylval.i = 6; return TOK_I8089_VALREG; }
<I8089>mc	{ yylval.i = 7; return TOK_I8089_VALREG; }

<I8089>addbi	{ yylval.i = MNEM_I8089_ADDBI; return TOK_I8089_MNEM; }
<I8089>addb	{ yylval.i = MNEM_I8089_ADDB; return TOK_I8089_MNEM; }
<I8089>addi	{ yylval.i = MNEM_I8089_ADDI; return TOK_I8089_MNEM; }
<I8089>add	{ yylval.i = MNEM_I8089_ADD; return TOK_I8089_MNEM; }
<I8089>andbi	{ yylval.i = MNEM_I8089_ANDBI; return TOK_I8089_MNEM; }
<I8089>andb	{ yylval.i = MNEM_I8089_ANDB; return TOK_I8089_MNEM; }
<I8089>andi	{ yylval.i = MNEM_I8089_ANDI; return TOK_I8089_MNEM; }
<I8089>and	{ yylval.i = MNEM_I8089_AND; return TOK_I8089_MNEM; }
<I8089>call	{ yylval.i = MNEM_I8089_CALL; return TOK_I8089_MNEM; }
<I8089>clr	{ yylval.i = MNEM_I8089_CLR; return TOK_I8089_MNEM; }
<I8089>decb	{ yylval.i = MNEM_I8089_DECB; return TOK_I8089_MNEM; }
<I8089>dec	{ yylval.i = MNEM_I8089_DEC; return TOK_I8089_MNEM; }
<I8089>hlt	{ yylval.i = MNEM_I8089_HLT; return TOK_I8089_MNEM; }
<I8089>incb	{ yylval.i = MNEM_I8089_INCB; return TOK_I8089_MNEM; }
<I8089>inc	{ yylval.i = MNEM_I8089_INC; return TOK_I8089_MNEM; }
<I8089>jbt	{ yylval.i = MNEM_I8089_JBT; return TOK_I8089_MNEM; }
<I8089>jmce	{ yylval.i = MNEM_I8089_JMCE; return TOK_I8089_MNEM; }
<I8089>jmcne	{ yylval.i = MNEM_I8089_JMCNE; return TOK_I8089_MNEM; }
<I8089>jmp	{ yylval.i = MNEM_I8089_JMP; return TOK_I8089_MNEM; }
<I8089>jnbt	{ yylval.i = MNEM_I8089_JNBT; return TOK_I8089_MNEM; }
<I8089>jnzb	{ yylval.i = MNEM_I8089_JNZB; return TOK_I8089_MNEM; }
<I8089>jnz	{ yylval.i = MNEM_I8089_JNZ; return TOK_I8089_MNEM; }
<I8089>jzb	{ yylval.i = MNEM_I8089_JZB; return TOK_I8089_MNEM; }
<I8089>jz	{ yylval.i = MNEM_I8089_JZ; return TOK_I8089_MNEM; }
<I8089>lcall	{ yylval.i = MNEM_I8089_LCALL; return TOK_I8089_MNEM; }
<I8089>ljbt	{ yylval.i = MNEM_I8089_LJBT; return TOK_I8089_MNEM; }
<I8089>ljmce	{ yylval.i = MNEM_I8089_LJMCE; return TOK_I8089_MNEM; }
<I8089>ljmcne	{ yylval.i = MNEM_I8089_LJMCNE; return TOK_I8089_MNEM; }
<I8089>ljmp	{ yylval.i = MNEM_I8089_LJMP; return TOK_I8089_MNEM; }
<I8089>ljnbt	{ yylval.i = MNEM_I8089_LJNBT; return TOK_I8089_MNEM; }
<I8089>ljnzb	{ yylval.i = MNEM_I8089_LJNZB; return TOK_I8089_MNEM; }
<I8089>ljnz	{ yylval.i = MNEM_I8089_LJNZ; return TOK_I8089_MNEM; }
<I8089>ljzb	{ yylval.i = MNEM_I8089_LJZB; return TOK_I8089_MNEM; }
<I8089>ljz	{ yylval.i = MNEM_I8089_LJZ; return TOK_I8089_MNEM; }
<I8089>lpdi	{ yylval.i = MNEM_I8089_LPDI; return TOK_I8089_MNEM; }
<I8089>lpd	{ yylval.i = MNEM_I8089_LPD; return TOK_I8089_MNEM; }
<I8089>movbi	{ yylval.i = MNEM_I8089_MOVBI; return TOK_I8089_MNEM; }
<I8089>movb	{ yylval.i = MNEM_I8089_MOVB; return TOK_I8089_MNEM; }
<I8089>movi	{ yylval.i = MNEM_I8089_MOVI; return TOK_I8089_MNEM; }
<I8089>movp	{ yylval.i = MNEM_I8089_MOVP; return TOK_I8089_MNEM; }
<I8089>mov	{ yylval.i = MNEM_I8089_MOV; return TOK_I8089_MNEM; }
<I8089>nop	{ yylval.i = MNEM_I8089_NOP; return TOK_I8089_MNEM; }
<I8089>notb	{ yylval.i = MNEM_I8089_NOTB; return TOK_I8089_MNEM; }
<I8089>not	{ yylval.i = MNEM_I8089_NOT; return TOK_I8089_MNEM; }
<I8089>orbi	{ yylval.i = MNEM_I8089_ORBI; return TOK_I8089_MNEM; }
<I8089>orb	{ yylval.i = MNEM_I8089_ORB; return TOK_I8089_MNEM; }
<I8089>ori	{ yylval.i = MNEM_I8089_ORI; return TOK_I8089_MNEM; }
<I8089>or	{ yylval.i = MNEM_I8089_OR; return TOK_I8089_MNEM; }
<I8089>setb	{ yylval.i = MNEM_I8089_SETB; return TOK_I8089_MNEM; }
<I8089>sintr	{ yylval.i = MNEM_I8089_SINTR; return TOK_I8089_MNEM; }
<I8089>tsl	{ yylval.i = MNEM_I8089_TSL; return TOK_I8089_MNEM; }
<I8089>wid	{ yylval.i = MNEM_I8089_WID; return TOK_I8089_MNEM; }
<I8089>xfer	{ yylval.i = MNEM_I8089_XFER; return TOK_I8089_MNEM; }

<I8089>"]."	{ return TOK_BRACKET_DOT; }

%%

void setup_lexer(parser_state_t * state)
{
	if(state->bit_size == BITSIZE8)
	{
		switch(state->x80_cpu_type)
		{
		case CPU_8080:
			BEGIN(I8080);
			break;
		case CPU_Z80:
			BEGIN(Z80);
			break;
		default:
			assert(false);
			break;
		}
		return;
	}

	switch(state->cpu_type)
	{
	case CPU_V30:
	case CPU_9002:
	case CPU_V33:
	case CPU_V25:
	case CPU_V55:
		BEGIN(NEC);
		break;
	case CPU_8089:
		BEGIN(I8089);
		break;
	default:
		BEGIN(INTEL);
		break;
	}
}

static int parse_cond(const char * cond)
{
	switch(cond[0])
	{
	case 'a':
		switch(cond[1])
		{
		case 'e':
			return COND_AE;
		default:
			return COND_A;
		}
	case 'b':
		switch(cond[1])
		{
		case 'e':
			return COND_BE;
		default:
			return COND_B;
		}
	case 'c':
		return COND_C;
	case 'e':
		return COND_E;
	case 'g':
		switch(cond[1])
		{
		case 'e':
			return COND_GE;
		default:
			return COND_G;
		}
	case 'l':
		switch(cond[1])
		{
		case 'e':
			return COND_LE;
		default:
			return COND_L;
		}
	case 'n':
		switch(cond[1])
		{
		case 'a':
			switch(cond[2])
			{
			case 'e':
				return COND_NAE;
			default:
				return COND_NA;
			}
		case 'b':
			switch(cond[2])
			{
			case 'e':
				return COND_NBE;
			default:
				return COND_NB;
			}
		case 'c':
			return COND_NC;
		case 'e':
			return COND_NE;
		case 'g':
			switch(cond[2])
			{
			case 'e':
				return COND_NGE;
			default:
				return COND_NG;
			}
		case 'l':
			switch(cond[2])
			{
			case 'e':
				return COND_NLE;
			default:
				return COND_NL;
			}
		case 'o':
			return COND_NO;
		case 'p':
			return COND_NP;
		case 's':
			return COND_NS;
		case 'z':
			return COND_NZ;
		default:
			return -1;
		}
	case 'o':
		return COND_O;
	case 'p':
		switch(cond[1])
		{
		case 'e':
			return COND_PE;
		case 'o':
			return COND_PO;
		default:
			return COND_P;
		}
	case 's':
		return COND_S;
	case 'z':
		return COND_Z;
	default:
		return -1;
	}
}

static int parse_cond_nec(const char * cond)
{
	switch(cond[0])
	{
	case 'c':
		return COND_C;
	case 'e':
		return COND_E;
	case 'g':
		switch(cond[1])
		{
		case 'e':
			return COND_GE;
		case 't':
			return COND_GT;
		default:
			return -1;
		}
	case 'h':
		return COND_H;
	case 'l':
		switch(cond[1])
		{
		case 'e':
			return COND_LE;
		case 't':
			return COND_LT;
		default:
			return COND_L_NEC;
		}
	case 'n':
		switch(cond[1])
		{
		case 'c':
			return COND_NC;
		case 'e':
			return COND_NE;
		case 'h':
			return COND_NH;
		case 'l':
			return COND_NL_NEC;
		case 'v':
			return COND_NV;
		case 'z':
			return COND_NZ;
		default:
			return COND_N;
		}
	case 'p':
		switch(cond[1])
		{
		case 'e':
			return COND_PE;
		case 'o':
			return COND_PO;
		default:
			return COND_P_NEC;
		}
	case 'v':
		return COND_V;
	case 'z':
		return COND_Z;
	default:
		return -1;
	}
}

static int parse_cond_x80(const char * cond)
{
	switch(cond[0])
	{
	case 'c':
		return 3;
	case 'm':
		return 7;
	case 'n':
		switch(cond[1])
		{
		case 'c':
			return 2;
		case 'z':
			return 0;
		default:
			return -1;
		}
	case 'p':
		switch(cond[1])
		{
		case 'e':
			return 5;
		case 'o':
			return 4;
		default:
			return 6;
		}
	case 'z':
		return 1;
	default:
		return -1;
	}
}

static int parse_cond_x87(const char * cond)
{
	switch(cond[0])
	{
	case 'b':
		switch(cond[1])
		{
		case 'e':
			return 2;
		default:
			return 0;
		}
	case 'e':
		return 1;
	case 'n':
		switch(cond[1])
		{
		case 'b':
			switch(cond[2])
			{
			case 'e':
				return 6;
			default:
				return 4;
			}
		case 'e':
			return 5;
		case 'u':
			return 7;
		default:
			return -1;
		}
	case 'u':
		return 3;
	default:
		return -1;
	}
}

