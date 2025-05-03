all: simplefs run

simplefs:
	gcc -c fs.c
	gcc -c main.c
	gcc -o simplefs main.o fs.o

run:
	./simplefs

clean:
	rm -f *.o simplefs disk.sim