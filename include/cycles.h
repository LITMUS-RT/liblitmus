#ifndef CYCLES_H
#define CYCLES_H

#ifdef __x86_64__

#define rdtscll(val) do { \
	unsigned int __a,__d; \
	__asm__ __volatile__("rdtsc" : "=a" (__a), "=d" (__d)); \
	(val) = ((unsigned long)__a) | (((unsigned long)__d)<<32); \
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
#elif defined __i386__
static inline unsigned long long native_read_tsc(void) {
	unsigned long long val;
	__asm__ __volatile__("rdtsc" : "=A" (val));
	return val;
}

typedef unsigned long long cycles_t;

#define CYCLES_FMT "llu"

static inline cycles_t get_cycles(void)
{
        return native_read_tsc();
}
#elif defined __sparc__

#define NPT_BIT 63

typedef unsigned long cycles_t;

#define CYCLES_FMT "lu"

static inline cycles_t get_cycles(void) {
	unsigned long cycles = 0;
	__asm__ __volatile__("rd %%asr24, %0" : "=r" (cycles));
	return cycles & ~(1UL << NPT_BIT);
}

#endif

#endif
