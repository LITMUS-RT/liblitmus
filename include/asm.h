/* liblitmus platform dependent includes */

#ifndef ASM_H
#define ASM_H

#ifdef __i386__
#include "asm_i386.h"
#endif

#ifdef __sparc__
#include "asm_sparc.h"
#endif

#endif
