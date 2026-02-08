
%include "../parser.lex"

%{
#include <assert.h>

#include "../../../src/asm.h"
#if TARGET_X86
# include "../../../src/x86/isa.h"
#else
# include "../../../src/x80/isa.h"
#endif
#include "parser.tab.h"

static int parse_cond_x80(const char * cond);

static const int _i8008reg['m' - 'a' + 1];
static const int _i8008cond['z' - 'c' + 1];
#define i8008reg(r) (_i8008reg[(r) - 'a'])
#define i8008cond(r) (condition_i8_to_i80(_i8008cond[*(&(r) + 1) - 'c'] + ((r) == 't' ? 4 : 0)))
%}

X80_COND	(nz|z|nc|c|po|pe|p|m)

%s DP2200 I8008 I8080 Z80 R800
%%

".word16"	{ yylval.i = _DATA_LE(BITSIZE16); return TOK_DATA; }
".word32"	{ yylval.i = _DATA_LE(BITSIZE32); return TOK_DATA; }
".word64"	{ yylval.i = _DATA_LE(BITSIZE64); return TOK_DATA; }

".byte"|"byte"|"db"	{ yylval.i = BITSIZE8; return TOK_DATA; }
".word"|"word"|"dw"	{ yylval.i = _DATA_LE(BITSIZE16); return TOK_DATA; }
".dword"|"dword"|"dd"	{ yylval.i = _DATA_LE(BITSIZE32); return TOK_DATA; }
".long"|"long"	{ yylval.i = _DATA_LE(BITSIZE32); return TOK_DATA; }
".qword"|"qword"|"dq"	{ yylval.i = _DATA_LE(BITSIZE64); return TOK_DATA; }
".quad"|"quad"	{ yylval.i = _DATA_LE(BITSIZE64); return TOK_DATA; }

".dp2200"	{ yylval.i = CPU_DP2200; return TOK_ARCH; }
".dp2200v2"	{ yylval.i = CPU_DP2200V2; return TOK_ARCH; }
".8008"	{ yylval.i = CPU_8008; return TOK_ARCH; }
".8080"	{ yylval.i = CPU_8080; return TOK_ARCH; }
".8085"	{ yylval.i = CPU_8085; return TOK_ARCH; }
".z80"	{ yylval.i = CPU_Z80; return TOK_ARCH; }
".r800"	{ yylval.i = CPU_R800; return TOK_ARCH; }
".gbz80"|".lr35902"|".sm83"	{ yylval.i = CPU_GBZ80; return TOK_ARCH; }

".dp2200syntax"	{ yylval.i = SYNTAX_DP2200; return TOK_SYNTAX; }
".8008syntax"	{ yylval.i = SYNTAX_I8008; return TOK_SYNTAX; }
".intel"	{ yylval.i = SYNTAX_INTEL; return TOK_SYNTAX; }
".zilog"	{ yylval.i = SYNTAX_ZILOG; return TOK_SYNTAX; }
".ascii"	{ yylval.i = SYNTAX_ASCII; return TOK_SYNTAX; }

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
<Z80>hl	{ yylval.i = X80_HL; return TOK_X80_HL; }
<Z80>hld	{ yylval.i = X80_HLD; return TOK_X80_HLD; }
<Z80>hli	{ yylval.i = X80_HLI; return TOK_X80_HLI; }
<Z80>ix	{ yylval.i = X80_IX; return TOK_X80_IDX; }
<Z80>iy	{ yylval.i = X80_IY; return TOK_X80_IDX; }
<I8080,Z80>sp	{ yylval.i = X80_SP; return TOK_X80_MEM; }
<I8080>psw	{ yylval.i = X80_PSW; return TOK_X80_REG; }
<Z80>af	{ yylval.i = X80_AF; return TOK_X80_REG; }
<Z80>"af'"	{ yylval.i = X80_AF2; return TOK_X80_REG; }

