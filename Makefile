CFLAGS=-Wall -g
CPPFLAGS=-Wall -g


all: showsched iotest set_rt_mode  run timeout rt_launch edfhsb liblitmus.a wait_test

clean:
	rm *.o showsched iotest set_rt_mode run timeout rt_launch edfhsb liblitmus.a wait_test

wait_test: wait_test.o litmus.h litmus.o
	cc -static -o wait_test litmus.o wait_test.o

iotest: iotest.o litmus.h litmus.o
	cc -static -o iotest litmus.o iotest.o

run: run.o
	cc -o run run.o

set_rt_mode: litmus.o set_rt_mode.o
	cc -o set_rt_mode litmus.o set_rt_mode.o

showsched: show_scheduler.o litmus.o litmus.h
	cc -o showsched show_scheduler.o litmus.o

timeout: litmus.o timeout.o litmus.h
	cc -static -o timeout litmus.o timeout.o

rt_launch: litmus.o litmus.h rt_launch.o
	cc -static -o rt_launch litmus.o rt_launch.o

edfhsb: litmus.o edf-hsb.o litmus.h edf-hsb.h hrt.o
	cc -o edfhsb hrt.o litmus.o edf-hsb.o

liblitmus.a: litmus.o litmus.h edf-hsb.o edf-hsb.h
	${AR} rcs liblitmus.a litmus.o edf-hsb.o
