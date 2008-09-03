KERNEL_DIR = ../litmus2008

INC=-Iinclude/ -I${KERNEL_DIR}/include/

ARCH=$(shell uname -m | sed -e s/i.86/i386/)

ifeq ($(ARCH),sparc64)
  CPU=-mcpu=v9 -m64
else
  CPU=
endif


CFLAGS=-Wall -Wdeclaration-after-statement ${INC} ${CPU} -g  -D_XOPEN_SOURCE=600 -D_GNU_SOURCE
CPPFLAGS=-Wall -g

LIBS= ./liblitmus.a

LIB_OBJ=  litmus.o syscalls.o sched_trace.o task.o kernel_iface.o

TARGETS = run rt_launch liblitmus.a \
          wait_test np_test stdump mode_test base_task base_mt_task release_ts cycles

vpath %.h include/
vpath %.c src/ bin/

all: ${TARGETS}
clean:
	rm -f *.o *~  ${TARGETS}

base_mt_task: base_mt_task.o liblitmus.a
	${CC} ${CFLAGS} -static -pthread -o base_mt_task base_mt_task.o  ${LIBS}

base_task: base_task.o liblitmus.a
	${CC} ${CFLAGS} -static -o base_task base_task.o  ${LIBS}

wait_test: wait_test.o litmus.h liblitmus.a
	${CC} ${CFLAGS} -static -o wait_test wait_test.o  ${LIBS}

mode_test: mode_test.o litmus.h liblitmus.a
	${CC} ${CFLAGS} -static -o mode_test mode_test.o  ${LIBS}

np_test: np_test.o litmus.h liblitmus.a
	${CC} ${CFLAGS} -static -o np_test np_test.o  ${LIBS}

run: run.o ${LIBS}
	${CC} ${CFLAGS} -o run run.o ${LIBS}

rt_launch: liblitmus.a litmus.h rt_launch.o common.o
	${CC} ${CFLAGS} -static -o rt_launch  rt_launch.o common.o  ${LIBS}

release_ts: liblitmus.a litmus.h release_ts.o
	${CC} ${CFLAGS} -static -o release_ts release_ts.o ${LIBS}

stdump: liblitmus.a litmus.h sched_trace.h stdump.o
	${CC} ${CFLAGS} -o stdump  stdump.o ${LIBS}

cycles: cycles.o
	${CC} ${CFLAGS} -o cycles cycles.o

liblitmus.a:  ${LIB_OBJ} litmus.h
	${AR} rcs liblitmus.a ${LIB_OBJ}

check:
	sparse ${CFLAGS} src/*.c bin/*.c
