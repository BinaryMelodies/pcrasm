#ifndef _X86_ISA_H
#define _X86_ISA_H

#include <stdio.h>
#include "../asm.h"
#include "../elf.h"
#include "../coff.h"
#include "../../obj/x86/x86/parser.tab.h"

#ifndef TARGET_NAME
# define TARGET_NAME "x86"
#endif

#define ARCH_BITS_IN_UNIT 8
#define ARCH_BITS_IN_CHAR 8

enum cpu_type_t
{
	// Intel and generic CPUs
	CPU_8086 = 1,
	CPU_186,
	// NEC CPUs
	CPU_V30,
	CPU_9002,
	CPU_V33,
	CPU_V25,
	CPU_V55,
	// Intel and generic CPUs
	CPU_286,
	CPU_386,
	CPU_386B1,
	CPU_486,
	CPU_486B,
	CPU_586,
	// Cyrix CPUs
	CPU_CYRIX,
	CPU_MX, /* Cyrix 6x86MX/MII and MediaGX */
	CPU_GX, /* MediaGX, not MII, III */
	CPU_GX2,
	// Intel, AMD and generic CPUs
	CPU_IA64, // x86 mode on Itanium
	CPU_X64,

	// not an x86 CPU
	CPU_8080,
	CPU_Z80,

	// not an x86 CPU, but a coprocessor, with a different instruction set
	CPU_8089,

	// these are not implemented, but included as they appear in the source code
	CPU_8085 = -1,
	CPU_Z280 = -1,
	CPU_Z180 = -1,
	CPU_Z380 = -1,
	CPU_EZ80 = -1,
	CPU_R800 = -1,
	CPU_GBZ80 = -1,

	CPU_686 = CPU_586,
	CPU_ALL = CPU_X64
};
typedef enum cpu_type_t cpu_type_t;

enum fpu_type_t
{
	FPU_NONE,
	FPU_8087,
	FPU_287,
	FPU_387,
	FPU_INTEGRATED = FPU_387,
};
typedef enum fpu_type_t fpu_type_t;

typedef enum instruction_set_t
{
	ISA_X86,
	ISA_X80,
	ISA_8089,
} instruction_set_t;

enum operand_type_t
{
	OPD_IMM,
	OPD_GPRB,
	OPD_GPRW,
	OPD_GPRD,
	OPD_GPRQ,
	OPD_GPRH,
	OPD_SEG,
	OPD_STREG,
	OPD_CREG,
	OPD_DREG,
	OPD_TREG,
	OPD_MMREG,
	OPD_XMMREG,
	OPD_YMMREG,
	OPD_ZMMREG,
	OPD_MEM,
	OPD_FARIMM,
	OPD_FARMEM,
	// NEC specific
	OPD_PSW,
	OPD_RREG,
	OPD_CY,
	OPD_DIR,
	// 8080/Z80 specific
	OPD_TOKEN,
};
typedef enum operand_type_t operand_type_t;

#define _OPD(type, value) ((type) << 8 | ((value) & 0xFF))
#define _OPD_TYPE(value) ((value) >> 8)
#define _OPD_VALUE(value) ((value) & 0xFF)

enum regnumber_t
{
	REG_AX = 0,
	REG_CX,
	REG_DX,
	REG_BX,
	REG_SP,
	REG_BP,
	REG_SI,
	REG_DI,

	REG_IP = 32,
	REG_IPREL = 33,

	REG_AL = 0,
	REG_CL,
	REG_DL,
	REG_BL,
	REG_AH,
	REG_CH,
	REG_DH,
	REG_BH,

	REG_NONE = -1,
};
typedef enum regnumber_t regnumber_t;

enum segment_t
{
	REG_ES = 0,
	REG_CS,
	REG_SS,
	REG_DS,
	REG_FS,
	REG_GS,
	REG_DS3,
	REG_DS2,
	REG_IRAM,

	REG_NOSEG = -1,
};
typedef enum segment_t segment_t;

