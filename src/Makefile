
RM=rm -f

VERSION=$(shell date "+%F_%T")
COMILE_OPT=
MACROS=-D__BUILD_VER__="\"V1.0 $(VERSION)\"" -D__LOG_LEVEL__=1 

ifndef debug
CC=g++ -s -O3 -W -Wall $(MACROS) $(COMILE_OPT)
libs=-lcrypto -ldl -lrt -pthread
else
CC=g++ -g -O0 -W -Wall  $(MACROS) $(COMILE_OPT) 
libs=-lcrypto -ldl -lrt -pthread



endif

prog:=bin_test
libname:=libagent.a

inc_dir=-I. -I../include
lib_dir=-L../libs

lib_srcs=$(wildcard *.cpp) 
main_srcs=$(wildcard main/*.cpp)
	
lib_objs=$(lib_srcs:.cpp=.o)
main_objs=$(main_srcs:.cpp=.o)
objs=$(main_objs) $(lib_objs) 

default:$(prog)
.PHONY:default

$(prog):$(main_objs) $(libname)
	$(CC) -o $@ $^ $(lib_dir) $(libs)

$(libname):$(lib_objs) 
	ar rcs $@ $^ 

$(lib_objs):%.o:%.cpp
	$(CC) -c -o $@ $< $(inc_dir)	
	
$(main_objs):%.o:%.cpp
	$(CC) -c -o $@ $< $(inc_dir)

clean:
	-@$(RM)  $(objs) $(prog) $(libname)
.PHONY:clean


