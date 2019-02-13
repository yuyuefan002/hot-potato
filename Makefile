CC=gcc
#CFLAGS=-03
CFLAGS=-Werror -Wall -pedantic -ggdb3 -std=gnu99
all:ringmaster player
ringmaster:ringmaster.o server.o base.o
	$(CC) -o $@ ringmaster.o server.o base.o

ringmaster.o:ringmaster.c
	$(CC) $(CFLAGS) -c ringmaster.c
server.o:server.c
	$(CC) $(CFLAGS) -c server.c

player:player.o client.o base.o
	$(CC) -o $@ player.o client.o base.o
player.o:player.c
	$(CC) $(CFLAGS) -c player.c
client.o:client.c
	$(CC) $(CFLAGS) -c client.c
base.o:base.c
	$(CC) $(CFLAGS) -c base.c
clean:
	rm -f *~ *.o ringmaster player
clobber:
	rm -f *~ *.o