<R800>".a"	{ yylval.i = X80_A; return TOK_X80_REG; }
<R800>".b"	{ yylval.i = X80_B; return TOK_X80_MEM; }
<R800>".c"	{ yylval.i = X80_C; return TOK_X80_MEM; }
<R800>".d"	{ yylval.i = X80_D; return TOK_X80_REG; }
<R800>".e"	{ yylval.i = X80_E; return TOK_X80_REG; }
<R800>".f"	{ yylval.i = X80_F; return TOK_X80_REG; }
<R800>".h"	{ yylval.i = X80_H; return TOK_X80_REG; }
<R800>".l"	{ yylval.i = X80_L; return TOK_X80_REG; }
<R800>".ixh"	{ yylval.i = X80_IXH; return TOK_X80_REG; }
<R800>".ixl"	{ yylval.i = X80_IXL; return TOK_X80_REG; }
<R800>".iyh"	{ yylval.i = X80_IYH; return TOK_X80_REG; }
<R800>".iyl"	{ yylval.i = X80_IYL; return TOK_X80_REG; }
<R800>".i"	{ yylval.i = X80_I; return TOK_X80_REG; }
<R800>".r"	{ yylval.i = X80_R; return TOK_X80_REG; }

<R800>".bc"	{ yylval.i = X80_BC; return TOK_X80_MEM; }
<R800>".de"	{ yylval.i = X80_DE; return TOK_X80_MEM; }
<R800>".hl"	{ yylval.i = X80_HL; return TOK_X80_MEM; }
<R800>".ix"	{ yylval.i = X80_IX; return TOK_X80_IDX; }
<R800>".iy"	{ yylval.i = X80_IY; return TOK_X80_IDX; }
<R800>".sp"	{ yylval.i = X80_SP; return TOK_X80_MEM; }
<R800>".af"	{ yylval.i = X80_AF; return TOK_X80_REG; }
<R800>".af'"	{ yylval.i = X80_AF2; return TOK_X80_REG; }

<Z80,R800>nz	{ yylval.i = X80_NZ; return TOK_X80_REG; }
<Z80,R800>z	{ yylval.i = X80_Z; return TOK_X80_REG; }
<Z80,R800>nc	{ yylval.i = X80_NC; return TOK_X80_REG; }
<Z80,R800>po	{ yylval.i = X80_PO; return TOK_X80_REG; }
<Z80,R800>pe	{ yylval.i = X80_PE; return TOK_X80_REG; }
<Z80,R800>p	{ yylval.i = X80_P; return TOK_X80_REG; }
<Z80,R800>m	{ yylval.i = X80_CND_M; return TOK_X80_REG; }
<R800>c	{ yylval.i = X80_C; return TOK_X80_REG; }

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

<I8080>arhl	{ yylval.i = MNEM_I8085_ARHL; return TOK_I8080_MNEM; }
<I8080>dsub	{ yylval.i = MNEM_I8085_DSUB; return TOK_I8080_MNEM; }
<I8080>jn(k|x5|ui)	{ yylval.i = MNEM_I8085_JNUI; return TOK_I8080_MNEM; }
<I8080>j(k|x5|ui)	{ yylval.i = MNEM_I8085_JUI; return TOK_I8080_MNEM; }
<I8080>ldhi	{ yylval.i = MNEM_I8085_LDHI; return TOK_I8080_MNEM; }
<I8080>ldsi	{ yylval.i = MNEM_I8085_LDSI; return TOK_I8080_MNEM; }
<I8080>lhlx	{ yylval.i = MNEM_I8085_LHLX; return TOK_I8080_MNEM; }
<I8080>rdel	{ yylval.i = MNEM_I8085_RDEL; return TOK_I8080_MNEM; }
<I8080>rim	{ yylval.i = MNEM_I8085_RIM; return TOK_I8080_MNEM; }
<I8080>rstv|ovrst8	{ yylval.i = MNEM_I8085_RSTV; return TOK_I8080_MNEM; }
<I8080>shlx	{ yylval.i = MNEM_I8085_SHLX; return TOK_I8080_MNEM; }
<I8080>sim	{ yylval.i = MNEM_I8085_SIM; return TOK_I8080_MNEM; }

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
<Z80>ldhl	{ yylval.i = MNEM_GBZ80_LDHL; return TOK_Z80_MNEM; }
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
<Z80>stop	{ yylval.i = MNEM_GBZ80_STOP; return TOK_Z80_MNEM; }
<Z80>sub	{ yylval.i = MNEM_Z80_SUB; return TOK_Z80_MNEM; }
<Z80>swap	{ yylval.i = MNEM_GBZ80_SWAP; return TOK_Z80_MNEM; }
<Z80>xor	{ yylval.i = MNEM_Z80_XOR; return TOK_Z80_MNEM; }

