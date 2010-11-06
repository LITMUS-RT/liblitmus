#ifndef ASM_CYCLES_H
#define ASM_CYCLES_H

#define rdtscll(val) do { \
	unsigned int __a,__d; \
	__asm__ __volatile__("rdtsc" : "=a" (__a), "=d" (__d)); \
	(val) = ((unsigned long long)__a) | (((unsigned long long)__d)<<32); \
} while(0)

static __inline__ unsigned long long native_read_tsc(void)
{
	unsigned long long val;

	__asm__ __volatile__("mfence":::"memory");
	rdtscll(val);
	__asm__ __volatile__("mfence":::"memory");

	return val;
}

#define CYCLES_FMT "llu"

typedef unsigned long long cycles_t;

static inline cycles_t get_cycles(void)
{
	return native_read_tsc();
}

#endif
