#ifndef ASM_CYCLES_H
#define ASM_CYCLES_H

#include <stdint.h>
#include <inttypes.h>

typedef uint64_t cycles_t;

#define CYCLES_FMT PRIu64

static inline cycles_t get_cycles(void)
{
	cycles_t c;
	asm volatile(
		"isb\n"
		"mrs %0, cntvct_el0" : "=r" (c));
	return c;
}

#endif