<R800>addc	{ yylval.i = MNEM_Z80_ADC; return TOK_R800_MNEM; }
<R800>add	{ yylval.i = MNEM_Z80_ADD; return TOK_R800_MNEM; }
<R800>and	{ yylval.i = MNEM_R800_AND; return TOK_R800_MNEM; }
<R800>bit	{ yylval.i = MNEM_Z80_BIT; return TOK_R800_MNEM; }
<R800>call	{ yylval.i = MNEM_Z80_CALL; return TOK_R800_MNEM; }
<R800>notc	{ yylval.i = MNEM_Z80_CCF; return TOK_R800_MNEM; }
<R800>cmp	{ yylval.i = MNEM_R800_CMP; return TOK_R800_MNEM; }
<R800>cmpm	{ yylval.i = MNEM_R800_CMPM; return TOK_R800_MNEM; }
<R800>not	{ yylval.i = MNEM_R800_NOT; return TOK_R800_MNEM; }
<R800>adj	{ yylval.i = MNEM_R800_ADJ; return TOK_R800_MNEM; }
<R800>dec	{ yylval.i = MNEM_Z80_DEC; return TOK_R800_MNEM; }
<R800>di	{ yylval.i = MNEM_Z80_DI; return TOK_R800_MNEM; }
<R800>dbnz	{ yylval.i = MNEM_Z80_DJNZ; return TOK_R800_MNEM; }
<R800>ei	{ yylval.i = MNEM_Z80_EI; return TOK_R800_MNEM; }
<R800>xch	{ yylval.i = MNEM_Z80_EX; return TOK_R800_MNEM; }
<R800>xchx	{ yylval.i = MNEM_Z80_EXX; return TOK_R800_MNEM; }
<R800>halt	{ yylval.i = MNEM_Z80_HALT; return TOK_R800_MNEM; }
<R800>im	{ yylval.i = MNEM_Z80_IM; return TOK_R800_MNEM; }
<R800>in	{ yylval.i = MNEM_R800_IN; return TOK_R800_MNEM; }
<R800>inc	{ yylval.i = MNEM_Z80_INC; return TOK_R800_MNEM; }
<R800>inm	{ yylval.i = MNEM_R800_INM; return TOK_R800_MNEM; }
<R800>br	{ yylval.i = MNEM_R800_BR; return TOK_R800_BRANCH; }
<R800>jr	{ yylval.i = MNEM_Z80_JR; return TOK_R800_MNEM; }
<R800>b(nz|z|nc|c)	{ yylval.i = _COND_MNEM(parse_cond_x80(yytext + 1), MNEM_I8080_J_CC); return TOK_R800_BRANCH; }
<R800>b(po|pe|p|m)	{ yylval.i = _COND_MNEM(parse_cond_x80(yytext + 1), MNEM_I8080_J_CC); return TOK_R800_MNEM; }
<R800>short	{ return TOK_SHORT; }
<R800>ld	{ yylval.i = MNEM_Z80_LD; return TOK_R800_MNEM; }
<R800>move	{ yylval.i = MNEM_R800_MOVE; return TOK_R800_MNEM; }
<R800>movem	{ yylval.i = MNEM_R800_MOVEM; return TOK_R800_MNEM; }
<R800>mulub	{ yylval.i = MNEM_R800_MULUB; return TOK_R800_MNEM; }
<R800>muluw	{ yylval.i = MNEM_R800_MULUW; return TOK_R800_MNEM; }
<R800>neg	{ yylval.i = MNEM_R800_NEG; return TOK_R800_MNEM; }
<R800>nop	{ yylval.i = MNEM_Z80_NOP; return TOK_R800_MNEM; }
<R800>or	{ yylval.i = MNEM_R800_OR; return TOK_R800_MNEM; }
<R800>out	{ yylval.i = MNEM_R800_OUT; return TOK_R800_MNEM; }
<R800>outm	{ yylval.i = MNEM_R800_OUTM; return TOK_R800_MNEM; }
<R800>pop	{ yylval.i = MNEM_Z80_POP; return TOK_R800_MNEM; }
<R800>push	{ yylval.i = MNEM_Z80_PUSH; return TOK_R800_MNEM; }
<R800>clr	{ yylval.i = MNEM_Z80_RES; return TOK_R800_MNEM; }
<R800>ret	{ yylval.i = MNEM_Z80_RET; return TOK_R800_MNEM; }
<R800>reti	{ yylval.i = MNEM_Z80_RETI; return TOK_R800_MNEM; }
<R800>retn	{ yylval.i = MNEM_Z80_RETN; return TOK_R800_MNEM; }
<R800>rolc	{ yylval.i = MNEM_Z80_RL; return TOK_R800_MNEM; }
<R800>rolca	{ yylval.i = MNEM_Z80_RLA; return TOK_R800_MNEM; }
<R800>rol	{ yylval.i = MNEM_Z80_RLC; return TOK_R800_MNEM; }
<R800>rola	{ yylval.i = MNEM_Z80_RLCA; return TOK_R800_MNEM; }
<R800>rol4	{ yylval.i = MNEM_R800_ROL4; return TOK_R800_MNEM; }
<R800>rorc	{ yylval.i = MNEM_Z80_RR; return TOK_R800_MNEM; }
<R800>rorca	{ yylval.i = MNEM_Z80_RRA; return TOK_R800_MNEM; }
<R800>ror	{ yylval.i = MNEM_Z80_RRC; return TOK_R800_MNEM; }
<R800>rora	{ yylval.i = MNEM_Z80_RRCA; return TOK_R800_MNEM; }
<R800>ror4	{ yylval.i = MNEM_R800_ROR4; return TOK_R800_MNEM; }
<R800>brk	{ yylval.i = MNEM_Z80_RST; return TOK_R800_MNEM; }
<R800>subc	{ yylval.i = MNEM_Z80_SBC; return TOK_R800_MNEM; }
<R800>setc	{ yylval.i = MNEM_Z80_SCF; return TOK_R800_MNEM; }
<R800>set	{ yylval.i = MNEM_Z80_SET; return TOK_R800_MNEM; }
<R800>shla?	{ yylval.i = MNEM_Z80_SLA; return TOK_R800_MNEM; }
<R800>shra	{ yylval.i = MNEM_Z80_SRA; return TOK_R800_MNEM; }
<R800>shr	{ yylval.i = MNEM_Z80_SRL; return TOK_R800_MNEM; }
<R800>sub	{ yylval.i = MNEM_R800_SUB; return TOK_R800_MNEM; }
<R800>xor	{ yylval.i = MNEM_R800_XOR; return TOK_R800_MNEM; }

