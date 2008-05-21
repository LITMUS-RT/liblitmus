#ifndef CYCLES_H
#define CYCLES_H

#ifdef __i386__

typedef unsigned long long cycles_t;

#define CYCLES_FMT "llu"

static inline cycles_t get_cycles(void) 
{
        unsigned long long ret;
        __asm__ __volatile__("rdtsc" : "=A" (ret));
        return ret;
}

#endif


#ifdef __sparc__

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
