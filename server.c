#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <libwebsockets.h>
#include <math.h>
#include <modbus.h>
#include <oath.h>
#include "json.h"
#include "server.h"

static volatile float V, I, P, Bar, Bar_pozzo;
static volatile uint16_t in1; /*prima parte dei 16 input, quelli da 0 a 15 : registro 65 sul plc master*/
static volatile uint16_t in2; /*prima parte dei 16 input, quelli da 16 a 23 : registro 66 sul plc master*/
static volatile uint16_t in3; /*8 ingressi digitali sul modulo di espansione del PLC*/
static volatile uint16_t otb_din; /* ingressi digitali dell'OTB */
static volatile uint16_t plc_dout; /*  OUT digitali del PLC */
static volatile uint16_t otb_dout; /*  OUT digitali dell'OTB */
static volatile uint64_t inlong=0; /* contiene fino a 64 ingressi digitali concatenazione di in1 in2, in3 e otb_din */

static JsonNode *node=NULL /* contiene TUTTO il JSON */;
static JsonNode *SoBs=NULL; /* la parte JSON per Spie o Bobine */
static JsonNode *Energia=NULL;
static JsonNode *E=NULL; // generico elemento
static char *gh; /* stringa che contiene il file di configuarazione JSON di spie, bobine e energia (VIP,BAR,BAR pozzo) */
static char *gh_current; /*stringa generata dinamicamente che contiene i valori aggiornati da inviare al client */

/* current token*/
char * tok;

void printbitssimple64(uint64_t n) {
  /*dato l'intero n stampa la rappresentazione binaria*/
  uint64_t i;
  int j;
  i = (uint64_t)1<<(sizeof(n) * 8 - 1); /* 2^n */
  for (j=63;j>=0;j--) {   
    printf(" %2i",j);
  }
  printf("\n");  

  while (i > 0) {
    if (n & i)
      printf(" %2i",1);
    else
      printf(" %2i",0);
    i >>= 1;
  }
  printf("\n");
}

uint16_t read_single_state64(uint64_t reg, uint16_t q) {
	/*legge q-esomo bit di reg*/
  if (q < (uint16_t)64) {
		uint64_t i=0;

		i = ((uint64_t)1 << q); /* 2^q */
		//printf("i=%u\n",i);
		if (reg & i) {
		  return (uint16_t)1;
		} else {
		  return (uint16_t)0;
		};
	} else {
		return -1;
	}
}

uint16_t read_single_state(uint16_t reg, uint16_t q) {
	/*legge q-esomo bit di reg*/
	if (q < 16) {
		uint16_t i;
		i = (1 << q); /* 2^q */
		if (reg & i) {
			return 1;
		} else {
			return 0;
		};
	} else {
		return -1;
	}
}

uint16_t invert_state(uint16_t reg, uint16_t q) {
	/*inverte il q-esimo bit*/
	return reg ^ (1 << q);
}

/*==============================ATTIVAZIONI===========================================================*/

uint16_t attiva(modbus_t *m, char *t, double registro, double bit) {
  
  /* per la conversione double-->int vedi:  http://c-faq.com/fp/round.html C FAQ num 14.6*/
  int b=(int)(bit+0.5);

  /* servono per leggere lo stato del bit remoto */
  uint8_t *bits=NULL;
  bits=(uint8_t *)malloc(nbits*sizeof(uint8_t));
  memset(bits, 0, nbits * sizeof(uint8_t));
  /***********************************************/

  // int r=(int)(registro +0.5);


  ///////////////////////////////////////////////////////
  // Si richiede di aggiornare una Bobina come Pulsante
  if ( (registro<0) && (strncmp(t,"P",(size_t)1)!=0)) { 
    printf("attivo la bobina %i di tipo %s\n",b,t); 
    // la metto a ON
    if (modbus_write_bit(m, b, TRUE) != 1) {
      printf("ERRORE DI SCRITTURA:PULSANTE ON\n");
      return -1;
    }
    sleep(1);
    // la metto a OFF
    if (modbus_write_bit(m, b, FALSE) != 1) {
      printf("ERRORE DI SCRITTURA:PULSANTE OFF\n");
      return -1;
    }
    return 1;
  }
  //////////////////////////////////////////////////////////



  /////////////////////////////////////////////////////////
  // Si richiede di aggiornare una Bobina come Interruttore
  if ( (registro<0) && (strncmp(t,"I",(size_t)1)!=0)) { 
    // leggo i bit remoto il cui valore si trova in bits[0]
    modbus_read_bits(mb, b, 1, bits);
    if (bits[0]==ON) { modbus_write_bit(m, b, FALSE) } else {modbus_write_bit(m, b, TRUE)}
    return 1;
  }
  //////////////////////////////////////////////////////////





  return 1;
}
    	
      


