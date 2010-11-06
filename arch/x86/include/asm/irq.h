#ifndef ASM_IRQ_H
#define ASM_IRQ_H

/* please, use these only if you _really_ know what you're doing
 * ... and remember iopl(3) first!! (include sys/io.h)
 */
static inline void cli(void) {
	asm volatile("cli": : :"memory");
}

static inline void sti(void) {
	asm volatile("sti": : :"memory");
}

#endif
