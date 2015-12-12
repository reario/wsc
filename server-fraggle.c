/*
 * libwebsockets-test-fraggle - random fragmentation test
 *
 * Copyright (C) 2010-2011 Andy Green <andy@warmcat.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation:
 *  version 2.1 of the License.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA  02110-1301  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <libwebsockets.h>

#define BUFFER 1024

static int client;
static int terminate;


int StringL;
char *json;	

/*==============================READ JSON FILE===========================================================*/
char * readconfig(char *file) {

  char *buffer=NULL;
  FILE *fh = fopen(file, "rb");
  if ( fh != NULL )
    {
      fseek(fh, 0L, SEEK_END);
      long s = ftell(fh);
      rewind(fh);
      buffer = malloc(s+1);
      if ( buffer != NULL )
        {
          fread(buffer, s, 1, fh);
          // we can now close the file
          fclose(fh); fh = NULL;

          // do something, e.g.
          //fwrite(buffer, s, 1, stdout);

          //free(buffer);
        } else { printf("buffer non allocato\n"); return (char *)NULL;}
      if (fh != NULL) fclose(fh);
    }
  return buffer;
}


enum demo_protocols {
	PROTOCOL_FRAGGLE,
	/* always last */
	DEMO_PROTOCOL_COUNT
};

/* fraggle protocol */

enum fraggle_states {
	FRAGSTATE_START_MESSAGE,
	FRAGSTATE_RANDOM_PAYLOAD,
	FRAGSTATE_POST_PAYLOAD_SUM,
};

struct per_session_data__fraggle {
  int packets_left;
  int total_message;
  int leftover;
  unsigned long sum;
  
  char *pl;
  enum fraggle_states state;
};

static int
callback_fraggle(struct libwebsocket_context *context,
			struct libwebsocket *wsi,
			enum libwebsocket_callback_reasons reason,
			void *user, 
			void *in, 
			size_t len)
{
	int n=0;
	int tosent;
	unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + 8000 +  LWS_SEND_BUFFER_POST_PADDING];
	struct per_session_data__fraggle *psf = user;
	//int chunk;
	int write_mode = LWS_WRITE_CONTINUATION;
	unsigned long sum;
	unsigned char *p = (unsigned char *)in;
	unsigned char *bp = &buf[LWS_SEND_BUFFER_PRE_PADDING];

#ifdef MODE
	printf("LWS_WRITE_BINARY = %i\n\
LWS_WRITE_BINARY|LWS_WRITE_NO_FIN = %i\n\
LWS_WRITE_CONTINUATION|LWS_WRITE_NO_FIN = %i\n\
LWS_WRITE_CONTINUATION = %i\n -----------------------------\n",
LWS_WRITE_BINARY,
LWS_WRITE_BINARY|LWS_WRITE_NO_FIN,
LWS_WRITE_CONTINUATION|LWS_WRITE_NO_FIN,
LWS_WRITE_CONTINUATION);
#endif
	
	switch (reason) {

	case LWS_CALLBACK_ESTABLISHED:

		fprintf(stderr, "server sees client connect\n");
		psf->state = FRAGSTATE_START_MESSAGE;
		/* start the ball rolling */
		libwebsocket_callback_on_writable(context, wsi);

		break;

	case LWS_CALLBACK_CLIENT_ESTABLISHED:

		fprintf(stderr, "client connects to server\n");
		psf->state = FRAGSTATE_START_MESSAGE;
		break;

	case LWS_CALLBACK_CLIENT_RECEIVE:

		switch (psf->state) {

		case FRAGSTATE_START_MESSAGE:
			psf->state = FRAGSTATE_RANDOM_PAYLOAD;
			psf->sum = 0;
			psf->total_message = 0;
			psf->packets_left = 0;

			/* fallthru */
		case FRAGSTATE_RANDOM_PAYLOAD:		

		  psf->sum +=  len;
		  printf("\n");
		  psf->total_message += len;
		  psf->packets_left++;
		  fprintf(stderr, "Fragment: %i bytes (total till now: %li)\n %s ", len,psf->sum,p);
		  if (libwebsocket_is_final_fragment(wsi)) {
		    psf->state = FRAGSTATE_POST_PAYLOAD_SUM;		    
		  }
		  printf ("write_mode (case BETWEEN)=%i\n",write_mode);
		  break;
		  
		case FRAGSTATE_POST_PAYLOAD_SUM:
		  
		  sum = ((unsigned int)p[0]) << 24;
		  sum |= p[1] << 16;
		  sum |= p[2] << 8;
		  sum |= p[3];
		  if (sum == psf->sum) {
		    fprintf(stderr, "EOM received %d correctly "
			    "from %d fragments\n",
			    psf->total_message, psf->packets_left);
		    
		  }
		  else
		    fprintf(stderr, "**** ERROR at EOM: "
			    "length %d, rx sum = %li, "
			    "server says it sent %li\n",
			    psf->total_message, psf->sum, sum);
		  
		   //		  psf->state = FRAGSTATE_START_MESSAGE;
		  terminate =1;
	
		  break;
		}
		break;
		
	case LWS_CALLBACK_SERVER_WRITEABLE:
	  
		switch (psf->state) {

		case FRAGSTATE_START_MESSAGE:
		
			psf->packets_left= (StringL / BUFFER) + 1 ;
			psf->leftover=(StringL % BUFFER );
			fprintf(stderr, "Spamming %d random fragments\n",psf->packets_left);
			psf->total_message = StringL;
			
			//psf->total_message = 0;
			write_mode = LWS_WRITE_TEXT;
			psf->state = FRAGSTATE_RANDOM_PAYLOAD;
			psf->sum=0; 
			printf ("write_mode in START =%i\n",write_mode);
			/* fallthru */

		case FRAGSTATE_RANDOM_PAYLOAD:

		  strncpy((char *)bp,json+psf->sum,BUFFER);			
		
			psf->packets_left--;
			if (psf->packets_left) {
				write_mode |= LWS_WRITE_NO_FIN;
				tosent=BUFFER;
			} else {
				psf->state = FRAGSTATE_POST_PAYLOAD_SUM;
				tosent=psf->leftover;
			}

			n = libwebsocket_write(wsi, bp, tosent, write_mode);
			psf->sum += n;
			
			if (n < 0)
				return -1;
			if (n < tosent) {
				lwsl_err("Partial write\n");
				return -1;
			}
			printf ("write_mode in RANDOM =%i\n",write_mode);
			libwebsocket_callback_on_writable(context, wsi);
			break;

		case FRAGSTATE_POST_PAYLOAD_SUM:
			printf ("write_mode in END =%i\n",write_mode);
			fprintf(stderr, "Spamming session over, "
					"len = %d. sum = 0x%lX\n",
						  psf->total_message, psf->sum);
			/*il numero psf-sum lo mando come 4 bytes*/
			bp[0] = psf->sum >> 24;
			bp[1] = psf->sum >> 16;
			bp[2] = psf->sum >> 8;
			bp[3] = psf->sum;

			/*n = libwebsocket_write(wsi, (unsigned char *)bp, 4, LWS_WRITE_BINARY);
			if (n < 0)
				return -1;
			if (n < 4) {
				lwsl_err("Partial write\n");
				return -1;
				}*/
			break;
		}
		break;
	/* because we are protocols[0] ... */

	case LWS_CALLBACK_CLIENT_CONFIRM_EXTENSION_SUPPORTED:
		if (strcmp(in, "deflate-stream") == 0) {
		  fprintf(stderr, "denied deflate-stream extension\n");
		  return 1;
		}
		break;

	default:
		break;
	}

	return 0;
}



