#ifndef EDF_HSB_H
#define EDF_HSB_H


int set_hrt(int cpu, unsigned int  wcet, unsigned int  period);
int get_hrt(int cpu, unsigned int *wcet, unsigned int *period);
int create_be(unsigned int wcet, unsigned int period);


#endif
