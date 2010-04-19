/* Intel ia32 assembly.
 * Don't include directly, use asm.h instead.
 *
 * Most of this code comes straight out of the Linux kernel.
 *
 * The terms of the GPL v2 apply.
 */

static inline void barrier(void)
{
	__asm__ __volatile__("mfence": : :"memory");
}

static __inline__ void cpu_relax(void)
{
	__asm__ __volatile("pause");
}

/* please, use these only if you _really_ know what you're doing
 * ... and remember iopl(3) first!! (include sys/io.h)
 */
static inline void cli(void) {
	asm volatile("cli": : :"memory");
}

static inline void sti(void) {
	asm volatile("sti": : :"memory");
}

typedef struct { int counter; } atomic_t;

#ifdef __i386__

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

#elif defined(__x86_64__)

/* almost the same as i386, but extra care must be taken when
 * specifying clobbered registers
 */

#define ATOMIC_INIT(i)	{ (i) }

/**
 * atomic_read - read atomic variable
 * @v: pointer of type atomic_t
 *
 * Atomically reads the value of @v.
 */
static inline int atomic_read(const atomic_t *v)
{
	return v->counter;
}

/**
 * atomic_set - set atomic variable
 * @v: pointer of type atomic_t
 * @i: required value
 *
 * Atomically sets the value of @v to @i.
 */
static inline void atomic_set(atomic_t *v, int i)
{
	v->counter = i;
}

/**
 * atomic_add - add integer to atomic variable
 * @i: integer value to add
 * @v: pointer of type atomic_t
 *
 * Atomically adds @i to @v.
 */
static inline void atomic_add(int i, atomic_t *v)
{
	asm volatile("lock; addl %1,%0"
		     : "=m" (v->counter)
		     : "ir" (i), "m" (v->counter));
}

/**
 * atomic_add_return - add and return
 * @i: integer value to add
 * @v: pointer of type atomic_t
 *
 * Atomically adds @i to @v and returns @i + @v
 */
static inline int atomic_add_return(int i, atomic_t *v)
{
	int __i = i;
	asm volatile("lock; xaddl %0, %1"
		     : "+r" (i), "+m" (v->counter)
		     : : "memory");
	return i + __i;
}

#define atomic_inc_return(v)  (atomic_add_return(1, v))

#endif