/*==============================READ JSON FILE===========================================================*/
char * readconfig(char *file) {

  char *buffer;
  FILE *fh = fopen(file, "rb");
  if ( fh != NULL )
    {
      fseek(fh, 0L, SEEK_END);
      long s = ftell(fh);
      rewind(fh);
      buffer = malloc(s);
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

/*==============================Estraggo i SoBs======================================================*/





/*==============================CALLBACKs===========================================================*/
static int callback_http(struct libwebsocket_context * this,
		struct libwebsocket *wsi,
		enum libwebsocket_callback_reasons reason, void *user,
		void *in, size_t len)
{
	return 0;
}

/*****> ENERGY */
static int callback_energy(struct libwebsocket_context * this,
		struct libwebsocket *wsi,
		enum libwebsocket_callback_reasons reason,
		void *user,
		void *in,
		size_t len)

{
	int n,m;
	modbus_t *mbw;
	JsonNode *Elem,*S;
	
	// char PLC_IP[] = "192.168.1.157";
	//	char OTB_IP[] = "192.168.1.11";

	unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING+4096+LWS_SEND_BUFFER_POST_PADDING];
	unsigned char *p = &buf[LWS_SEND_BUFFER_PRE_PADDING];
	// le tre libwebsocket_callback_on_writable sostituiscono quelle nel main che erano presenti in precedenza
	switch (reason) {

	case LWS_CALLBACK_ESTABLISHED: // just log message that someone is connecting
	  fprintf(stderr,"**** connection established for protocol*energy*\n");
	  libwebsocket_callback_on_writable(this,wsi);// comincia tutto il processo richiedendo la disponibilità a scrivere sul socket
	  break;
	  
	case LWS_CALLBACK_RECEIVE:// il browser ha fatto una websocket.send e il valore è nella variabile *in*
	  S=json_find_member(node,"SoBs");
	  Elem=json_find_member(S,(char *)in);
	  char *ip,*type;
	  double bobina,reg;
	  
	  if (Elem) {
	    ip=json_find_member(Elem,"ip")->string_;
	    type=json_find_member(Elem,"type")->string_;
	    
	    
	    if (json_find_member(Elem,"rele")->tag==JSON_NUMBER) {
	      reg=-1;
	      bobina=json_find_member(Elem,"rele")->number_;
	    } else { /* è un array: il primo valore è il registro, il secondo il bit del registro da accender/spegnere */
	      reg=json_find_member(Elem,"rele")->children.head->number_;
	      bobina=json_find_member(Elem,"rele")->children.tail->number_;

	    }


	    /* viene chiamata quando si clicca su una bobina*/
	    mbw = modbus_new_tcp(ip, 502);
	    if (modbus_connect(mbw) == -1) {
	      fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
	      modbus_free(mbw);
	      return -1;
	    }
	    //pulsante(mbw,(int)json_find_member(Elem,"rele")->number_);
	    attiva(mbw,type,reg,bobina);
	    modbus_close(mbw);
	    modbus_free(mbw);
	    libwebsocket_callback_on_writable(this,wsi); // richiedi la disponibilità a scrivere  
	  }
	  else {
	    printf("Ricevuto un comando non riconosciuto");
	  }
	  

	  
	  break;
	  
	case LWS_CALLBACK_SERVER_WRITEABLE: 


	  // faccio la write per aggiornare lo stato delle spie appena il socket è disponibile. In questo modo implemeto il push dei dati
	  n=sprintf((char *)p,gh_current);
	  m = libwebsocket_write(wsi, p, n, LWS_WRITE_TEXT);
	  if (m < n) {
	    lwsl_err("ERROR %d writing to di socket (returned %d)\n", n,m);
	    return -1;
	  }
	  //printf("%s\n",p);
	  usleep((useconds_t)50000); // 50 ms di pausa altrimenti il browser non gli sta dietro e si rallenta
	  libwebsocket_callback_on_writable(this,wsi);// richiedi di nuovo la disponibilità a scrivere
	  break;
	  
	case LWS_CALLBACK_CLOSED:
	  fprintf(stderr,"connection *CLOSED* for energy\n");
	  break;
	  
	default:
	  break;
	}
	
	return 0;
}

/*****>  spie_bobine*/
static int callback_spie_bobine(
  struct libwebsocket_context * this,
  struct libwebsocket *wsi,
  enum libwebsocket_callback_reasons reason,
  void *user,
  void *in,
  size_t len)
{
  /* questa callback invia al client (browser) la lista delle spie e delle bobine in bodo che il browser crei la maschera relativa 
     la variabile è contenuta dentro la stringa JSON "spie_bobine"
   */
	int n,m;

	unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING+4500+LWS_SEND_BUFFER_POST_PADDING];
	unsigned char *p = &buf[LWS_SEND_BUFFER_PRE_PADDING];

	switch (reason) {
	  // faccio solo una chiamata a server_writeable perchè invio solo una volta i valori
	case LWS_CALLBACK_ESTABLISHED:// just log message that someone is connecting
	  fprintf(stderr,"**** connection established for protocol *spie_bobine*\n");
	  libwebsocket_callback_on_writable(this,wsi);// chiede la disponibilità a scrivere sul socket
	  break;
	  
	case LWS_CALLBACK_SERVER_WRITEABLE:// se c'è un socket aperto e questo è disponibile, invio la lista delle spie e bobine
	  /* 8<------8<------8<------8<------8<------8<------8<------8<------8<------8< */
	  //n = sprintf((char *)p,spie_bobine);
	  /* 8<------8<------8<------8<------8<------8<------8<------8<------8<------8< */
	  n = sprintf((char *)p,gh_current); /* invio il json letto dal file statico per dire al client la conformazione di tutto */	  
	  
	  m = libwebsocket_write(wsi, p, n, LWS_WRITE_TEXT);
	  fprintf(stderr,"constants spie and bobine sent\n");
	  	  if (m < n) {
	    //	    [1448056884:7846] ERR: ERROR 4471 writing to di socket spie_bobine (returned 4376)
	    lwsl_err("ERROR %d writing to socket spie_bobine (returned %d)\n", n,m);
	    return -1;
	    }
	  break;
	  
	case LWS_CALLBACK_CLOSED:
	  fprintf(stderr,"connection *CLOSED* for spie_bobine\n");
	  break;
	default:
	  break;
	}

	return 0;
}

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
#define CHECK
#ifdef CHECK
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
#endif
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

