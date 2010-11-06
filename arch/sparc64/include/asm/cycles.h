#ifndef ASM_CYCLES_H
#define ASM_CYCLES_H

#define NPT_BIT 63

typedef unsigned long cycles_t;

#define CYCLES_FMT "lu"

static inline cycles_t get_cycles(void) {
	cycles_t cycles = 0;
	__asm__ __volatile__("rd %%asr24, %0" : "=r" (cycles));
	return cycles & ~(1UL << NPT_BIT);
}

#endif
