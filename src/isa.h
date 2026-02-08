#if TARGET_DUMMY
# include "dummy/isa.h"
#elif TARGET_X86
# include "x86/isa.h"
#elif TARGET_X80
# include "x80/isa.h"
#elif TARGET_I4
# include "i4/isa.h"
#elif TARGET_X65
# include "x65/isa.h"
#elif TARGET_680X
# include "680x/isa.h"
#elif TARGET_68K
# include "68k/isa.h"
#else
# error Unknown target
#endif
