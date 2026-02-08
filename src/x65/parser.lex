
%include "../parser.lex"

%{
#include <assert.h>

#include "../../../src/asm.h"
#include "../../../src/x65/isa.h"
#include "parser.tab.h"
%}

%s WDC65C02_OLD WDC65C02_NEW WDC65CE02 WDC65C816
%%

".word24le"	{ yylval.i = _DATA_LE(BITSIZE24); return TOK_DATA; }
".word24be"	{ yylval.i = _DATA_BE(BITSIZE24); return TOK_DATA; }

".word16"	{ yylval.i = _DATA_LE(BITSIZE16); return TOK_DATA; }
".word24"	{ yylval.i = _DATA_LE(BITSIZE24); return TOK_DATA; }
".word32"	{ yylval.i = _DATA_LE(BITSIZE32); return TOK_DATA; }
".word64"	{ yylval.i = _DATA_LE(BITSIZE64); return TOK_DATA; }

".byte"|"byte"|"db"	{ yylval.i = BITSIZE8; return TOK_DATA; }
".word"|"word"|"dw"	{ yylval.i = _DATA_LE(BITSIZE16); return TOK_DATA; }
".dword"|"dword"|"dd"	{ yylval.i = _DATA_LE(BITSIZE32); return TOK_DATA; }
".long"|"long"	{ yylval.i = _DATA_LE(BITSIZE24); return TOK_DATA; }

".lo"w?	{ return KWD_LOW; }
".hi"(gh)?	{ return KWD_HIGH; }
".addr"	{ return KWD_ADDR; }
".bank" { return KWD_BANK; }

".6502"	{ yylval.i = CPU_6502; return TOK_ARCH; }
".65"[Cc]"02"	{ yylval.i = CPU_65C02; return TOK_ARCH; }
"."[Ww][Dd][Zz]"65"[Cc]"02"|".rockwell"	{ yylval.i = CPU_WDC65C02; return TOK_ARCH; }
".65"[Cc]?"816"	{ yylval.i = CPU_65C816; return TOK_ARCH; }
".65"[Cc][Ee]"02"	{ yylval.i = CPU_65CE02; return TOK_ARCH; }

".width"	{ return KWD_WIDTH; }

"a"	{ return TOK_AREG; }
<WDC65C816>"s"	{ return TOK_SREG; }
<WDC65CE02>"sp"	{ return TOK_SREG; }
"x"	{ return TOK_XREG; }
"y"	{ return TOK_YREG; }
<WDC65CE02>"z"	{ return TOK_ZREG; }