<I8080>calln	{ yylval.i = MNEM_Z80_CALLN; return TOK_I8080_MNEM; }
<Z80>calln	{ yylval.i = MNEM_Z80_CALLN; return TOK_Z80_MNEM; }
<I8080>retem	{ yylval.i = MNEM_Z80_RETEM; return TOK_I8080_MNEM; }
<Z80>retem	{ yylval.i = MNEM_Z80_RETEM; return TOK_Z80_MNEM; }

<I8008,DP2200>ac[abcdehlm]	{ yylval.i = _COND_MNEM(i8008reg(yytext[2]), MNEM_I8008_AC_R); return TOK_I8008_MNEM; }
<I8008>aci	{ yylval.i = MNEM_I8008_ACI; return TOK_I8008_MNEM; }
<DP2200>ac	{ yylval.i = MNEM_I8008_ACI; return TOK_I8008_MNEM; }
<I8008,DP2200>ad[abcdehlm]	{ yylval.i = _COND_MNEM(i8008reg(yytext[2]), MNEM_I8008_AD_R); return TOK_I8008_MNEM; }
<I8008>adi	{ yylval.i = MNEM_I8008_ADI; return TOK_I8008_MNEM; }
<DP2200>ad	{ yylval.i = MNEM_I8008_ADI; return TOK_I8008_MNEM; }
<I8008>cal	{ yylval.i = MNEM_I8008_CAL; return TOK_I8008_MNEM; }
<DP2200>call	{ yylval.i = MNEM_I8008_CAL; return TOK_I8008_MNEM; }
<I8008,DP2200>c[ft][cpsz]	{ yylval.i = _COND_MNEM(i8008cond(yytext[1]), MNEM_I8008_C_CC); return TOK_I8008_MNEM; }
<I8008,DP2200>cp[abcdehlm]	{ yylval.i = _COND_MNEM(i8008reg(yytext[2]), MNEM_I8008_CP_R); return TOK_I8008_MNEM; }
<I8008>cpi	{ yylval.i = MNEM_I8008_CPI; return TOK_I8008_MNEM; }
<DP2200>cp	{ yylval.i = MNEM_I8008_CPI; return TOK_I8008_MNEM; }
<I8008>dc[bcdehl]	{ yylval.i = _COND_MNEM(i8008reg(yytext[2]), MNEM_I8008_DC_R); return TOK_I8008_MNEM; }
<I8008>hlt	{ yylval.i = MNEM_I8008_HLT; return TOK_I8008_MNEM; }
<DP2200>halt	{ yylval.i = MNEM_I8008_HLT; return TOK_I8008_MNEM; }
<I8008>in[bcdehl]	{ yylval.i = _COND_MNEM(i8008reg(yytext[2]), MNEM_I8008_IN_R); return TOK_I8008_MNEM; }
<I8008>inp	{ yylval.i = MNEM_I8008_INP; return TOK_I8008_MNEM; }
<I8008,DP2200>j[ft][cpsz]	{ yylval.i = _COND_MNEM(i8008cond(yytext[1]), MNEM_I8008_J_CC); return TOK_I8008_MNEM; }
<I8008,DP2200>jmp	{ yylval.i = MNEM_I8008_JMP; return TOK_I8008_MNEM; }
<I8008,DP2200>l[abcdehlm][abcdehlm]	{
		if(yytext[1] == yytext[2])
		{
			yylval.s = strdup(yytext);
			return TOK_IDENTIFIER;
		}
		yylval.i = _COND_MNEM((i8008reg(yytext[1]) << 3) | i8008reg(yytext[2]), MNEM_I8008_L_R_R);
		return TOK_I8008_MNEM;
	}
