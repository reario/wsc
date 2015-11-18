.SUFFIXES: .c.o

CC = gcc

objs = json.o server.o

INCDIR = /home/reario/include
LIBDIR = /home/reario/lib


all : server

server :  json.o server.o
	$(CC) -Wall -I${INCDIR} -I${INCDIR}/modbus -I${INCDIR}/liboath -L${LIBDIR} -lmodbus -lwebsockets -loath $^ -o $@

conf : conf.o
	$(CC) -Wall -I${INCDIR} -L${LIBDIR} -lconfig $^ -o $@

.c.o : json.h server.h
	$(CC) -c -g -Wall -I$(INCDIR) -I${INCDIR}/modbus -I${INCDIR}/liboath $< -o $@

clean :
	rm -f *~ *.o *.i *.s *.core server
