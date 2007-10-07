CFLAGS=-Wall -g -Iinclude/ -D_XOPEN_SOURCE=600
CPPFLAGS=-Wall -g

LIBS= ./liblitmus.a

TARGETS = showsched iotest set_rt_mode  run timeout rt_launch edfhsb liblitmus.a wait_test np_test

vpath %.h include/
vpath %.c src/

all: ${TARGETS}
clean:
	rm -f *.o *~  ${TARGETS}

wait_test: wait_test.o litmus.h liblitmus.a
	cc -static -o wait_test wait_test.o  ${LIBS}

np_test: np_test.o litmus.h liblitmus.a
	cc -static -o np_test np_test.o  ${LIBS}

iotest: iotest.o litmus.h liblitmus.a
	cc -static -o iotest iotest.o  ${LIBS}

run: run.o
	cc -o run run.o

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

liblitmus.a: litmus.o adaptive.o adaptive.h litmus.h edf-hsb.o edf-hsb.h
	${AR} rcs liblitmus.a litmus.o adaptive.o edf-hsb.o 