typedef enum x80_regnumber_t
{
	X80_A,
	X80_AF,
	X80_AF2,
	X80_B,
	X80_BC,
	X80_C,
	X80_CND_M,
	X80_D,
	X80_DE,
	X80_E,
	X80_H,
	X80_HL,
	X80_I,
	X80_IX,
	X80_IXH,
	X80_IXL,
	X80_IY,
	X80_IYH,
	X80_IYL,
	X80_L,
	X80_NC,
	X80_NZ,
	X80_P,
	X80_PE,
	X80_PO,
	X80_PSW,
	X80_R,
	X80_REG_M,
	X80_SP,
	X80_Z,
} x80_regnumber_t;

typedef enum x89_regnumber_t
{
	X89_GA,
	X89_GB,
	X89_GC,
	X89_BC,
	X89_TP,
	X89_IX,
	X89_CC,
	X89_MC,

	X89_PP = X89_BC,
} x89_regnumber_t;

enum repeat_t
{
	PREF_NOREP,
	PREF_REP,
	PREF_REPE,
	PREF_REPNE,
	PREF_REPC,
	PREF_REPNC,
};
typedef enum repeat_t repeat_t;

enum condition_t
{
	COND_O,
	COND_NO,
	COND_C,
	COND_NC,
	COND_Z,
	COND_NZ,
	COND_BE,
	COND_NBE,
	COND_S,
	COND_NS,
	COND_P,
	COND_NP,
	COND_L,
	COND_NL,
	COND_LE,
	COND_NLE,

	COND_A = COND_NBE,
	COND_AE = COND_NC,
	COND_B = COND_C,
	COND_E = COND_Z,
	COND_G = COND_NLE,
	COND_GE = COND_NL,
	COND_NB = COND_NC,
	COND_NA = COND_BE,
	COND_NAE = COND_B,
	COND_NE = COND_NZ,
	COND_NG = COND_LE,
	COND_NGE = COND_L,
	COND_PE = COND_P,
	COND_PO = COND_NP,

	// NEC
	COND_GT = COND_G,
	COND_H = COND_A,
	COND_L_NEC = COND_B,
	COND_LT = COND_L,
	COND_NH = COND_NA,
	COND_NL_NEC = COND_NB,
	COND_NV = COND_NO,
	COND_N = COND_S,
	COND_P_NEC = COND_NS,
	COND_V = COND_O,

	COND_NONE = -1,
};
typedef enum condition_t condition_t;

enum mnemonic_t
{
	MNEM_NONE,

