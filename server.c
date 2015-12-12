#include <math.h>
#include "server.h"

/* Gli input si riferiscono alla posizione del bit nella variabile inlong del programma */
extern JsonNode *node /* contiene TUTTO il JSON */;
extern JsonNode *SoBs; /* la parte JSON per Spie o Bobine */
extern JsonNode *Energia;
extern JsonNode *E; // generico elemento

extern char *gh;
extern char *gh_current;
extern int StringL;

//#define CHECK
#ifdef CHECK
#include <oath.h>
/* current token*/
char * tok;
#endif





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

/*==============================CALLBACKs===========================================================*/
static int callback_http(struct libwebsocket_context * this,
		struct libwebsocket *wsi,
		enum libwebsocket_callback_reasons reason, void *user,
		void *in, size_t len)
{
	return 0;
}

/*****> SPIE_BOBINE */
// si trova sul file spie_bobine.c

/*****> ENERGY */
// si trova sul file energy.c

#ifdef CHECK
static int callback_totp(struct libwebsocket_context * this,
		struct libwebsocket *wsi,
		enum libwebsocket_callback_reasons reason,
		void *user,
		void *in,
		size_t len)
{
	int n,m;

	unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING+1024+LWS_SEND_BUFFER_POST_PADDING];
	unsigned char *p = &buf[LWS_SEND_BUFFER_PRE_PADDING];

	switch (reason) {

		case LWS_CALLBACK_ESTABLISHED: // just log message that someone is connecting
		printf("**** connection established for protocol *TOTP*\n");

		break;

		case LWS_CALLBACK_SERVER_WRITEABLE:

		n = sprintf((char *)p,tok);// la variabile globale tok contiene il token ricevuto nella callback receive
		m = libwebsocket_write(wsi, p, n, LWS_WRITE_TEXT);
		if (m < n) {
			lwsl_err("ERROR %d writing to di socket totp (returned %d)\n", n,m);
			return -1;
		}


		/*
		 Qui le routine per il controllo della validità del token
		 */
		char * sec="ipadsmf01";
		size_t seclen;
		time_t now, t0, time_step_size;
		int valid;
		seclen = (size_t)(strlen(sec)+1);
		time_step_size=OATH_TOTP_DEFAULT_TIME_STEP_SIZE;
		t0=OATH_TOTP_DEFAULT_START_TIME;
		now=time(NULL);
		valid=oath_totp_validate (sec, //const char *secret,
				seclen,//size_t secret_length,
				now,// time_t now,
				time_step_size,//unsigned  time_step_size,
				t0,// time_t start_offset,
				0,//size_t window,
				tok//const char *otp
		);
		printf("-->%d\n",valid);
		if (valid==OATH_INVALID_OTP) {printf("INVALID\n");} else {printf("VALID\n");}

		fprintf(stderr,"ti ho rimandato questo token: %s\n",tok);
		break;

		case LWS_CALLBACK_RECEIVE: // il browser ha fatto una websocket.send e il valore è nella variabile *in*
		tok=malloc(sizeof(char)*len+1);
		strcpy(tok,(char *)in);// mi memorizzo il token ricevuto in una variabile globale
		fprintf(stderr, "valore del token ricevuto: %s\n", (char *)in);
		libwebsocket_callback_on_writable(this,wsi);// richiedi di la disponibilità a scrivere        
		break;

		case LWS_CALLBACK_CLOSED:
		printf("connection *CLOSED* for TOTP\n");
		break;
		default:
		break;
	}

	return 0;
}
#endif // check
/*============================END CALBACKs======================================*/

/*============================PROTOCOLS=========================================*/
enum protocols {
	/* always first */
	PROTOCOL_HTTP = 0,

	PROTOCOL_ENERGY, PROTOCOL_SPIE_BOBINE, 
	#ifdef CHECK
	PROTOCOL_TOTP,
	#endif
	/* always last */
	DEMO_PROTOCOL_COUNT
};

static struct libwebsocket_protocols protocols[] = {
/* first protocol must always be HTTP handler */
  { "http-only",   // name
    callback_http, // callback
    0              // per_session_data_size
  }, { "energy", // protocol name - very important!
       callback_energy,   // callback
       sizeof(struct per_session_data_fraggle)
  }, { "spie_bobine", // protocol name - very important!
       callback_spie_bobine,   // callback
       sizeof(struct per_session_data_fraggle)
  },
  
#ifdef CHECK
  { "totp", // protocol name - very important!
    callback_totp,   // callback
    0                  // we don't use any per session data
  },
#endif
  { NULL, NULL, 0 /* End of list */
  } };
/*==========================================================================*/