/*============================END CALBACKs======================================*/

/*============================PROTOCOLS=========================================*/
enum protocols {
	/* always first */
	PROTOCOL_HTTP = 0,

	PROTOCOL_ENERGY, PROTOCOL_SPIE_BOBINE, PROTOCOL_TOTP,

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
				0                  // we don't use any per session data
		}, { "spie_bobine", // protocol name - very important!
				callback_spie_bobine,   // callback
				0                  // we don't use any per session data
		}, { "totp", // protocol name - very important!
				callback_totp,   // callback
				0                  // we don't use any per session data
		},

		{ NULL, NULL, 0 /* End of list */
		} };
/*==========================================================================*/


uint64_t place (uint64_t dest, uint16_t source, uint16_t pos) {

  /* places the string of bit represented by source in position pos (starting from left) into dest */
 
  uint64_t temp=0;

  temp=temp|source;
  temp=temp<<pos;
  dest=dest|temp;
  return dest;

}


/*============================MAIN==============================================*/
int main(void) {
  
  modbus_t *mb;
  uint16_t tab_reg[17]; // max 15 reg. in realtà ne servono 16
  // uint16_t x; // usata per loop dentro la variabile inlong di 64 bit  
  
  mb = modbus_new_tcp("127.0.0.1", 502);
  
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
  // 
  
  /*****************************************************/
  /* JSON STUFF */
  /*****************************************************/
  gh=readconfig("gh.json");

  // printf("%s\n",gh);  
  node=json_decode(gh);

  //free(gh);
  if (node == NULL) {
    printf("Failed to decode \n");
    exit(EXIT_FAILURE);

  }
  SoBs=json_find_member(node,"SoBs");
  Energia=json_find_member(node,"Energia");

  /*
	json_foreach(En,Energia) 
	{     
		printf("%lf - %lf\n",
		       json_find_member(En,"V")->number_,
		       json_find_member(En,"I")->number_);
	}
  
  *****************************************************/

  
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
	
	inlong=place(inlong,in1,0);      // 0-15
	inlong=place(inlong,in2,16);     // 16-22
	inlong=place(inlong,in3,23);     // 23-38
	inlong=place(inlong,otb_din,39); // 39-50
	/* adesso in inlong ho lo stato di tutti gli input nelle posizioni sopra indicate */
	
	// VALORI ANALOGICI (PM9 + OTB)
	I=(float)(tab_reg[4]+(tab_reg[3]<<16))/1000; // PM9        
	V=(float)(tab_reg[6]+(tab_reg[5]<<16))/1000; // PM9        
	P=(float)(tab_reg[8]+(tab_reg[7]<<16))/100;  // PM9        
	Bar=(float)(tab_reg[10]*0.002442); // INPUT ANALOGICO OTB
	Bar_pozzo=(float)(tab_reg[11]*0.002442); // INPUT ANALOGICO OTB
	//printf("-----------------\n");
	uint16_t input=1000;

	json_foreach(E,SoBs) {
	  if (E) {
	    input=json_find_member(E,"input")->number_;
	    if (input>=0 && input!=1000) { /* è una spia e/o una bobina */
	      json_find_member(E,"stato")->number_=read_single_state64(inlong,input)==1?1:0;
	    }
	   
	  }
	}  
	json_find_member(Energia,"V")->number_=V;
	json_find_member(Energia,"I")->number_=I;
	json_find_member(Energia,"P")->number_=P;
	json_find_member(Energia,"BAR")->number_=Bar;
	json_find_member(Energia,"BAR_POZZO")->number_=Bar_pozzo;
	//	    printf("%s\n",json_stringify(Energia,"\t"));
	gh_current = json_stringify(node,NULL);
	//	printf("%s\n",gh_current);
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