	MNEM_AAA,
	MNEM_AAD,
	MNEM_AAM,
	MNEM_AAS,
	MNEM_ADC,
	MNEM_ADD,
	MNEM_ADD4S,
	MNEM_ALBIT,
	MNEM_AND,
	MNEM_ARPL,
	MNEM_BB0_RESET,
	MNEM_BB1_RESET,
	MNEM_BOUND,
	MNEM_BRK,
	MNEM_BRKCS,
	MNEM_BRKEM,
	MNEM_BRKEM2, // BRKFEM,
	MNEM_BRKN,
	MNEM_BRKS,
	MNEM_BRKXA,
	MNEM_BSCH,
	MNEM_BSF,
	MNEM_BSR,
	MNEM_BSWAP,
	MNEM_BT,
	MNEM_BTC,
	MNEM_BTCLR,
	MNEM_BTCLRL,
	MNEM_BTR,
	MNEM_BTS,
	MNEM_CALL,
	MNEM_CBW,
	MNEM_CDQ,
	MNEM_CDQE,
	MNEM_CLC,
	MNEM_CLD,
	MNEM_CLI,
	MNEM_CLR1,
	MNEM_CLTS,
	MNEM_CMC,
	MNEM_CMOV_CC,
	MNEM_CMP,
	MNEM_CMP4S,
	MNEM_CMPS,
	MNEM_CMPSB,
	MNEM_CMPSW,
	MNEM_CMPSD,
	MNEM_CMPSQ,
	MNEM_CMPXCHG,
	MNEM_CMPXCHG16B,
	MNEM_CMPXCHG8B,
	MNEM_CNVTRP,
	MNEM_COLTRP,
	MNEM_CPU_READ,
	MNEM_CPU_WRITE,
	MNEM_CPUID,
	MNEM_CQO,
	MNEM_CWD,
	MNEM_CWDE,
	MNEM_DAA,
	MNEM_DAS,
	MNEM_DEC,
	MNEM_DIV,
	MNEM_DMINT,
	MNEM_ENTER,
	MNEM_EXT,
	MNEM_FINT,
	MNEM_GETBIT,
	MNEM_HLT,
	MNEM_IBTS,
	MNEM_IDIV,
	MNEM_IDLE,
	MNEM_IMUL,
	MNEM_IN,
	MNEM_INC,
	MNEM_INS,
	MNEM_INSB,
	MNEM_INSW,
	MNEM_INSD,
	MNEM_INS_NEC,
	MNEM_INT,
	MNEM_INT1, // ICEBP
	MNEM_INT3,
	MNEM_INTO,
	MNEM_INVD,
	MNEM_INVLPG,
	MNEM_IRET,
	MNEM_IRETD,
	MNEM_IRETQ,
	MNEM_J_CC,
	MNEM_JCXZ,
	MNEM_JECXZ,
	MNEM_JRCXZ,
	MNEM_JMP,
	MNEM_JMPE,
	MNEM_LAHF,
	MNEM_LAR,
	MNEM_LDEA,
	MNEM_LDS,
	MNEM_LEA,
	MNEM_LEAVE,
	MNEM_LES,
	MNEM_LFS,
	MNEM_LGDT,
	MNEM_LGS,
	MNEM_LIDT,
	MNEM_LLDT,
	MNEM_LMSW,
	MNEM_LOADALL,
	MNEM_LOADALL286,
	MNEM_LOADALL386, // LOADALLD
	MNEM_LODS,
	MNEM_LODSB,
	MNEM_LODSW,
	MNEM_LODSD,
	MNEM_LODSQ,
	MNEM_LOOP,
	MNEM_LOOPE,
	MNEM_LOOPNE,
	MNEM_LSL,
	MNEM_LSS,
	MNEM_LTR,
	MNEM_LZCNT,
	MNEM_MHDEC,
	MNEM_MHENC,
	MNEM_MOV,
	MNEM_MOV_NEC,
	MNEM_MOVS,
	MNEM_MOVSB,
	MNEM_MOVSW,
	MNEM_MOVSD,
	MNEM_MOVSQ,
	MNEM_MOVSPA,
	MNEM_MOVSPB,
	MNEM_MOVSX,
	MNEM_MOVSXD,
	MNEM_MOVZX,
	MNEM_MRDEC,
	MNEM_MRENC,
	MNEM_MUL,
	MNEM_MUL_NEC,
	MNEM_MULU,
	MNEM_NEG,
	MNEM_NOP,
	MNEM_NOPL,
	MNEM_NOT,
	MNEM_NOT1,
	MNEM_OR,
	MNEM_OUT,
	MNEM_OUTS,
	MNEM_OUTSB,
	MNEM_OUTSW,
	MNEM_OUTSD,
	MNEM_POP,
	MNEM_POPA,
	MNEM_POPAD,
	MNEM_POPCNT,
	MNEM_POPF,
	MNEM_POPFD,
	MNEM_POPFQ,
	MNEM_PUSH,
	MNEM_PUSHA,
	MNEM_PUSHAD,
	MNEM_PUSHF,
	MNEM_PUSHFD,
	MNEM_PUSHFQ,
	MNEM_QHOUT,
	MNEM_QOUT,
	MNEM_QTIN,
	MNEM_RCL,
	MNEM_RCR,
	MNEM_RDM,
	MNEM_RDFSBASE,
	MNEM_RDGSBASE,
	MNEM_RDMSR,
	MNEM_RDPID,
	MNEM_RDPMC,
	MNEM_RDSHR,
	MNEM_RDTSC,
	MNEM_RDTSCP,
	MNEM_RET,
	MNEM_RETW,
	MNEM_RETD,
	MNEM_RETQ,
	MNEM_RETF,
	MNEM_RETFW,
	MNEM_RETFD,
	MNEM_RETFQ,
	MNEM_RETI,
	MNEM_RETRBI,
	MNEM_RETXA,
	MNEM_ROL,
	MNEM_ROL4,
	MNEM_ROR,
	MNEM_ROR4,
	MNEM_RSDC,
	MNEM_RSM,
	MNEM_RSLDT,
	MNEM_RSTS,
	MNEM_RSTWDT,
	MNEM_SAHF,
	MNEM_SAL,
	MNEM_SALC,
	MNEM_SAR,
	MNEM_SBB,
	MNEM_SCAS,
	MNEM_SCASB,
	MNEM_SCASW,
	MNEM_SCASD,
	MNEM_SCASQ,
	MNEM_SCHEOL,
	MNEM_SET1,
	MNEM_SET_CC,
	MNEM_SETMO,
	MNEM_SETMOC,
	MNEM_SGDT,
	MNEM_SHL,
	MNEM_SHLD,
	MNEM_SHR,
	MNEM_SHRD,
	MNEM_SIDT,
	MNEM_SLDT,
	MNEM_SMINT,
	MNEM_SMSW,
	MNEM_STC,
	MNEM_STD,
	MNEM_STI,
	MNEM_STOP,
	MNEM_STOREALL,
	MNEM_STOS,
	MNEM_STOSB,
	MNEM_STOSW,
	MNEM_STOSD,
	MNEM_STOSQ,
	MNEM_STR,
	MNEM_SUB,
	MNEM_SUB4S,
	MNEM_SVDC,
	MNEM_SVLDT,
	MNEM_SVTS,
	MNEM_SWAPGS,
	MNEM_SYSCALL,
	MNEM_SYSENTER,
	MNEM_SYSEXIT,
	MNEM_SYSRET,
	MNEM_TEST,
	MNEM_TEST1,
	MNEM_TSKSW,
	MNEM_TZCNT,
	MNEM_UD0,
	MNEM_UD1,
	MNEM_UD2,
	MNEM_UMOV,
	MNEM_VERR,
	MNEM_VERW,
	MNEM_WAIT,
	MNEM_WBINVD,
	MNEM_WRFSBASE,
	MNEM_WRGSBASE,
	MNEM_WRMSR,
	MNEM_WRSHR,
	MNEM_XADD,
	MNEM_XBTS,
	MNEM_XCHG,
	MNEM_XLAT,
	MNEM_XOR,

