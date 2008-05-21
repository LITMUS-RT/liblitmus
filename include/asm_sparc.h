/* sparc64 assembly.
 * Don't include directly, use asm.h instead.
 *
 * Most of this code comes straight out of the Linux kernel.
 *
 * The terms of the GPL v2 apply.
 *
 */

#define membar_safe(type) \
do {	__asm__ __volatile__("ba,pt	%%xcc, 1f\n\t" \
			     " membar	" type "\n" \
			     "1:\n" \
			     : : : "memory"); \
} while (0)

#define mb()	\
	membar_safe("#LoadLoad | #LoadStore | #StoreStore | #StoreLoad")

static inline void barrier(void)
{
	mb();
}


#define cpu_relax() barrier()

static inline int
cmpxchg(volatile int *m, int old, int new)
{
	__asm__ __volatile__("membar #StoreLoad | #LoadLoad\n"
			     "cas [%2], %3, %0\n\t"
			     "membar #StoreLoad | #StoreStore"
			     : "=&r" (new)
			     : "0" (new), "r" (m), "r" (old)
			     : "memory");

	return new;
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


/**
 * atomic_add_return - add and return
 * @v: pointer of type atomic_t
 * @i: integer value to add
 *
 * Atomically adds @i to @v and returns @i + @v
 */
static __inline__ int atomic_add_return(int i, atomic_t *v)
{
	int old; 
	int ret;
	goto first;
	do {
		cpu_relax();
	first:
		old = atomic_read(v);
		ret = cmpxchg(&v->counter, old, old + i);		
	} while (ret != old);
	return old + i;
}
 
static __inline__ void atomic_add(int i, atomic_t *v)
{
	atomic_add_return(i, v);
}

#define atomic_inc_return(v)  (atomic_add_return(1,v))