/*============================MAIN==============================================*/
int main(void) {
  
  modbus_t *mb;
  uint16_t tab_reg[17]; // max 15 reg. in realtà ne servono 16
  // uint16_t x; // usata per loop dentro la variabile inlong di 64 bit  
  
  mb = modbus_new_tcp("192.168.1.103", 502);
  
  if (modbus_connect(mb) == -1) {
    fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
    modbus_free(mb);
    return -1;
  }
  
  // server url will be http://localhost:9999
  int port = 8081;
  int n = 0;
  unsigned int ms, oldms = 0;
  struct libwebsocket_context *context;
  
  struct lws_context_creation_info context_info =
    {
      .port = port,
      .iface = NULL,
      .protocols = protocols,
      .extensions = NULL,
      .ssl_cert_filepath = NULL,
      .ssl_private_key_filepath = NULL,
      .ssl_ca_filepath = NULL,
      .gid = -1,
      .uid = -1,
      .options = 0, NULL,
      .ka_time = 0,
      .ka_probes = 0,
      .ka_interval = 0
    };
  
  // create libwebsocket context representing this server
  context = libwebsocket_create_context(&context_info);
  
  if (context == NULL) {
    fprintf(stderr, "libwebsocket init failed\n");
    return -1;
  }
  
  
  printf("starting server...\n");
  version();

  /*****************************************************/
  /* JSON STUFF */
  /*****************************************************/
  
  
  gh=readconfig("gh.json");
  StringL=strlen(gh);

  printf("%s\n",gh);  
  node=json_decode(gh);

  if (node == NULL) {
    printf("Failed to decode \n");
    exit(EXIT_FAILURE);
  }
  SoBs=json_find_member(node,"SoBs");
  Energia=json_find_member(node,"Energia");

  /*	json_foreach(En,Energia) 
	{     
		printf("%lf - %lf\n",
		       json_find_member(En,"V")->number_,
		       json_find_member(En,"I")->number_);
	}
  *****************************************************/ 


  if (lws_daemonize("/tmp/.lwsts-lock")) {
    fprintf(stderr, "Failed to daemonize\n");
    return 1;
  }
  

// infinite loop, to end this server send SIGTERM. (CTRL+C)

  while (n >= 0) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    
    /*
     * This provokes the LWS_CALLBACK_SERVER_WRITEABLE for every
     * live websocket connection using the ENERGY protocol,
     * as soon as it can take more packets (usually immediately)
    */
    ms = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
    if ((ms - oldms) > 50) {
      /* Read 12 registers from the address 65 */
      
      if (modbus_read_registers(mb, 65, 16, tab_reg) > 0) {
	
	
	/*------------------- PLC IN--------------------------------------*/		  
	in1=tab_reg[0]; /* 65 */
	in2=tab_reg[1]; /* 66 */
	in3=tab_reg[15]; /* Ingressi Modulo esterno del PLC TM2... (80) */
	plc_dout=tab_reg[2]; /* uscite del PLC */
	/*------------------- OTB DIGITAL IN------------------------------*/
	otb_din=tab_reg[9];
	otb_dout=tab_reg[12];
	inlong=0;
	
	inlong=place64(inlong,in1,0);      // 0-15
	inlong=place64(inlong,in2,16);     // 16-22
	inlong=place64(inlong,in3,23);     // 23-38
	inlong=place64(inlong,otb_din,39); // 39-50
	/* adesso in inlong ho lo stato di tutti gli input nelle posizioni sopra indicate */
	
	// VALORI ANALOGICI (PM9 + OTB)
	I=(float)(tab_reg[4]+(tab_reg[3]<<16))/1000; // PM9        
	V=(float)(tab_reg[6]+(tab_reg[5]<<16))/1000; // PM9        
	P=(float)(tab_reg[8]+(tab_reg[7]<<16))/100;  // PM9        
	Bar=(float)(tab_reg[10]*0.002442); // INPUT ANALOGICO OTB
	Bar_pozzo=(float)(tab_reg[11]*0.002442); // INPUT ANALOGICO OTB
	//printf("-----------------\n");
	uint16_t input;

	json_foreach(E,SoBs) {
	  if (E) {
	    input=json_find_member(E,"input")->number_;
	    if (input>=0 && input!=1000) { /* 
					      è una spia e/o una bobina. 1000 è un upperbound arbitrario,
					      che rappresenta il num massimo che può assumere un input
					   */
	      //number_ è un duble e non posso assegnargli quello che ritorna read_single_state64
	      json_find_member(E,"stato")->number_=read_single_state64(inlong,input)==1?1:0;
	    }
	    
	  }
	}  
	json_find_member(Energia,"V")->number_=V;
	json_find_member(Energia,"I")->number_=I;
	json_find_member(Energia,"P")->number_=P;
	json_find_member(Energia,"BAR")->number_=Bar;
	json_find_member(Energia,"BAR_POZZO")->number_=Bar_pozzo;
	gh_current = json_stringify(node,NULL);
	json_delete(E);
	
      } else {
	printf("ERR: modbus read registers riprovo a creare il context\n");
	modbus_close(mb);
	modbus_free(mb);
	mb = modbus_new_tcp("127.0.0.1", 502);
	if (modbus_connect(mb) == -1) {
	  fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
	  modbus_free(mb);
	  return -1;
	}
      } // else
      oldms = ms;
    } // (ms - oldms) > 50) 
    n = libwebsocket_service(context, 50);
  }
  
  modbus_close (mb);
  modbus_free (mb);
  
  libwebsocket_context_destroy (context);
  lwsl_notice("libwebsockets-test-server exited cleanly\n");
  return 0;
}