	MNEM_F2XM1,
	MNEM_FABS,
	MNEM_FADD,
	MNEM_FADDP,
	MNEM_FBLD,
	MNEM_FBSTP,
	MNEM_FCHS,
	MNEM_FNCLEX,
	MNEM_FCLEX,
	MNEM_FCMOV_CC,
	MNEM_FCOM,
	MNEM_FCOMI,
	MNEM_FCOMIP,
	MNEM_FCOMP,
	MNEM_FCOMPP,
	MNEM_FCOS,
	MNEM_FDECSTP,
	MNEM_FNDISI,
	MNEM_FDISI,
	MNEM_FDIV,
	MNEM_FDIVP,
	MNEM_FDIVR,
	MNEM_FDIVRP,
	MNEM_FNENI,
	MNEM_FENI,
	MNEM_FFREE,
	MNEM_FFREEP,
	MNEM_FIADD,
	MNEM_FICOM,
	MNEM_FICOMP,
	MNEM_FIDIV,
	MNEM_FIDIVR,
	MNEM_FILD,
	MNEM_FIMUL,
	MNEM_FINCSTP,
	MNEM_FNINIT,
	MNEM_FINIT,
	MNEM_FIST,
	MNEM_FISTP,
	MNEM_FISTTP,
	MNEM_FISUB,
	MNEM_FISUBR,
	MNEM_FLD,
	MNEM_FLD1,
	MNEM_FLDCW,
	MNEM_FLDENV,
	MNEM_FLDL2E,
	MNEM_FLDL2T,
	MNEM_FLDLG2,
	MNEM_FLDLN2,
	MNEM_FLDPI,
	MNEM_FLDZ,
	MNEM_FMUL,
	MNEM_FMULP,
	MNEM_FNOP,
	MNEM_FPATAN,
	MNEM_FPREM,
	MNEM_FPREM1,
	MNEM_FPTAN,
	MNEM_FRNDINT,
	MNEM_FRSTOR,
	MNEM_FRSTPM,
	MNEM_FNSAVE,
	MNEM_FSAVE,
	MNEM_FSCALE,
	MNEM_FNSETPM,
	MNEM_FSETPM,
	MNEM_FSIN,
	MNEM_FSINCOS,
	MNEM_FSQRT,
	MNEM_FST,
	MNEM_FNSTCW,
	MNEM_FSTCW,
	MNEM_FNSTDW,
	MNEM_FSTDW,
	MNEM_FNSTENV,
	MNEM_FSTENV,
	MNEM_FSTP,
	MNEM_FSTPNCE,
	MNEM_FNSTSG,
	MNEM_FSTSG,
	MNEM_FNSTSW,
	MNEM_FSTSW,
	MNEM_FSUB,
	MNEM_FSUBP,
	MNEM_FSUBR,
	MNEM_FSUBRP,
	MNEM_FTST,
	MNEM_FUCOM,
	MNEM_FUCOMI,
	MNEM_FUCOMIP,
	MNEM_FUCOMP,
	MNEM_FUCOMPP,
	MNEM_FXAM,
	MNEM_FXCH,
	MNEM_FXTRACT,
	MNEM_FYL2X,
	MNEM_FYL2XP1,

