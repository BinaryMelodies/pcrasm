
%include "../parser.lex"

%{
#include <assert.h>

#include "../../../src/asm.h"
#include "../../../src/i4/isa.h"
#include "parser.tab.h"
%}

%%

".4004"	{ yylval.i = CPU_4004; return TOK_ARCH; }
".4040"	{ yylval.i = CPU_4040; return TOK_ARCH; }

"add"	{ yylval.i = MNEM_ADD; return TOK_MNEM; }
"adm"	{ yylval.i = MNEM_ADM; return TOK_MNEM; }
"bbl"	{ yylval.i = MNEM_BBL; return TOK_MNEM; }
"clb"	{ yylval.i = MNEM_CLB; return TOK_MNEM; }
"clc"	{ yylval.i = MNEM_CLC; return TOK_MNEM; }
"cma"	{ yylval.i = MNEM_CMA; return TOK_MNEM; }
"cmc"	{ yylval.i = MNEM_CMC; return TOK_MNEM; }
"daa"	{ yylval.i = MNEM_DAA; return TOK_MNEM; }
"dac"	{ yylval.i = MNEM_DAC; return TOK_MNEM; }
"dcl"	{ yylval.i = MNEM_DCL; return TOK_MNEM; }
"fim"	{ yylval.i = MNEM_FIM; return TOK_MNEM; }
"fin"	{ yylval.i = MNEM_FIN; return TOK_MNEM; }
"iac"	{ yylval.i = MNEM_IAC; return TOK_MNEM; }
"inc"	{ yylval.i = MNEM_INC; return TOK_MNEM; }
"isz"	{ yylval.i = MNEM_ISZ; return TOK_MNEM; }
"jcn"	{ yylval.i = MNEM_JCN; return TOK_MNEM; }
"jin"	{ yylval.i = MNEM_JIN; return TOK_MNEM; }
"jms"	{ yylval.i = MNEM_JMS; return TOK_MNEM; }
"jun"	{ yylval.i = MNEM_JUN; return TOK_MNEM; }
"kbp"	{ yylval.i = MNEM_KBP; return TOK_MNEM; }
"ld"	{ yylval.i = MNEM_LD; return TOK_MNEM; }
"ldm"	{ yylval.i = MNEM_LDM; return TOK_MNEM; }
"nop"	{ yylval.i = MNEM_NOP; return TOK_MNEM; }
"ral"	{ yylval.i = MNEM_RAL; return TOK_MNEM; }
"rar"	{ yylval.i = MNEM_RAR; return TOK_MNEM; }
"rd0"	{ yylval.i = MNEM_RD0; return TOK_MNEM; }
"rd1"	{ yylval.i = MNEM_RD1; return TOK_MNEM; }
"rd2"	{ yylval.i = MNEM_RD2; return TOK_MNEM; }
"rd3"	{ yylval.i = MNEM_RD3; return TOK_MNEM; }
"rdm"	{ yylval.i = MNEM_RDM; return TOK_MNEM; }
"rdr"	{ yylval.i = MNEM_RDR; return TOK_MNEM; }
"sbm"	{ yylval.i = MNEM_SBM; return TOK_MNEM; }
"src"	{ yylval.i = MNEM_SRC; return TOK_MNEM; }
"stc"	{ yylval.i = MNEM_STC; return TOK_MNEM; }
"sub"	{ yylval.i = MNEM_SUB; return TOK_MNEM; }
"tcc"	{ yylval.i = MNEM_TCC; return TOK_MNEM; }
"tcs"	{ yylval.i = MNEM_TCS; return TOK_MNEM; }
"wmp"	{ yylval.i = MNEM_WMP; return TOK_MNEM; }
"wpm"	{ yylval.i = MNEM_WPM; return TOK_MNEM; }
"wr0"	{ yylval.i = MNEM_WR0; return TOK_MNEM; }
"wr1"	{ yylval.i = MNEM_WR1; return TOK_MNEM; }
"wr2"	{ yylval.i = MNEM_WR2; return TOK_MNEM; }
"wr3"	{ yylval.i = MNEM_WR3; return TOK_MNEM; }
"wrm"	{ yylval.i = MNEM_WRM; return TOK_MNEM; }
"wrr"	{ yylval.i = MNEM_WRR; return TOK_MNEM; }
"xch"	{ yylval.i = MNEM_XCH; return TOK_MNEM; }

"an6"	{ yylval.i = MNEM_AN6; return TOK_MNEM; }
"an7"	{ yylval.i = MNEM_AN7; return TOK_MNEM; }
"bbs"	{ yylval.i = MNEM_BBS; return TOK_MNEM; }
"db0"	{ yylval.i = MNEM_DB0; return TOK_MNEM; }
"db1"	{ yylval.i = MNEM_DB1; return TOK_MNEM; }
"din"	{ yylval.i = MNEM_DIN; return TOK_MNEM; }
"ein"	{ yylval.i = MNEM_EIN; return TOK_MNEM; }
"hlt"	{ yylval.i = MNEM_HLT; return TOK_MNEM; }
"lcr"	{ yylval.i = MNEM_LCR; return TOK_MNEM; }
"or4"	{ yylval.i = MNEM_OR4; return TOK_MNEM; }
"or5"	{ yylval.i = MNEM_OR5; return TOK_MNEM; }
"rpm"	{ yylval.i = MNEM_RPM; return TOK_MNEM; }
"sb0"	{ yylval.i = MNEM_SB0; return TOK_MNEM; }
"sb1"	{ yylval.i = MNEM_SB1; return TOK_MNEM; }

0|[1-9][0-9]*[Pp]	{
#if USE_GMP
		yytext[strlen(yytext) - 1] = '\0';
#endif
		uint_parse(yylval.j, yytext, 10);
		int_shl(INTVAL(yylval.j), 1);
		return TOK_INTEGER;
	}

%%

void setup_lexer(parser_state_t * state)
{
}

