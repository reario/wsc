.SUFFIXES: .c.o

CC = gcc 

INCDIR = /home/reario/include
LIBDIR = /home/reario/lib

#-DDAEMON

all : server

server :  json.o  bit.o attiva.o server.o spie_bobine.o energy.o
	$(CC) -Wall -I${INCDIR} -I${INCDIR}/modbus -I${INCDIR}/liboath -L${LIBDIR} -lmodbus -lwebsockets -loath $^ -o $@

.c.o : json.h server.h
<<<<<<< HEAD

=======
	$(CC) -c -g -Wall -Werror=maybe-uninitialized -DCHECK  -I$(INCDIR) -I${INCDIR}/modbus -I${INCDIR}/liboath $< -o $@
>>>>>>> 1aa00c0cbdf6e0ce0d5672bf2899c6391d4f348e

clean :
	rm -f *~ *.o *.i *.s *.core server

#tmb : tmb.o
#	$(CC) -Wall -I${INCDIR} -L${LIBDIR} -lmodbus $^ -o $@
