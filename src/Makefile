
RM=rm -f

COMILE_OPT=
MACROS=

CC=g++ -g -Wall  $(MACROS) $(COMILE_OPT)

prog:=bin_test
inc_dir=-I../include
lib_dir=
libs=../libs/libcrypto.a -ldl -lrt -pthread

srcs=$(wildcard *.cpp)
	
objs=$(srcs:.cpp=.o)

default:$(prog)
.PHONY:default

$(prog):$(objs)
	$(CC) -o $@ $^ $(lib_dir) $(libs)

$(objs):%.o:%.cpp
	$(CC) -c -o $@ $< $(inc_dir)	

clean:
	-@$(RM)  $(objs) $(prog)
.PHONY:clean