	MNEM_I8080_ACI,
	MNEM_I8080_ADC,
	MNEM_I8080_ADD,
	MNEM_I8080_ADI,
	MNEM_I8080_ANA,
	MNEM_I8080_ANI,
	MNEM_I8080_CALL,
	MNEM_I8080_C_CC,
	MNEM_I8080_CMA,
	MNEM_Z80_CPL = MNEM_I8080_CMA,
	MNEM_I8080_CMC,
	MNEM_Z80_CCF = MNEM_I8080_CMC,
	MNEM_I8080_CMP,
	MNEM_I8080_CPI,
	MNEM_I8080_DAA,
	MNEM_Z80_DAA = MNEM_I8080_DAA,
	MNEM_I8080_DAD,
	MNEM_I8080_DCR,
	MNEM_I8080_DCX,
	MNEM_I8080_DI,
	MNEM_Z80_DI = MNEM_I8080_DI,
	MNEM_I8080_EI,
	MNEM_Z80_EI = MNEM_I8080_EI,
	MNEM_I8080_HLT,
	MNEM_Z80_HALT = MNEM_I8080_HLT,
	MNEM_I8080_IN,
	MNEM_I8080_INR,
	MNEM_I8080_INX,
	MNEM_I8080_J_CC,
	MNEM_I8080_JMP,
	MNEM_I8080_LDA,
	MNEM_I8080_LDAX,
	MNEM_I8080_LHLD,
	MNEM_I8080_LXI,
	MNEM_I8080_MOV,
	MNEM_I8080_MVI,
	MNEM_I8080_NOP,
	MNEM_Z80_NOP = MNEM_I8080_NOP,
	MNEM_I8080_ORA,
	MNEM_I8080_ORI,
	MNEM_I8080_OUT,
	MNEM_I8080_PCHL,
	MNEM_I8080_POP,
	MNEM_Z80_POP = MNEM_I8080_POP,
	MNEM_I8080_PUSH,
	MNEM_Z80_PUSH = MNEM_I8080_PUSH,
	MNEM_I8080_RAL,
	MNEM_Z80_RLA = MNEM_I8080_RAL,
	MNEM_I8080_RAR,
	MNEM_Z80_RRA = MNEM_I8080_RAR,
	MNEM_I8080_R_CC,
	MNEM_I8080_RET,
	MNEM_I8080_RLC,
	MNEM_Z80_RLCA = MNEM_I8080_RLC,
	MNEM_I8080_RRC,
	MNEM_Z80_RRCA = MNEM_I8080_RRC,
	MNEM_I8080_RST,
	MNEM_I8080_SBB,
	MNEM_I8080_SBI,
	MNEM_I8080_SHLD,
	MNEM_I8080_SPHL,
	MNEM_I8080_STA,
	MNEM_I8080_STAX,
	MNEM_I8080_STC,
	MNEM_Z80_SCF = MNEM_I8080_STC,
	MNEM_I8080_SUB,
	MNEM_I8080_SUI,
	MNEM_I8080_XCHG,
	MNEM_I8080_XRA,
	MNEM_I8080_XRI,
	MNEM_I8080_XTHL,

