default: all

CFLAGS := -I./include -g --std=gnu99
CXXFLAGS := -I./include -g

CC := gcc
CXX := g++

BINARIES := hubo-read-trajectory hubo-read-func
all : $(BINARIES)

LIBS :=  -lrt -lm -lc -lach

hubo-read-trajectory: src/hubo-read-trajectory.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

hubo-read-func: src/hubo-read-trajectory-as-func.c
	$(CC) $(CFLAGS) -c -o  $@.o $< $(LIBS) 
	ar rcs libhubo-func.a $@.o
	gcc -c -fPIC src/hubo-read-trajectory-as-func.c -o hubo-read-func.o
INSTALL = /usr/local/bin 

clean:
	rm -f $(BINARIES) src/*.o

.PHONY: install
install: all
	 cp ./include/hubo-read-trajectory-as-func.h /usr/local/lib/hubo-read-trajectory-as-func.h
	 cp ./include/hubo-ref-filter.h /usr/local/include/hubo-ref-filter.h
	 cp ./hubo-read-func /usr/local/bin/hubo-read-func