<I8008>l[abcdehlm]i	{ yylval.i = _COND_MNEM(i8008reg(yytext[1]), MNEM_I8008_L_R_I); return TOK_I8008_MNEM; }
<DP2200>l[abcdehlm]	{ yylval.i = _COND_MNEM(i8008reg(yytext[1]), MNEM_I8008_L_R_I); return TOK_I8008_MNEM; }
<I8008,DP2200>nd[abcdehlm]	{ yylval.i = _COND_MNEM(i8008reg(yytext[2]), MNEM_I8008_ND_R); return TOK_I8008_MNEM; }
<I8008>ndi	{ yylval.i = MNEM_I8008_NDI; return TOK_I8008_MNEM; }
<DP2200>nd	{ yylval.i = MNEM_I8008_NDI; return TOK_I8008_MNEM; }
<I8008,DP2200>nop	{ yylval.i = MNEM_I8008_NOP; return TOK_I8008_MNEM; }
<I8008,DP2200>or[abcdehlm]	{ yylval.i = _COND_MNEM(i8008reg(yytext[2]), MNEM_I8008_OR_R); return TOK_I8008_MNEM; }
<I8008>ori	{ yylval.i = MNEM_I8008_ORI; return TOK_I8008_MNEM; }
<DP2200>or	{ yylval.i = MNEM_I8008_ORI; return TOK_I8008_MNEM; }
<I8008>out	{ yylval.i = MNEM_I8008_OUT; return TOK_I8008_MNEM; }
<I8008>ral	{ yylval.i = MNEM_I8008_RAL; return TOK_I8008_MNEM; }
<I8008>rar	{ yylval.i = MNEM_I8008_RAR; return TOK_I8008_MNEM; }
<I8008>ret	{ yylval.i = MNEM_I8008_RET; return TOK_I8008_MNEM; }
<DP2200>return	{ yylval.i = MNEM_I8008_RET; return TOK_I8008_MNEM; }
<I8008,DP2200>r[ft][cpsz]	{ yylval.i = _COND_MNEM(i8008cond(yytext[1]), MNEM_I8008_R_CC); return TOK_I8008_MNEM; }
<I8008>rlc	{ yylval.i = MNEM_I8008_RLC; return TOK_I8008_MNEM; }
<DP2200>slc	{ yylval.i = MNEM_I8008_RLC; return TOK_I8008_MNEM; }
<I8008>rrc	{ yylval.i = MNEM_I8008_RRC; return TOK_I8008_MNEM; }
<DP2200>src	{ yylval.i = MNEM_I8008_RRC; return TOK_I8008_MNEM; }
<I8008>rst	{ yylval.i = MNEM_I8008_RST; return TOK_I8008_MNEM; }
<I8008,DP2200>sb[abcdehlm]	{ yylval.i = _COND_MNEM(i8008reg(yytext[2]), MNEM_I8008_SB_R); return TOK_I8008_MNEM; }
<I8008>sbi	{ yylval.i = MNEM_I8008_SBI; return TOK_I8008_MNEM; }
<DP2200>sb	{ yylval.i = MNEM_I8008_SBI; return TOK_I8008_MNEM; }
<I8008,DP2200>su[abcdehlm]	{ yylval.i = _COND_MNEM(i8008reg(yytext[2]), MNEM_I8008_SU_R); return TOK_I8008_MNEM; }
<I8008>sui	{ yylval.i = MNEM_I8008_SUI; return TOK_I8008_MNEM; }
<DP2200>su	{ yylval.i = MNEM_I8008_SUI; return TOK_I8008_MNEM; }
<I8008,DP2200>xr[abcdehlm]	{ yylval.i = _COND_MNEM(i8008reg(yytext[2]), MNEM_I8008_XR_R); return TOK_I8008_MNEM; }
<I8008>xri	{ yylval.i = MNEM_I8008_XRI; return TOK_I8008_MNEM; }
<DP2200>xr	{ yylval.i = MNEM_I8008_XRI; return TOK_I8008_MNEM; }