"adc"	{ yylval.i = MNEM_ADC; return TOK_MNEM; }
"and"	{ yylval.i = MNEM_AND; return TOK_MNEM; }
"asl"	{ yylval.i = MNEM_ASL; return TOK_MNEM; }
<WDC65CE02>"asr"	{ yylval.i = MNEM_ASR; return TOK_MNEM; }
<WDC65CE02>"asw"	{ yylval.i = MNEM_ASW; return TOK_MNEM; }
<WDC65CE02>"aug"|"map"	{ yylval.i = MNEM_AUG; return TOK_MNEM; }
<WDC65C02_NEW,WDC65CE02,WDC65C816>"bbr"[0-7]	{ yylval.i = ((yytext[3] - '0') << 12) | MNEM_BBR; return TOK_MNEM; }
<WDC65C02_NEW,WDC65CE02,WDC65C816>"bbs"[0-7]	{ yylval.i = ((yytext[3] - '0') << 12) | MNEM_BBS; return TOK_MNEM; }
"bcc"	{ yylval.i = MNEM_BCC; return TOK_MNEM; }
"bcs"	{ yylval.i = MNEM_BCS; return TOK_MNEM; }
"beq"	{ yylval.i = MNEM_BEQ; return TOK_MNEM; }
"bit"	{ yylval.i = MNEM_BIT; return TOK_MNEM; }
"bmi"	{ yylval.i = MNEM_BMI; return TOK_MNEM; }
"bne"	{ yylval.i = MNEM_BNE; return TOK_MNEM; }
"bpl"	{ yylval.i = MNEM_BPL; return TOK_MNEM; }
<WDC65C02_OLD,WDC65C02_NEW,WDC65CE02,WDC65C816>"bra"	{ yylval.i = MNEM_BRA; return TOK_MNEM; }
"brk"	{ yylval.i = MNEM_BRK; return TOK_MNEM; }
<WDC65C816>"brl"	{ yylval.i = MNEM_BRL; return TOK_MNEM; }
<WDC65CE02>"bsr"	{ yylval.i = MNEM_BSR; return TOK_MNEM; }
"bvc"	{ yylval.i = MNEM_BVC; return TOK_MNEM; }
"bvs"	{ yylval.i = MNEM_BVS; return TOK_MNEM; }
"clc"	{ yylval.i = MNEM_CLC; return TOK_MNEM; }
"cld"	{ yylval.i = MNEM_CLD; return TOK_MNEM; }
<WDC65CE02>"cle"	{ yylval.i = MNEM_CLE; return TOK_MNEM; }
"cli"	{ yylval.i = MNEM_CLI; return TOK_MNEM; }
"clv"	{ yylval.i = MNEM_CLV; return TOK_MNEM; }
"cmp"	{ yylval.i = MNEM_CMP; return TOK_MNEM; }
<WDC65C816>"cop"	{ yylval.i = MNEM_COP; return TOK_MNEM; }
"cpx"	{ yylval.i = MNEM_CPX; return TOK_MNEM; }
"cpy"	{ yylval.i = MNEM_CPY; return TOK_MNEM; }
<WDC65CE02>"cpz"	{ yylval.i = MNEM_CPZ; return TOK_MNEM; }
"dec"	{ yylval.i = MNEM_DEC; return TOK_MNEM; }
<WDC65CE02>"dew"	{ yylval.i = MNEM_DEW; return TOK_MNEM; }
"dex"	{ yylval.i = MNEM_DEX; return TOK_MNEM; }
"dey"	{ yylval.i = MNEM_DEY; return TOK_MNEM; }
<WDC65CE02>"dez"	{ yylval.i = MNEM_DEZ; return TOK_MNEM; }
"eor"	{ yylval.i = MNEM_EOR; return TOK_MNEM; }
"inc"	{ yylval.i = MNEM_INC; return TOK_MNEM; }
<WDC65CE02>"inw"	{ yylval.i = MNEM_INW; return TOK_MNEM; }
"inx"	{ yylval.i = MNEM_INX; return TOK_MNEM; }
"iny"	{ yylval.i = MNEM_INY; return TOK_MNEM; }
<WDC65CE02>"inz"	{ yylval.i = MNEM_INZ; return TOK_MNEM; }
<WDC65C816>"jml"	{ yylval.i = MNEM_JML; return TOK_MNEM; }
"jmp"	{ yylval.i = MNEM_JMP; return TOK_MNEM; }
<WDC65C816>"jsl"	{ yylval.i = MNEM_JSL; return TOK_MNEM; }
"jsr"	{ yylval.i = MNEM_JSR; return TOK_MNEM; }
"lda"	{ yylval.i = MNEM_LDA; return TOK_MNEM; }
"ldx"	{ yylval.i = MNEM_LDX; return TOK_MNEM; }
"ldy"	{ yylval.i = MNEM_LDY; return TOK_MNEM; }
<WDC65CE02>"ldz"	{ yylval.i = MNEM_LDZ; return TOK_MNEM; }
"lsr"	{ yylval.i = MNEM_LSR; return TOK_MNEM; }
<WDC65C816>"mvn"	{ yylval.i = MNEM_MVN; return TOK_MNEM; }
<WDC65C816>"mvp"	{ yylval.i = MNEM_MVP; return TOK_MNEM; }
<WDC65CE02>"neg"	{ yylval.i = MNEM_NEG; return TOK_MNEM; }
"nop"	{ yylval.i = MNEM_NOP; return TOK_MNEM; }
"ora"	{ yylval.i = MNEM_ORA; return TOK_MNEM; }
<WDC65C816>"pea"	{ yylval.i = MNEM_PEA; return TOK_MNEM; }
<WDC65C816>"pei"	{ yylval.i = MNEM_PEI; return TOK_MNEM; }
<WDC65C816>"per"	{ yylval.i = MNEM_PER; return TOK_MNEM; }
"pha"	{ yylval.i = MNEM_PHA; return TOK_MNEM; }
<WDC65C816>"phb"	{ yylval.i = MNEM_PHB; return TOK_MNEM; }
<WDC65C816>"phd"	{ yylval.i = MNEM_PHD; return TOK_MNEM; }
<WDC65C816>"phk"	{ yylval.i = MNEM_PHK; return TOK_MNEM; }
"php"	{ yylval.i = MNEM_PHP; return TOK_MNEM; }
<WDC65CE02>"phw"	{ yylval.i = MNEM_PHW; return TOK_MNEM; }
<WDC65C02_OLD,WDC65C02_NEW,WDC65CE02,WDC65C816>"phx"	{ yylval.i = MNEM_PHX; return TOK_MNEM; }
<WDC65C02_OLD,WDC65C02_NEW,WDC65CE02,WDC65C816>"phy"	{ yylval.i = MNEM_PHY; return TOK_MNEM; }
<WDC65CE02>"phz"	{ yylval.i = MNEM_PHZ; return TOK_MNEM; }
"pla"	{ yylval.i = MNEM_PLA; return TOK_MNEM; }
<WDC65C816>"plb"	{ yylval.i = MNEM_PLB; return TOK_MNEM; }
<WDC65C816>"pld"	{ yylval.i = MNEM_PLD; return TOK_MNEM; }
"plp"	{ yylval.i = MNEM_PLP; return TOK_MNEM; }
<WDC65C02_OLD,WDC65C02_NEW,WDC65CE02,WDC65C816>"plx"	{ yylval.i = MNEM_PLX; return TOK_MNEM; }
<WDC65C02_OLD,WDC65C02_NEW,WDC65CE02,WDC65C816>"ply"	{ yylval.i = MNEM_PLY; return TOK_MNEM; }
<WDC65CE02>"plz"	{ yylval.i = MNEM_PLZ; return TOK_MNEM; }
<WDC65C816>"rep"	{ yylval.i = MNEM_REP; return TOK_MNEM; }
<WDC65C02_NEW,WDC65CE02,WDC65C816>"rmb"[0-7]	{ yylval.i = ((yytext[3] - '0') << 12) | MNEM_RMB; return TOK_MNEM; }
"rol"	{ yylval.i = MNEM_ROL; return TOK_MNEM; }
"ror"	{ yylval.i = MNEM_ROR; return TOK_MNEM; }
<WDC65CE02>"row"	{ yylval.i = MNEM_ROW; return TOK_MNEM; }
"rti"	{ yylval.i = MNEM_RTI; return TOK_MNEM; }
<WDC65C816>"rtl"	{ yylval.i = MNEM_RTL; return TOK_MNEM; }
"rts"	{ yylval.i = MNEM_RTS; return TOK_MNEM; }
"sbc"	{ yylval.i = MNEM_SBC; return TOK_MNEM; }
"sec"	{ yylval.i = MNEM_SEC; return TOK_MNEM; }
"sed"	{ yylval.i = MNEM_SED; return TOK_MNEM; }
<WDC65CE02>"see"	{ yylval.i = MNEM_SEE; return TOK_MNEM; }
"sei"	{ yylval.i = MNEM_SEI; return TOK_MNEM; }
<WDC65C816>"sep"	{ yylval.i = MNEM_SEP; return TOK_MNEM; }
<WDC65C02_NEW,WDC65CE02,WDC65C816>"smb"[0-7]	{ yylval.i = ((yytext[3] - '0') << 12) | MNEM_SMB; return TOK_MNEM; }
"sta"	{ yylval.i = MNEM_STA; return TOK_MNEM; }
"stp"	{ yylval.i = MNEM_STP; return TOK_MNEM; }
"stx"	{ yylval.i = MNEM_STX; return TOK_MNEM; }
"sty"	{ yylval.i = MNEM_STY; return TOK_MNEM; }
<WDC65C02_OLD,WDC65C02_NEW,WDC65CE02,WDC65C816>"stz"	{ yylval.i = MNEM_STZ; return TOK_MNEM; }
<WDC65CE02>"tab"	{ yylval.i = MNEM_TAB; return TOK_MNEM; }
"tax"	{ yylval.i = MNEM_TAX; return TOK_MNEM; }
"tay"	{ yylval.i = MNEM_TAY; return TOK_MNEM; }
<WDC65CE02>"taz"	{ yylval.i = MNEM_TAZ; return TOK_MNEM; }
<WDC65CE02>"tba"	{ yylval.i = MNEM_TBA; return TOK_MNEM; }
<WDC65C816>"tcd"	{ yylval.i = MNEM_TCD; return TOK_MNEM; }
<WDC65C816>"tcs"	{ yylval.i = MNEM_TCS; return TOK_MNEM; }
<WDC65C816>"tdc"	{ yylval.i = MNEM_TDC; return TOK_MNEM; }
<WDC65C02_OLD,WDC65C02_NEW,WDC65CE02,WDC65C816>"trb"	{ yylval.i = MNEM_TRB; return TOK_MNEM; }
<WDC65C02_OLD,WDC65C02_NEW,WDC65CE02,WDC65C816>"tsb"	{ yylval.i = MNEM_TSB; return TOK_MNEM; }
<WDC65C816>"tsc"	{ yylval.i = MNEM_TSC; return TOK_MNEM; }
"tsx"	{ yylval.i = MNEM_TSX; return TOK_MNEM; }
<WDC65CE02>"tsy"	{ yylval.i = MNEM_TSY; return TOK_MNEM; }
"txa"	{ yylval.i = MNEM_TXA; return TOK_MNEM; }
"txs"	{ yylval.i = MNEM_TXS; return TOK_MNEM; }
<WDC65C816>"txy"	{ yylval.i = MNEM_TXY; return TOK_MNEM; }
"tya"	{ yylval.i = MNEM_TYA; return TOK_MNEM; }
<WDC65CE02>"tys"	{ yylval.i = MNEM_TYS; return TOK_MNEM; }
<WDC65C816>"tyx"	{ yylval.i = MNEM_TYX; return TOK_MNEM; }
<WDC65CE02>"tza"	{ yylval.i = MNEM_TZA; return TOK_MNEM; }
<WDC65C816>"wai"	{ yylval.i = MNEM_WAI; return TOK_MNEM; }
<WDC65C816>"wdm"	{ yylval.i = MNEM_WDM; return TOK_MNEM; }
<WDC65C816>"xba"	{ yylval.i = MNEM_XBA; return TOK_MNEM; }
<WDC65C816>"xce"	{ yylval.i = MNEM_XCE; return TOK_MNEM; }

"!"	{ return KWD_ADDR; /* to avoid clashing with the negation operator */ }

%%

void setup_lexer(parser_state_t * state)
{
	switch(state->cpu_type)
	{
	case CPU_6502:
		BEGIN(INITIAL);
		break;
	case CPU_65C02:
		BEGIN(WDC65C02_OLD);
		break;
	case CPU_WDC65C02:
		BEGIN(WDC65C02_NEW);
		break;
	case CPU_65CE02:
		BEGIN(WDC65CE02);
		break;
	case CPU_65C816:
		BEGIN(WDC65C816);
		break;
	default:
		assert(false);
	}
}

