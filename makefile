all: clean simplefs run

simplefs: fs.c main.c
	gcc -c fs.c
	gcc -c main.c
	gcc -o simplefs main.o fs.o

run: simplefs
	./simplefs

clean:
	rm -f *.o simplefs
