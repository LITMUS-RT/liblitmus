KERNEL_DIR = ../litmus

INC=-Iinclude/ -I${KERNEL_DIR}/include/

CFLAGS=-Wall -Wdeclaration-after-statement ${INC} -g  -D_XOPEN_SOURCE=600
CPPFLAGS=-Wall -g

LIBS= ./liblitmus.a

LIB_OBJ=  litmus.o syscalls.o sched_trace.o task.o kernel_iface.o

TARGETS = run rt_launch liblitmus.a \
          wait_test np_test stdump mode_test base_task base_mt_task

vpath %.h include/
vpath %.c src/ bin/

all: ${TARGETS}
clean:
	rm -f *.o *~  ${TARGETS}

base_mt_task: base_mt_task.o liblitmus.a
	cc -static -pthread -o base_mt_task base_mt_task.o  ${LIBS}

base_task: base_task.o liblitmus.a
	cc -static -o base_task base_task.o  ${LIBS}

wait_test: wait_test.o litmus.h liblitmus.a
	cc -static -o wait_test wait_test.o  ${LIBS}

mode_test: mode_test.o litmus.h liblitmus.a
	cc -static -o mode_test mode_test.o  ${LIBS}

np_test: np_test.o litmus.h liblitmus.a
	cc -static -o np_test np_test.o  ${LIBS}

run: run.o ${LIBS}
	cc -o run run.o ${LIBS}

rt_launch: liblitmus.a litmus.h rt_launch.o
	cc -static -o rt_launch  rt_launch.o ${LIBS}

stdump: liblitmus.a litmus.h sched_trace.h stdump.o
	cc -o stdump  stdump.o ${LIBS}

liblitmus.a:  ${LIB_OBJ} litmus.h 
	${AR} rcs liblitmus.a ${LIB_OBJ}

check:
	sparse ${CFLAGS} src/*.c bin/*.c