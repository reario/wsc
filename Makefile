.SUFFIXES: .c.o

CC = gcc 

INCDIR = /home/reario/include
LIBDIR = /home/reario/lib


all : server

server :  json.o  bit.o attiva.o server.o spie_bobine.o energy.o
	$(CC) -Wall -I${INCDIR} -I${INCDIR}/modbus -I${INCDIR}/liboath -L${LIBDIR} -lmodbus -lwebsockets -loath $^ -o $@

.c.o : json.h server.h
	$(CC) -c -g -Wall -DCHECK -DDAEMON -I$(INCDIR) -I${INCDIR}/modbus -I${INCDIR}/liboath $< -o $@

clean :
	rm -f *~ *.o *.i *.s *.core server

#tmb : tmb.o
#	$(CC) -Wall -I${INCDIR} -L${LIBDIR} -lmodbus $^ -o $@
