CFLAGS=-g -Wall -std=c99 
CC=mpicc

gameoflife: main.o array.o gol.o options.o partition.o rundata.o
	$(CC) -o $@ $^ -lm

clean:
	rm -f *.o gameoflife gol_*txt

cleandata:
	rm -f gol_*txt
