/* liblitmus platform dependent includes */

#ifndef ASM_H
#define ASM_H

#if defined(__i386__) || defined(__x86_64__)
#include "asm_x86.h"
#endif


#ifdef __sparc__
#include "asm_sparc.h"
#endif

#endif
