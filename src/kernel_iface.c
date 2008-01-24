#include <stdio.h>

#include "litmus.h"
#include "internal.h"

/* per real-time thread kernel <-> user space flags */


struct np_flag {
	#define RT_PREEMPTIVE 		0x2050 /* = NP */
	#define RT_NON_PREEMPTIVE 	0x4e50 /* =  P */
	unsigned short preemptivity;

	#define RT_EXIT_NP_REQUESTED	0x5251 /* = RQ */
	unsigned short request;

	unsigned int ctr;
};

int register_np_flag(struct np_flag* flag);
int signal_exit_np(void);



static __thread struct np_flag np_flag;


int init_kernel_iface(void)
{
	int ret;
	np_flag.preemptivity = RT_PREEMPTIVE;
	np_flag.ctr = 0;
	ret = register_np_flag(&np_flag);
	check("register_np_flag()");  
	return ret;
}

static inline void barrier(void)
{
	__asm__ __volatile__("sfence": : :"memory");
}

void enter_np(void)
{
	if (++np_flag.ctr == 1)
	{
		np_flag.request = 0;
		barrier();
		np_flag.preemptivity = RT_NON_PREEMPTIVE;
	}
}


void exit_np(void)
{
	if (--np_flag.ctr == 0)
	{
		np_flag.preemptivity = RT_PREEMPTIVE;
		barrier();
		if (np_flag.request == RT_EXIT_NP_REQUESTED)
			signal_exit_np();
	}
}