	MNEM_Z80_ADC,
	MNEM_Z80_ADD,
	MNEM_Z80_AND,
	MNEM_Z80_BIT,
	MNEM_Z80_CALL,
	MNEM_Z80_CP,
	MNEM_Z80_CPD,
	MNEM_Z80_CPDR,
	MNEM_Z80_CPI,
	MNEM_Z80_CPIR,
	MNEM_Z80_DEC,
	MNEM_Z80_DJNZ,
	MNEM_Z80_EX,
	MNEM_Z80_EXX,
	MNEM_Z80_IM,
	MNEM_Z80_IN,
	MNEM_Z80_INC,
	MNEM_Z80_IND,
	MNEM_Z80_INDR,
	MNEM_Z80_INI,
	MNEM_Z80_INIR,
	MNEM_Z80_JP,
	MNEM_Z80_JR,
	MNEM_Z80_LD,
	MNEM_Z80_LDD,
	MNEM_Z80_LDDR,
	MNEM_Z80_LDI,
	MNEM_Z80_LDIR,
	MNEM_Z80_NEG,
	MNEM_Z80_OR,
	MNEM_Z80_OTDR,
	MNEM_Z80_OTIR,
	MNEM_Z80_OUT,
	MNEM_Z80_OUTD,
	MNEM_Z80_OUTI,
	MNEM_Z80_RES,
	MNEM_Z80_RET,
	MNEM_Z80_RETI,
	MNEM_Z80_RETN,
	MNEM_Z80_RL,
	MNEM_Z80_RLC,
	MNEM_Z80_RLD,
	MNEM_Z80_RR,
	MNEM_Z80_RRC,
	MNEM_Z80_RRD,
	MNEM_Z80_RST,
	MNEM_Z80_SBC,
	MNEM_Z80_SET,
	MNEM_Z80_SLA,
	MNEM_Z80_SLL,
	MNEM_Z80_SRA,
	MNEM_Z80_SRL,
	MNEM_Z80_SUB,
	MNEM_Z80_XOR,

	MNEM_Z80_CALLN,
	MNEM_Z80_RETEM,

