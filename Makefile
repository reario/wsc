.SUFFIXES: .c.o

CC = gcc

objs = server.o

INCDIR = /home/reario/include
LIBDIR = /home/reario/lib


all : server

server :  server.o
	$(CC) -Wall -I${INCDIR} -I${INCDIR}/modbus -I${INCDIR}/liboath -L${LIBDIR} -lmodbus -lwebsockets -loath $^ -o $@

.c.o :
	$(CC) -c -g -Wall -I$(INCDIR) -I${INCDIR}/modbus -I${INCDIR}/liboath $< -o $@

clean :
	rm -f *~ *.o *.i *.s *.core server
