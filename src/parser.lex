
%{

#define YY_DECL int yylex_direct(void)

%}

%option noyywrap

%%

[ \t]	{ }
;[^\n]*	{
		// suppress warnings
		(void) input;
		(void) yyunput;
	}

"."align	{ return KWD_ALIGN; }
"."comm(on)?	{ return KWD_COMMON; }
"."defined	{ return KWD_DEFINED; }
"."el(se)?if	{ return KWD_ELSE_IF; }
"."else	{ return KWD_ELSE; }
"."endfill	{ return KWD_ENDFILL; }
"."endif	{ return KWD_ENDIF; }
"."endmacro	{ return KWD_ENDMACRO; }
"."endrepeat	{ return KWD_ENDREPEAT; }
"."endtimes	{ return KWD_ENDTIMES; }
"."entry	{ return KWD_ENTRY; }
"."equ	{ return KWD_EQU; }
"."export	{ return KWD_EXPORT; }
"."extern	{ return KWD_EXTERN; }
"."fill	{ return KWD_FILL; }
"."global	{ return KWD_GLOBAL; }
"."if	{ return KWD_IF; }
"."import	{ return KWD_IMPORT; }
"."macro	{ return KWD_MACRO; }
"."org	{ return KWD_ORG; }
"."repeat	{ return KWD_REPEAT; }
"."section	{ return KWD_SECTION; }
"."skip	{ return KWD_SKIP; }
"."times	{ return KWD_TIMES; }

".word8"	{ yylval.i = BITSIZE8; return TOK_DATA; }
".word16le"	{ yylval.i = _DATA_LE(BITSIZE16); return TOK_DATA; }
".word16be"	{ yylval.i = _DATA_BE(BITSIZE16); return TOK_DATA; }
".word32le"	{ yylval.i = _DATA_LE(BITSIZE32); return TOK_DATA; }
".word32be"	{ yylval.i = _DATA_BE(BITSIZE32); return TOK_DATA; }
".word32pe"	{ yylval.i = _DATA_PE(BITSIZE32); return TOK_DATA; }
".word64le"	{ yylval.i = _DATA_LE(BITSIZE64); return TOK_DATA; }
".word64be"	{ yylval.i = _DATA_BE(BITSIZE64); return TOK_DATA; }
".word64pe"	{ yylval.i = _DATA_PE(BITSIZE64); return TOK_DATA; }

0|[1-9][0-9]*	{
		uint_parse(yylval.j, yytext, 10);
		return TOK_INTEGER;
	}
"%"[01]+	{
		uint_parse(yylval.j, yytext + 1, 2);
		return TOK_INTEGER;
	}
0[Bb][01]+	{
		uint_parse(yylval.j, yytext + 2, 2);
		return TOK_INTEGER;
	}
[01]+B	{
#if USE_GMP
		yytext[strlen(yytext) - 1] = '\0';
#endif
		uint_parse(yylval.j, yytext, 2);
		return TOK_INTEGER;
	}
"@"[0-7]+	{
		uint_parse(yylval.j, yytext + 1, 8);
		return TOK_INTEGER;
	}
0[0-7]+	{
		uint_parse(yylval.j, yytext + 1, 8);
		return TOK_INTEGER;
	}
0[Oo][0-7]+	{
		uint_parse(yylval.j, yytext + 2, 8);
		return TOK_INTEGER;
	}
[0-7]+[OoQq]	{
#if USE_GMP
		yytext[strlen(yytext) - 1] = '\0';
#endif
		uint_parse(yylval.j, yytext, 8);
		return TOK_INTEGER;
	}
"$"[A-Fa-f0-9]+	{
		uint_parse(yylval.j, yytext + 1, 16);
		return TOK_INTEGER;
	}
0[Xx][A-Fa-f0-9]+	{
		uint_parse(yylval.j, yytext + 2, 16);
		return TOK_INTEGER;
	}
[0-9][A-Fa-f0-9]*[Hh]	{
#if USE_GMP
		yytext[strlen(yytext) - 1] = '\0';
#endif
		uint_parse(yylval.j, yytext, 16);
		return TOK_INTEGER;
	}
[Xx]'[A-Fa-f0-9]+'	{
#if USE_GMP
		yytext[strlen(yytext) - 1] = '\0';
#endif
		uint_parse(yylval.j, yytext + 2, 16);
		return TOK_INTEGER;
	}

0|[1-9][0-9]*f	{
#if USE_GMP
		yytext[strlen(yytext) - 1] = '\0';
#endif
		uint_parse(yylval.j, yytext, 10);
		return TOK_LABEL_FORWARD;
	}

0|[1-9][0-9]*b	{
#if USE_GMP
		yytext[strlen(yytext) - 1] = '\0';
#endif
		uint_parse(yylval.j, yytext, 10);
		return TOK_LABEL_BACKWARD;
	}

"&&"	{ return DEL_AND; }
"<=>"	{ return DEL_CMP; }
"=="	{ return DEL_EQ; }
">="	{ return DEL_GE; }
"=>"	{ return DEL_GE1; }
"<="	{ return DEL_LE; }
"=<"	{ return DEL_LE1; }
"!="	{ return DEL_NE; }
"<>"	{ return DEL_NE1; }
"><"	{ return DEL_NE2; }
"||"	{ return DEL_OR; }
"$$"	{ return DEL_SECT; }
"<<"	{ return DEL_SHL; }
">>"	{ return DEL_SHR; }
"^^"	{ return DEL_XOR; }

"<<<"	{ return DEL_SHL_S; }
">>>"	{ return DEL_SHR_S; }
"//"	{ return DEL_DIV_S; }
"%%"	{ return DEL_MOD_S; }
"%<=>"	{ return DEL_CMP_U; }
"%>="	{ return DEL_GE_U; }
"%=>"	{ return DEL_GE1_U; }
"%<="	{ return DEL_LE_U; }
"%=<"	{ return DEL_LE1_U; }
"%<"	{ return DEL_LT_U; }
"%>"	{ return DEL_GT_U; }

[.$]	{ return yytext[0]; }

[.A-Za-z_][.A-Za-z_0-9$]*	{ yylval.s = strdup(yytext); return TOK_IDENTIFIER; }
$[.A-Za-z_0-9$]+	{ yylval.s = strdup(yytext + 1); return TOK_IDENTIFIER; }

'[^']*'	{ yytext[strlen(yytext) - 1] = '\0'; yylval.s = strdup(yytext + 1); return TOK_STRING; }
\"[^"]*\"	{ yytext[strlen(yytext) - 1] = '\0'; yylval.s = strdup(yytext + 1); return TOK_STRING; }

.|\n	{ return yytext[0]; }

%%

