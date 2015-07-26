.SUFFIXES: .c.o

CC = gcc

objs = server.o

INCDIR = /home/reario/include
LIBDIR = /home/reario/lib


all : server

server :  server.o
	$(CC) -Wall -I${INCDIR} -I${INCDIR}/modbus -L${LIBDIR} -lmodbus -lwebsockets $^ -o $@

.c.o :
	$(CC) -c -g -Wall -I$(INCDIR) -I${INCDIR}/modbus $< -o $@

clean :
	rm -f *~ *.o *.i *.s *.core server