	MNEM_I8089_ADD,
	MNEM_I8089_ADDB,
	MNEM_I8089_ADDBI,
	MNEM_I8089_ADDI,
	MNEM_I8089_AND,
	MNEM_I8089_ANDB,
	MNEM_I8089_ANDBI,
	MNEM_I8089_ANDI,
	MNEM_I8089_CALL,
	MNEM_I8089_CLR,
	MNEM_I8089_DEC,
	MNEM_I8089_DECB,
	MNEM_I8089_HLT,
	MNEM_I8089_INC,
	MNEM_I8089_INCB,
	MNEM_I8089_JBT,
	MNEM_I8089_JMCE,
	MNEM_I8089_JMCNE,
	MNEM_I8089_JMP,
	MNEM_I8089_JNBT,
	MNEM_I8089_JNZ,
	MNEM_I8089_JNZB,
	MNEM_I8089_JZ,
	MNEM_I8089_JZB,
	MNEM_I8089_LCALL,
	MNEM_I8089_LJBT,
	MNEM_I8089_LJMCE,
	MNEM_I8089_LJMCNE,
	MNEM_I8089_LJMP,
	MNEM_I8089_LJNBT,
	MNEM_I8089_LJNZ,
	MNEM_I8089_LJNZB,
	MNEM_I8089_LJZ,
	MNEM_I8089_LJZB,
	MNEM_I8089_LPD,
	MNEM_I8089_LPDI,
	MNEM_I8089_MOV,
	MNEM_I8089_MOVB,
	MNEM_I8089_MOVBI,
	MNEM_I8089_MOVI,
	MNEM_I8089_MOVP,
	MNEM_I8089_NOP,
	MNEM_I8089_NOT,
	MNEM_I8089_NOTB,
	MNEM_I8089_OR,
	MNEM_I8089_ORB,
	MNEM_I8089_ORBI,
	MNEM_I8089_ORI,
	MNEM_I8089_SETB,
	MNEM_I8089_SINTR,
	MNEM_I8089_TSL,
	MNEM_I8089_WID,
	MNEM_I8089_XFER,

	_MNEM_TOTAL,

	_MNEMONIC_COMMON_ENUMERATORS
};
typedef enum mnemonic_t mnemonic_t;

typedef enum x89_address_mode_t
{
	X89_BASE,
	X89_OFFSET,
	X89_INDEX,
	X89_INCREMENT,
} x89_address_mode_t;

#define _COND_MNEM(cond, mnem) (((mnem) & 0x0FFF) | ((cond) << 12))
#define _GET_MNEM(value) ((value) & 0x0FFF)
#define _GET_COND(value) ((value) >> 12)
struct operand_t
{
	operand_type_t type;
	bitsize_t size;
	bitsize_t address_size;
	segment_t segment;
	regnumber_t base;
	regnumber_t index;
	size_t scale;
	x89_address_mode_t x89_mode;
	expression_t * parameter;
	expression_t * segment_value;
};

#ifndef MAX_OPD_COUNT
# define MAX_OPD_COUNT 4
#endif

typedef struct instruction_pattern_t instruction_pattern_t;

struct instruction_patterns_t
{
	const instruction_pattern_t * pattern;
	size_t count;
};
typedef struct instruction_patterns_t instruction_patterns_t;

typedef struct pattern_t pattern_t;
struct pattern_t
{
	struct instruction_patterns_t pattern[MAX_OPD_COUNT + 1];
};

struct instruction_t
{
	_INSTRUCTION_COMMON_FIELDS

	instruction_set_t isa;
	cpu_type_t cpu;
	bitsize_t bits;
	fpu_type_t fpu;
	condition_t condition;
	bool lock_prefix;
	bool umov_prefix; // 286 only
	segment_t segment_prefix;
	segment_t source_segment_prefix;
	segment_t target_segment_prefix;
	repeat_t repeat_prefix;
	bitsize_t operation_size;
	bitsize_t address_size;
};

typedef struct parser_state_t
{
	instruction_stream_t stream;
	size_t line_number;

	cpu_type_t cpu_type;
	cpu_type_t x80_cpu_type;
	fpu_type_t fpu_type;
	bitsize_t bit_size;
} parser_state_t;

extern parser_state_t current_parser_state[1];
extern instruction_t current_instruction;

#define gprsize(gprtype) ((bitsize_t)(8 << ((gprtype) - OPD_GPRB)))

extern size_t x86_instruction_compute_length(instruction_t * ins, bool forgiving);
#ifndef instruction_compute_length
# define instruction_compute_length x86_instruction_compute_length
#endif

