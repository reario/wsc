.SUFFIXES: .c.o

CC = gcc

objs = json.o attiva.o server.o bit.o

INCDIR = /home/reario/include
LIBDIR = /home/reario/lib


all : server

server :  json.o server.o bit.o attiva.o
	$(CC) -Wall -I${INCDIR} -I${INCDIR}/modbus -I${INCDIR}/liboath -L${LIBDIR} -lmodbus -lwebsockets -loath $^ -o $@




.c.o : json.h server.h
	$(CC) -c -g -Wall -I$(INCDIR) -I${INCDIR}/modbus -I${INCDIR}/liboath $< -o $@

clean :
	rm -f *~ *.o *.i *.s *.core server

#tmb : tmb.o
#	$(CC) -Wall -I${INCDIR} -L${LIBDIR} -lmodbus $^ -o $@
