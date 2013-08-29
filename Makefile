default: all

CFLAGS := -I./include -g --std=gnu99
CXXFLAGS := -I./include -g

CC := gcc
CXX := g++

BINARIES := hubo-read-trajectory test
all : $(BINARIES)

LIBS :=  -lrt -lm -lc -lach

hubo-read-trajectory: src/hubo-read-trajectory.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

test: src/test.c
	$(CC) $(CFLAGS) -o $@ $< -include include/hubo-read-trajectory-as-func.h src/hubo-read-trajectory-as-func.c   $(LIBS) 

clean:
	rm -f $(BINARIES) src/*.o
