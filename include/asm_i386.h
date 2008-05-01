/* Intel ia32 assembly. 
 * Don't include directly, use asm.h instead.
 */

static inline void barrier(void)
{
	__asm__ __volatile__("sfence": : :"memory");
}
