#include "litmus.h"
#include "edf-hsb.h"


typedef enum {
	EDF_HSB_SET_HRT,
	EDF_HSB_GET_HRT,
	EDF_HSB_CREATE_BE
} edf_hsb_setup_cmds_t;

typedef struct {
	int 		cpu;
	unsigned int	wcet;
	unsigned int 	period;
} setup_hrt_param_t;

typedef struct {
	unsigned int	wcet;
	unsigned int	period;
} create_be_param_t;

int set_hrt(int cpu, unsigned int  wcet, unsigned int  period)
{
	setup_hrt_param_t param;
	param.cpu    = cpu;
	param.wcet   = wcet;
	param.period = period;
	return scheduler_setup(EDF_HSB_SET_HRT, &param);
}

int get_hrt(int cpu, unsigned int *wcet, unsigned int *period)
{
	setup_hrt_param_t param;
	int ret;
	param.cpu    = cpu;
	ret = scheduler_setup(EDF_HSB_GET_HRT, &param);
	*wcet   = param.wcet;
	*period = param.period;
	return ret;
}

int create_be(unsigned int wcet, unsigned int period) 
{
	create_be_param_t param;
	param.wcet   = wcet;
	param.period = period;
	return scheduler_setup(EDF_HSB_CREATE_BE, &param);
}
