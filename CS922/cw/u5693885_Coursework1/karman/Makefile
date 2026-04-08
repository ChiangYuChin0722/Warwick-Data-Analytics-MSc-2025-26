CC=gcc
CFLAGS=-O3 -Wall -g -fopenmp

.c.o:
	$(CC) -c $(CFLAGS) $<

all: karman bin2ppm diffbin diffbin_ext colcopy pingpong # karman-par

clean:
	rm -f bin2ppm diffbin diffbin_ext pingpong colcopy karman karman-par *.o

karman: alloc.o boundary.o init.o karman.o simulation.o
	$(CC) $(CFLAGS) -o $@ $^ -lm

karman-par: alloc.o boundary.o init.o karman-par.o simulation-par.o
	$(CC) $(CFLAGS) -o $@ $^ -lm

bin2ppm: bin2ppm.o alloc.o
	$(CC) $(CFLAGS) -o $@ $^ -lm

diffbin: diffbin.c
	$(CC) $(CFLAGS) -o $@ $^ -lm

diffbin_ext: diffbin_ext.c
	$(CC) $(CFLAGS) -o $@ $^ -lm

pingpong: pingpong.o
	$(CC) $(CFLAGS) -o $@ $^

colcopy: colcopy.o alloc.o
	$(CC) $(CFLAGS) -o $@ $^

bin2ppm.o        : alloc.h datadef.h
boundary.o       : datadef.h
colcopy.o        : alloc.h
init.o           : datadef.h
karman.o         : alloc.h boundary.h datadef.h init.h simulation.h
karman-par.o     : alloc.h boundary.h datadef.h init.h simulation.h
simulation.o     : datadef.h init.h
simulation-par.o : datadef.h init.h