<DP2200>input	{ yylval.i = MNEM_DP2200_INPUT; return TOK_I8008_MNEM; }
<DP2200>ex[ \t]+adr/[^A-Za-z_0-9*]	{ yylval.i = _COND_MNEM(8, MNEM_DP2200_EX); return TOK_I8008_MNEM; }
<DP2200>ex[ \t]+status/[^A-Za-z_0-9*]	{ yylval.i = _COND_MNEM(9, MNEM_DP2200_EX); return TOK_I8008_MNEM; }
<DP2200>ex[ \t]+data/[^A-Za-z_0-9*]	{ yylval.i = _COND_MNEM(10, MNEM_DP2200_EX); return TOK_I8008_MNEM; }
<DP2200>ex[ \t]+write/[^A-Za-z_0-9*]	{ yylval.i = _COND_MNEM(11, MNEM_DP2200_EX); return TOK_I8008_MNEM; }
<DP2200>ex[ \t]+com1/[^A-Za-z_0-9*]	{ yylval.i = _COND_MNEM(12, MNEM_DP2200_EX); return TOK_I8008_MNEM; }
<DP2200>ex[ \t]+com2/[^A-Za-z_0-9*]	{ yylval.i = _COND_MNEM(13, MNEM_DP2200_EX); return TOK_I8008_MNEM; }
<DP2200>ex[ \t]+com3/[^A-Za-z_0-9*]	{ yylval.i = _COND_MNEM(14, MNEM_DP2200_EX); return TOK_I8008_MNEM; }
<DP2200>ex[ \t]+com4/[^A-Za-z_0-9*]	{ yylval.i = _COND_MNEM(15, MNEM_DP2200_EX); return TOK_I8008_MNEM; }
<DP2200>ex[ \t]+beep/[^A-Za-z_0-9*]	{ yylval.i = _COND_MNEM(20, MNEM_DP2200_EX); return TOK_I8008_MNEM; }
<DP2200>ex[ \t]+click/[^A-Za-z_0-9*]	{ yylval.i = _COND_MNEM(21, MNEM_DP2200_EX); return TOK_I8008_MNEM; }
<DP2200>ex[ \t]+deck1/[^A-Za-z_0-9*]	{ yylval.i = _COND_MNEM(22, MNEM_DP2200_EX); return TOK_I8008_MNEM; }
<DP2200>ex[ \t]+deck2/[^A-Za-z_0-9*]	{ yylval.i = _COND_MNEM(23, MNEM_DP2200_EX); return TOK_I8008_MNEM; }
<DP2200>ex[ \t]+brk/[^A-Za-z_0-9*]	{ yylval.i = _COND_MNEM(24, MNEM_DP2200_EX); return TOK_I8008_MNEM; }
<DP2200>ex[ \t]+wbk/[^A-Za-z_0-9*]	{ yylval.i = _COND_MNEM(25, MNEM_DP2200_EX); return TOK_I8008_MNEM; }
<DP2200>ex[ \t]+bsp/[^A-Za-z_0-9*]	{ yylval.i = _COND_MNEM(27, MNEM_DP2200_EX); return TOK_I8008_MNEM; }
<DP2200>ex[ \t]+sf/[^A-Za-z_0-9*]	{ yylval.i = _COND_MNEM(28, MNEM_DP2200_EX); return TOK_I8008_MNEM; }
<DP2200>ex[ \t]+sb/[^A-Za-z_0-9*]	{ yylval.i = _COND_MNEM(29, MNEM_DP2200_EX); return TOK_I8008_MNEM; }
<DP2200>ex[ \t]+rewnd/[^A-Za-z_0-9*]	{ yylval.i = _COND_MNEM(30, MNEM_DP2200_EX); return TOK_I8008_MNEM; }
<DP2200>ex[ \t]+tstop/[^A-Za-z_0-9*]	{ yylval.i = _COND_MNEM(31, MNEM_DP2200_EX); return TOK_I8008_MNEM; }
<DP2200>ex	{ yylval.i = MNEM_I8008_OUT; return TOK_I8008_MNEM; }

