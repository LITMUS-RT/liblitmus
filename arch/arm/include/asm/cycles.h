#ifndef ASM_CYCLES_H
#define ASM_CYCLES_H

typedef unsigned long cycles_t;

#define CYCLES_FMT "lu"

/* system call wrapper */
int null_call(cycles_t *timestamp);

static inline cycles_t get_cycles(void)
{
	cycles_t c;
	/* On the ARM11 MPCore chips, userspace cannot access the cycle counter
	 * directly. So ask the kernel to read it instead. Eventually, this
	 * should support newer ARM chips that do allow accessing the cycle
	 * counter in userspace.
	 */
	null_call(&c);
	return c;
}

#endif
