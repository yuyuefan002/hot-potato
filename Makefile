CC=gcc
#CFLAGS=-03
CFLAGS=-Werror -Wall -pedantic -ggdb3 -std=gnu99
all:ringmaster player
ringmaster:ringmaster.o server.o
	$(CC) -o $@ ringmaster.o server.o

ringmaster.o:ringmaster.c
	$(CC) $(CFLAGS) -c ringmaster.c
server.o:server.c
	$(CC) $(CFLAGS) -c server.c
player:player.c
	$(CC) $(CFLAGS) -o $@ player.c

clean:
	rm -f *~ *.o ringmaster player
clobber:
	rm -f *~ *.o
