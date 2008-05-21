/* Intel ia32 assembly. 
 * Don't include directly, use asm.h instead.
 *
 * Most of this code comes straight out of the Linux kernel.
 *
 * The terms of the GPL v2 apply.
 */

static inline void barrier(void)
{
	__asm__ __volatile__("sfence": : :"memory");
}

static __inline__ void cpu_relax(void)
{
	__asm__ __volatile("pause");
}

typedef struct { int counter; } atomic_t;

#define ATOMIC_INIT(i)	{ (i) }

/**
 * atomic_read - read atomic variable
 * @v: pointer of type atomic_t
 * 
 * Atomically reads the value of @v.
 */ 
#define atomic_read(v)		((v)->counter)

/**
 * atomic_set - set atomic variable
 * @v: pointer of type atomic_t
 * @i: required value
 * 
 * Atomically sets the value of @v to @i.
 */ 
#define atomic_set(v,i)		(((v)->counter) = (i))

static __inline__ void atomic_add(int i, atomic_t *v)
{
	__asm__ __volatile__(
		"lock; addl %1,%0"
		:"+m" (v->counter)
		:"ir" (i));
}

/**
 * atomic_add_return - add and return
 * @v: pointer of type atomic_t
 * @i: integer value to add
 *
 * Atomically adds @i to @v and returns @i + @v
 */
static __inline__ int atomic_add_return(int i, atomic_t *v)
{
	int __i;
	__i = i;
	__asm__ __volatile__(
	        "lock; xaddl %0, %1"
		:"+r" (i), "+m" (v->counter)
		: : "memory");
	return i + __i;
}
 
#define atomic_inc_return(v)  (atomic_add_return(1,v))


