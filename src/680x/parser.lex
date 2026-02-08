
%include "../parser.lex"

%{
#include <assert.h>

#include "../../../src/680x/isa.h"
#include "parser.tab.h"
%}

%s M6800 M6809
%%

".word16"	{ yylval.i = _DATA_BE(BITSIZE16); return TOK_DATA; }
".word32"	{ yylval.i = _DATA_BE(BITSIZE32); return TOK_DATA; }
".word64"	{ yylval.i = _DATA_BE(BITSIZE64); return TOK_DATA; }

".byte"|"byte"|"db"	{ yylval.i = BITSIZE8; return TOK_DATA; }
".word"|"word"|"dw"	{ yylval.i = _DATA_BE(BITSIZE16); return TOK_DATA; }

".lo"w?	{ return KWD_LOW; }
".hi"(gh)?	{ return KWD_HIGH; }
".addr"	{ return KWD_ADDR; }
".page" { return KWD_PAGE; }

".6800"	{ yylval.i = CPU_6800; return TOK_ARCH; }
".6809"	{ yylval.i = CPU_6809; return TOK_ARCH; }

<M6809>"a"	{ yylval.i = REG_A; return TOK_AREG; }
<M6809>"b"	{ yylval.i = REG_B; return TOK_BREG; }
<M6809>"d"	{ yylval.i = REG_D; return TOK_DREG; }
<M6809>"u"	{ yylval.i = REG_U; return TOK_IXREG; }
<M6809>"s"	{ yylval.i = REG_S; return TOK_IXREG; }
"x"	{ yylval.i = REG_X; return TOK_IXREG; }
<M6809>"y"	{ yylval.i = REG_Y; return TOK_IXREG; }
<M6809>"dp"	{ yylval.i = REG_DP; return TOK_REG; }
<M6809>"cc"	{ yylval.i = REG_CC; return TOK_REG; }
<M6809>"pcr"	{ yylval.i = REG_PC; return TOK_PCREL; }
<M6809>"pc"	{ yylval.i = REG_PC; return TOK_PCREG; }

%include "../../obj/680x/680x/mnem.lex"

%%

void setup_lexer(parser_state_t * state)
{
	switch(state->cpu_type)
	{
	case CPU_6800:
		BEGIN(M6800);
		break;
	case CPU_6809:
		BEGIN(M6809);
		break;
	default:
		assert(false);
		break;
	}
}

