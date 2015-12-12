.SUFFIXES: .c.o

CC = gcc 

objs = json.o attiva.o server.o bit.o

INCDIR = /home/reario/include
LIBDIR = /home/reario/lib


all : server server-fraggle

server :  json.o  bit.o attiva.o server.o spie_bobine.o 
	$(CC) -Wall -I${INCDIR} -I${INCDIR}/modbus -I${INCDIR}/liboath -L${LIBDIR} -lmodbus -lwebsockets -loath $^ -o $@

server-fraggle	:	server-fraggle.o
	$(CC) -Wall -I${INCDIR} -L${LIBDIR} -lwebsockets $^ -o $@


.c.o : json.h server.h
	$(CC) -c -g -Wall -Werror=maybe-uninitialized -DCHECK -I$(INCDIR) -I${INCDIR}/modbus -I${INCDIR}/liboath $< -o $@

clean :
	rm -f *~ *.o *.i *.s *.core server

#tmb : tmb.o
#	$(CC) -Wall -I${INCDIR} -L${LIBDIR} -lmodbus $^ -o $@