/* list of supported protocols and callbacks */

static struct libwebsocket_protocols protocols[] = {
	{
		"fraggle-protocol",
		callback_fraggle,
		sizeof(struct per_session_data__fraggle),
	},
	{
		NULL, NULL, 0		/* End of list */
	}
};

static struct option options[] = {
	{ "help",	no_argument,		NULL, 'h' },
	{ "debug",	required_argument,	NULL, 'd' },
	{ "port",	required_argument,	NULL, 'p' },
	{ "interface",  required_argument,	NULL, 'i' },
	{ "client",	no_argument,		NULL, 'c' },
	{ NULL, 0, 0, 0 }
};

int main(int argc, char **argv)
{
	int n = 0;
	int port = 7681;

	
	struct libwebsocket_context *context;
	int opts = 0;
	char interface_name[128] = "";
	const char *iface = NULL;
	struct libwebsocket *wsi;
	const char *address;
	int server_port = port;
	struct lws_context_creation_info info;

	json=readconfig("/home/reario/wsc/gh.json");
	StringL=strlen(json);
	
	memset(&info, 0, sizeof info);

	fprintf(stderr, "libwebsockets test fraggle\n"
			"(C) Copyright 2010-2015 Andy Green <andy@warmcat.com> "
						    "licensed under LGPL2.1\n");

	while (n >= 0) {
		n = getopt_long(argc, argv, "ci:hsp:d:", options, NULL);
		if (n < 0)
			continue;
		switch (n) {
		case 'd':
			lws_set_log_level(atoi(optarg), NULL);
			break;
		case 'p':
			port = atoi(optarg);
			server_port = port;
			break;
		case 'i':
			strncpy(interface_name, optarg, sizeof interface_name);
			interface_name[(sizeof interface_name) - 1] = '\0';
			iface = interface_name;
			break;
		case 'c':
			client = 1;
			fprintf(stderr, " Client mode\n");
			break;
		case 'h':
			fprintf(stderr, "Usage: libwebsockets-test-fraggle "
					"[-d <log bitfield>] "
					"[--client]\n");
			exit(1);
		}
	}

	if (client) {
		server_port = CONTEXT_PORT_NO_LISTEN;
		if (optind >= argc) {
			fprintf(stderr, "Must give address of server\n");
			return 1;
		}
	}

	info.port = server_port;
	info.iface = iface;
	info.protocols = protocols;
	info.gid = -1;
	info.uid = -1;
	info.options = opts;

	context = libwebsocket_create_context(&info);
	if (context == NULL) {
		fprintf(stderr, "libwebsocket init failed\n");
		return -1;
	}

	if (client) {
		address = argv[optind];
		fprintf(stderr, "Connecting to %s:%u\n", address, port);
		wsi = libwebsocket_client_connect(context, address,
						   port, 0, "/", address,
				 "origin", protocols[PROTOCOL_FRAGGLE].name,
								  -1);
		if (wsi == NULL) {
			fprintf(stderr, "Client connect to server failed\n");

		}
			
	}

	n = 0;
	while (!n && !terminate)
		n = libwebsocket_service(context, 50);
	fprintf(stderr, "Terminating...\n");
	libwebsocket_context_destroy(context);
	return 0;
}
