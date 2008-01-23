CFLAGS=-Wall -Wdeclaration-after-statement  -g -Iinclude/ -D_XOPEN_SOURCE=600
CPPFLAGS=-Wall -g

LIBS= ./liblitmus.a

LIB_OBJ=  litmus.o syscalls.o sched_trace.o adaptive.o edf-hsb.o task.o kernel_iface.o

TARGETS = showsched iotest set_rt_mode  run timeout rt_launch edfhsb liblitmus.a \
          wait_test np_test stdump mode_test base_task

vpath %.h include/
vpath %.c src/ bin/

all: ${TARGETS}
clean:
	rm -f *.o *~  ${TARGETS}

base_task: base_task.o liblitmus.a
	cc -static -o base_task base_task.o  ${LIBS}

wait_test: wait_test.o litmus.h liblitmus.a
	cc -static -o wait_test wait_test.o  ${LIBS}

mode_test: mode_test.o litmus.h liblitmus.a
	cc -static -o mode_test mode_test.o  ${LIBS}

np_test: np_test.o litmus.h liblitmus.a
	cc -static -o np_test np_test.o  ${LIBS}

iotest: iotest.o litmus.h liblitmus.a
	cc -static -o iotest iotest.o  ${LIBS}

run: run.o
	cc -o run run.o ${LIBS}

set_rt_mode: liblitmus.a set_rt_mode.o
	cc -o set_rt_mode set_rt_mode.o  ${LIBS}

showsched: show_scheduler.o liblitmus.a litmus.h
	cc -o showsched  show_scheduler.o  ${LIBS}

timeout: liblitmus.a timeout.o litmus.h
	cc -static -o timeout timeout.o  ${LIBS}

rt_launch: liblitmus.a litmus.h rt_launch.o
	cc -static -o rt_launch  rt_launch.o ${LIBS}

edfhsb: liblitmus.a edf-hsb.o litmus.h edf-hsb.h hrt.o
	cc -o edfhsb hrt.o edf-hsb.o  ${LIBS}

stdump: liblitmus.a litmus.h sched_trace.h stdump.o
	cc -o stdump  stdump.o ${LIBS}

liblitmus.a:  ${LIB_OBJ} adaptive.h litmus.h edf-hsb.h
	${AR} rcs liblitmus.a ${LIB_OBJ}

