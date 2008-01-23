#ifndef SYSCALLS_H
#define SYSCALLS_H

/* this is missing in newer linux/unistd.h versions */

#define _syscall0(type,name) \
type name(void) \
{\
        return syscall(__NR_##name);\
}

#define _syscall1(type,name,type1,arg1) \
type name(type1 arg1) \
{\
        return syscall(__NR_##name, arg1);\
}


#define _syscall2(type,name,type1,arg1,type2,arg2) \
type name(type1 arg1,type2 arg2) \
{\
        return syscall(__NR_##name, arg1, arg2);\
}

#define _syscall3(type,name,type1,arg1,type2,arg2,type3,arg3)	\
type name(type1 arg1,type2 arg2, type3 arg3)   \
{\
        return syscall(__NR_##name, arg1, arg2, arg3);	\
}

#define _syscall4(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4)\
type name(type1 arg1,type2 arg2, type3 arg3, type4 arg4)	      	\
{\
        return syscall(__NR_##name, arg1, arg2, arg3, arg4);	\
}


#endif
