CC = gcc -Wall -pedantic

buildmessage: buildmessage.o
	$(CC) -o buildmessage buildmessage.o 

buildmessage.o: buildmessage.c buildmessage.h
	$(CC) -c buildmessage.c

clean:
	rm -rf buildmessage.o
