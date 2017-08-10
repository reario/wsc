#include "server.h"

/* la stringa JSON delle configurazioni 
int n = libwebsocket_write(wsi, &write_buffer[LWS_SEND_BUFFER_PRE_PADDING], write_len,(libwebsocket_write_protocol)write_mode);
int write_mode;
write_mode = LWS_WRITE_BINARY; // single frame, no fragmentation
write_mode = LWS_WRITE_BINARY | LWS_WRITE_NO_FIN; // first fragment
write_mode = LWS_WRITE_CONTINUATION | LWS_WRITE_NO_FIN; // all middle fragments
write_mode = LWS_WRITE_CONTINUATION; // last fragment
*/


char *gh;
int StringL;

int callback_spie_bobine(struct libwebsocket_context *context,
			struct libwebsocket *wsi,
			enum libwebsocket_callback_reasons reason,
			void *user, 
			void *in, 
			size_t len)
{
	int n=0;
	int tosent;

	struct per_session_data_fraggle *psf = user;
	int write_mode = LWS_WRITE_CONTINUATION;
	// unsigned long sum;
	// unsigned char *p = (unsigned char *)in;
	unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + 16000 +  LWS_SEND_BUFFER_POST_PADDING];
	unsigned char *bp = &buf[LWS_SEND_BUFFER_PRE_PADDING];
	
	switch (reason) {

	case LWS_CALLBACK_ESTABLISHED:

		fprintf(stderr, "server sees client connect\n");
		psf->state = START;
		/* start the ball rolling */
		libwebsocket_callback_on_writable(context, wsi);

		break;

	case LWS_CALLBACK_CLIENT_ESTABLISHED:
		fprintf(stderr, "client connects to server\n");
		psf->state = START;
		break;
		
	case LWS_CALLBACK_SERVER_WRITEABLE:
	  
	  switch (psf->state) {
	  case START:
	    	    
	    psf->packets_left= (StringL / BUFFER) + 1 ; // numero intero di frammenti lunghi BUFFER
	    psf->leftover=(StringL % BUFFER ); // sfrido
	    fprintf(stderr, "To be sent %d  fragments\n",psf->packets_left);	          
	    write_mode = LWS_WRITE_TEXT;
	    psf->state = IN_BETWEEN;
	    psf->sum=0; 
	    printf("------\n");
	    /* fallthru */
	    
	  case IN_BETWEEN:
          
	    strncpy((char *)bp,gh+psf->sum,BUFFER);				    
	    psf->packets_left--;
	    if (psf->packets_left) {
	      write_mode |= LWS_WRITE_NO_FIN;
	      tosent=BUFFER;
	    } else {
	      psf->state = END;
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
	    /* chiedo disponibilità a scrivere gli altri pacchetti */
	    libwebsocket_callback_on_writable(context, wsi);
	    break;
	    
	  case END:
	    /*il numero psf->sum lo mando come 4 bytes*/		  
	    bp[0] = psf->sum >> 24;
	    bp[1] = psf->sum >> 16;
	    bp[2] = psf->sum >> 8;
	    bp[3] = psf->sum; 

	    break; 
	  }
	  
	case LWS_CALLBACK_CLOSED:
	  fprintf(stderr,"connection *CLOSED* for spie_bobine\n");
	  break;
	  
	default:
	  break;
	}	
	return 0;
}
