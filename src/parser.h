#if TARGET_DUMMY
# include "../obj/dummy/dummy/parser.tab.h"
#elif TARGET_X86
# include "../obj/x86/x86/parser.tab.h"
#elif TARGET_X80
# include "../obj/x80/x80/parser.tab.h"
#elif TARGET_I4
# include "../obj/i4/i4/parser.tab.h"
#elif TARGET_X65
# include "../obj/x65/x65/parser.tab.h"
#elif TARGET_680X
# include "../obj/680x/680x/parser.tab.h"
#elif TARGET_68K
# include "../obj/68k/68k/parser.tab.h"
#else
# error Unknown target
#endif