extern void x86_generate_instruction(instruction_t * ins);
#ifndef generate_instruction
# define generate_instruction x86_generate_instruction
#endif

#ifndef elf_machine_type
# define elf_machine_type() (output.format != FORMAT_ELF64 ? EM_386 : EM_X86_64)
#endif

#ifndef elf_default_byte_order
# define elf_default_byte_order() ELFDATA2LSB
#endif

#ifndef elf_backend_uses_rela
# define elf_backend_uses_rela() (output.format == FORMAT_ELF64)
#endif

static inline int elf32_get_relocation_type(relocation_t rel)
{
	switch(rel.size)
	{
	case BITSIZE8:
		if(rel.pc_relative)
			return R_386_PC8;
		else
			return R_386_8;
	case BITSIZE16:
		if(rel.var.segment_of)
		{
			switch(elf32_segments)
			{
			case ELF32_NO_SEGMENTS:
				fprintf(stderr, "Segmentation not supported\n");
				return R_386_16;
			case ELF32_RETROLINKER:
				// modify symbol name
				return R_386_16;
			case ELF32_VMA_SEGMENTS:
				return R_386_OZSEG16;
			case ELF32_SEGELF:
				return R_386_SEG16;
			}
		}
		else if(rel.pc_relative)
			return R_386_PC16;
		else
			return R_386_16;
	case BITSIZE64:
		if(rel.pc_relative)
			return R_386_PC32;
		else
			return R_386_32;
	default:
		return -1;
	}
}

static inline int elf64_get_relocation_type(relocation_t rel)
{
	switch(rel.size)
	{
	case BITSIZE8:
		if(rel.pc_relative)
			return R_X86_64_PC8;
		else
			return R_X86_64_8;
	case BITSIZE16:
		if(rel.pc_relative)
			return R_X86_64_PC16;
		else
			return R_X86_64_16;
	case BITSIZE32:
		if(rel.pc_relative)
			return R_X86_64_PC32;
		else
			return R_X86_64_32;
	case BITSIZE64:
		if(rel.pc_relative)
			return R_X86_64_PC64;
		else
			return R_X86_64_64;
	default:
		return -1;
	}
}

#ifndef elf_get_relocation_type
# define elf_get_relocation_type(rel) (output.format != FORMAT_ELF64 ? elf32_get_relocation_type(rel) : elf64_get_relocation_type(rel))
#endif

#ifndef nop_byte
# define nop_byte(__ins, __index) 0x90
#endif

#ifndef coff_magic_number
# define coff_magic_number() I386MAGIC
#endif

#ifndef coff_header_flags
# define coff_header_flags() (output.format != FORMAT_WIN64 ? 0x0104 : 0x0004)
#endif

#ifndef coff_relocation_size
# define coff_relocation_size() 10
#endif

static inline int coff32_get_relocation_type(relocation_t rel)
{
	switch(rel.size)
	{
	case BITSIZE16:
		if(rel.var.segment_of)
			return R_SEG12;
		else if(rel.pc_relative)
			return R_REL16;
		else
			return R_DIR16;
	case BITSIZE32:
		if(rel.pc_relative)
			return R_REL32;
		else
			return R_DIR32;
	default:
		return -1;
	}
}

static inline int coff64_get_relocation_type(relocation_t rel)
{
	switch(rel.size)
	{
	case BITSIZE32:
		if(rel.pc_relative)
			return IMAGE_REL_AMD64_REL32;
		else
			return IMAGE_REL_AMD64_ADDR32;
	case BITSIZE64:
		return IMAGE_REL_AMD64_ADDR64;
	default:
		return -1;
	}
}

#ifndef coff_get_relocation_type
# define coff_get_relocation_type(rel) (output.format != FORMAT_WIN64 ? coff32_get_relocation_type(rel) : coff64_get_relocation_type(rel))
#endif

#endif // _X86_ISA_H
