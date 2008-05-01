/* sparc64 assembly.
 * Don't include directly, use asm.h instead.
 */

/* From Linux: 
   <<snip>>*/
#define membar_safe(type) \
do {	__asm__ __volatile__("ba,pt	%%xcc, 1f\n\t" \
			     " membar	" type "\n" \
			     "1:\n" \
			     : : : "memory"); \
} while (0)

#define mb()	\
	membar_safe("#LoadLoad | #LoadStore | #StoreStore | #StoreLoad")
/*  <<snap>> */

static inline void barrier(void)
{
	mb();
}
