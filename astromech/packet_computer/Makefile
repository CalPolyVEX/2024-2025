CC = gcc

buildmessage: buildmessage.o
	$(CC) -o buildmessage buildmessage.o PacketCRC.o

buildmessage.o: buildmessage.c PacketCRC.o
	$(CC) -c buildmessage.c

PacketCRC.o: PacketCRC.c PacketCRC.h
	$(CC) -c PacketCRC.c
	
clean:
	rm -rf PacketCRC.o
	rm -rf buildmessage.o
