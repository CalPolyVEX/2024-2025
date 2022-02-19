CC = gcc -g

buildmessage: buildmessage.o
	$(CC) -o buildmessage buildmessage.o 

buildmessage.o: buildmessage.c 
	$(CC) -c buildmessage.c

clean:
	rm -rf buildmessage.o