<DP2200>alpha	{ yylval.i = MNEM_DP2200_ALPHA; return TOK_I8008_MNEM; }
<DP2200>beta	{ yylval.i = MNEM_DP2200_BETA; return TOK_I8008_MNEM; }
<DP2200>di	{ yylval.i = MNEM_DP2200_DI; return TOK_I8008_MNEM; }
<DP2200>ei	{ yylval.i = MNEM_DP2200_EI; return TOK_I8008_MNEM; }
<DP2200>pop	{ yylval.i = MNEM_DP2200_POP; return TOK_I8008_MNEM; }
<DP2200>push	{ yylval.i = MNEM_DP2200_PUSH; return TOK_I8008_MNEM; }

%%

void setup_lexer(parser_state_t * state)
{
	switch(state->syntax)
	{
	case SYNTAX_DP2200:
		BEGIN(DP2200);
		break;
	case SYNTAX_I8008:
		BEGIN(I8008);
		break;
	case SYNTAX_INTEL:
		BEGIN(I8080);
		break;
	case SYNTAX_ZILOG:
		BEGIN(Z80);
		break;
	case SYNTAX_ASCII:
		BEGIN(R800);
		break;
	default:
		assert(false);
		break;
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

static const int _i8008reg['m' - 'a' + 1] =
{
	['a' - 'a'] = 0,
	['b' - 'a'] = 1,
	['c' - 'a'] = 2,
	['d' - 'a'] = 3,
	['e' - 'a'] = 4,
	['h' - 'a'] = 5,
	['l' - 'a'] = 6,
	['m' - 'a'] = 7,
};

static const int _i8008cond['z' - 'c' + 1] =
{
	['c' - 'c'] = 0,
	['z' - 'c'] = 1,
	['s' - 'c'] = 2,
	['p' - 'c'] = 3,
};

