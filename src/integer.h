#ifndef _INTEGER_H
#define _INTEGER_H

#include <stdbool.h>
#include <stdio.h>

#if USE_GMP
# include <gmp.h>
typedef mpz_t integer_t;
typedef mpz_t uinteger_t;
typedef mpz_t integer_result_t;
typedef mpz_t * integer_value_t;
# define INTREF(__x) (__x)
# define INTVAL(__x) (*(__x))
#else
typedef int64_t integer_t;
typedef uint64_t uinteger_t;
typedef int64_t * integer_result_t;
typedef int64_t integer_value_t;
# define INTREF(__x) (&(__x))
# define INTVAL(__x) (__x)
#endif

static inline bool int_is_zero(integer_t i)
{
#if USE_GMP
	return mpz_sgn(i) == 0;
#else
	return i == 0;
#endif
}

static inline int int_sgn(integer_t i)
{
#if USE_GMP
	return mpz_sgn(i);
#else
	return i < 0 ? -1 : i == 0 ? 0 : 1;
#endif
}

static inline int int_cmp_si(integer_t i, signed long int v)
{
#if USE_GMP
	return mpz_cmp_si(i, v);
#else
	return i < v ? -1 : i == v ? 0 : 1;
#endif
}

static inline int int_cmp_ui(integer_t i, unsigned long int v)
{
#if USE_GMP
	return mpz_cmp_ui(i, v);
#else
	return i < 0 || i < v ? -1 : i == v ? 0 : 1;
#endif
}

static inline int uint_sgn(uinteger_t i)
{
#if USE_GMP
	return mpz_sgn(i);
#else
	return i == 0 ? 0 : 1;
#endif
}

static inline int uint_cmp_ui(uinteger_t i, unsigned long int v)
{
#if USE_GMP
	return mpz_cmp_ui(i, v);
#else
	return i < v ? -1 : i == v ? 0 : 1;
#endif
}

static inline int uint_cmp(uinteger_t i, uinteger_t j)
{
#if USE_GMP
	return mpz_cmp(i, j);
#else
	return i < j ? -1 : i == j ? 0 : 1;
#endif
}

static inline void int_init(integer_t * d)
{
#if USE_GMP
	mpz_init(*d);
#else
	*d = 0;
#endif
}

#define int_init(d) int_init(&(d))

static inline void int_init_set(integer_t * d, integer_t s)
{
#if USE_GMP
	mpz_init_set(*d, s);
#else
	*d = s;
#endif
}

#define int_init_set(d, s) int_init_set(&(d), s)

static inline void int_init_set_ui(integer_t * d, unsigned long int s)
{
#if USE_GMP
	mpz_init_set_ui(*d, s);
#else
	*d = s;
#endif
}

#define int_init_set_ui(d, s) int_init_set_ui(&(d), s)

static inline void int_set(integer_t * d, integer_t s)
{
#if USE_GMP
	mpz_set(*d, s);
#else
	*d = s;
#endif
}

#define int_set(d, s) int_set(&(d), s)

static inline void int_set_ui(integer_t * d, unsigned long int v)
{
#if USE_GMP
	mpz_set_ui(*d, v);
#else
	*d = v;
#endif
}

#define int_set_ui(d, s) int_set_ui(&(d), s)

static inline void int_clear(integer_t d)
{
#if USE_GMP
	mpz_clear(d);
#endif
}

static inline void int_delete(integer_value_t d)
{
	int_clear(INTVAL(d));
#if USE_GMP
	free(d);
#endif
}

static inline bool uint_fits(uinteger_t v)
{
#if USE_GMP
	return mpz_fits_ulong_p(v);
#else
	return true;
#endif
}

static inline signed long int int_get(integer_t v)
{
#if USE_GMP
	return mpz_get_si(v);
#else
	return v;
#endif
}

static inline unsigned long int uint_get(uinteger_t v)
{
#if USE_GMP
	return mpz_get_si(v);
#else
	return v;
#endif
}

static inline void uint_add_ui(integer_t * v, unsigned long int i)
{
#if USE_GMP
	mpz_add_ui(*v, *v, i);
#else
	*v += i;
#endif
}

#define uint_add_ui(v, i) uint_add_ui(&(v), i)

static inline void uint_sub_ui(integer_t * v, unsigned long int i)
{
#if USE_GMP
	mpz_sub_ui(*v, *v, i);
#else
	*v -= i;
#endif
}

#define uint_sub_ui(v, i) uint_sub_ui(&(v), i)

static inline size_t uint_print_hex(FILE * output, uinteger_t v)
{
#if USE_GMP
	return mpz_out_str(output, 16, v);
#else
	return fprintf(output, "%lX", v);
#endif
}

static inline void uint_parse(integer_value_t * j, const char * text, int base)
{
#if USE_GMP
	*j = malloc(sizeof(mpz_t));
	mpz_init_set_str(**j, text, base);
#else
	*j = strtol(text, NULL, base);
#endif
}

#define uint_parse(j, text, base) uint_parse(&(j), text, base)

static inline void int_and_ui(integer_t * v, unsigned long i)
{
#if USE_GMP
	mpz_t u;
	mpz_init_set_ui(u, i);
	mpz_and(*v, *v, u);
	mpz_clear(u);
#else
	*v &= i;
#endif
}
#define int_and_ui(v, i) int_and_ui(&(v), i)

static inline void uint_shr(uinteger_t * v, unsigned long i)
{
#if USE_GMP
	mpz_fdiv_q_2exp(*v, *v, i);
#else
	*v >>= i;
#endif
}

#define uint_shr(v, i) uint_shr(&(v), i)

static inline void int_shl(integer_t * v, signed long i)
{
#if USE_GMP
	mpz_mul_2exp(*v, *v, i);
#else
	*v <<= i;
#endif
}

#define int_shl(v, i) int_shl(&(v), i)

#endif // _INTEGER_H
