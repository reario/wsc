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


/* 8<------8<------8<------8<------8<------8<------8<------8<------8<------8< */
static volatile uint16_t io1, io2, OTBDIN;;
/* 8<------8<------8<------8<------8<------8<------8<------8<------8<------8< */

#define QWORD 64
static volatile float V, I, P, Bar, Bar_pozzo;
static volatile uint16_t in1; /*prima parte dei 16 input, quelli da 0 a 15 : registro 65 sul plc master*/
static volatile uint16_t in2; /*prima parte dei 16 input, quelli da 16 a 23 : registro 66 sul plc master*/
static volatile uint16_t in3; /*8 ingressi digitali sul modulo di espansione del PLC*/
static volatile uint16_t otb_din; /* ingressi digitali dell'OTB */
static volatile uint64_t inlong=0; /* contiene fino a 64 ingressi digitali concatenazione di in1 in2, in3 e otb_din */

static JsonNode *node=NULL /* contiene TUTTO il JSON */;
static JsonNode *SoBs=NULL; /* la parte JSON per Spie o Bobine */
// static JsonNode *SoB=NULL; /* La singola spia o bobina all'interno di SoBs */
static JsonNode *Energia=NULL;
static JsonNode *E=NULL; // generico elemento
static char *gh; /* stringa che contiene il file di configuarazione JSON di spie, bobine e energia (VIP,BAR,BAR pozzo) */
static char *gh_current; /*stringa generata dinamicamente che contiene i valori aggiornati da inviare al client */

char *inputs_names[]={ 
  "AUTOCLAVE", // 0
  "POMPA_SOMMERSA", // 1
  "RIEMPIMENTO", // 2
  "LUCI_ESTERNE_SOTTO", // 3
  "CENTR_R8", // 4
  "LUCI_GARAGE_DA_4", // 5
  "LUCI_GARAGE_DA_2", // 6
  "LUCI_TAVERNA_1_di_2", // 7
  "LUCI_TAVERNA_2_di_2", // 8
  "INTERNET",            // 9
  "C9912",              // 10
  "LUCI_CUN_LUN",       // 11
  "LUCI_CUN_COR",       // 12
  "LUCI_STUDIO_SOTTO",  // 13
  "LUCI_ANDRONE_SCALE", // 14
  "GENERALE_AUTOCLAVE", // 15
  "LUCI_CANTINETTA",    // 16 
  "PLC_INPUT_17", // 17
  "PLC_INPUT_18", // 18
  "PLC_INPUT_19", // 19
  "PLC_INPUT_20", // 20
  "PLC_INPUT_21", // 21
  "PLC_INPUT_22", // 22
  "PLC_TM2_INPUT_0",  // 23
  "PLC_TM2_INPUT_1",  // 24
  "PLC_TM2_INPUT_2",  // 25
  "PLC_TM2_INPUT_3",  // 26
  "PLC_TM2_INPUT_4",  // 27
  "PLC_TM2_INPUT_5",  // 28
  "PLC_TM2_INPUT_6",  // 29
  "PLC_TM2_INPUT_7",  // 30
  "PLC_TM2_INPUT_8",  // 31
  "PLC_TM2_INPUT_9",  // 32
  "PLC_TM2_INPUT_10", // 33
  "PLC_TM2_INPUT_11", // 34
  "PLC_TM2_INPUT_12", // 35
  "PLC_TM2_INPUT_13", // 36
  "PLC_TM2_INPUT_14", // 37
  "PLC_TM2_INPUT_15", // 38
  "OTB_DIN_0", // 39
  "OTB_DIN_1", // 40
  "OTB_DIN_2", // 41
  "OTB_DIN_3", // 42
  "OTB_DIN_4", // 43
  "OTB_DIN_5", // 44
  "OTB_DIN_6", // 45
  "OTB_DIN_7", // 46
  "OTB_DIN_8", // 47
  "OTB_DIN_9", // 48
  "OTB_DIN_10",// 49
  "FARETTI_ESTERNI",// 50  (OTB)
  "IN_51",  // 51
  "IN_52",  // 52
  "IN_53",  // 53
  "IN_54",  // 54
  "IN_55",  // 55
  "IN_56",  // 56
  "IN_57",  // 57
  "IN_58",  // 58
  "IN_59",  // 59
  "IN_60",  // 60
  "IN_61",  // 61
  "IN_62",  // 62
  "IN_63"   // 63

};

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

int pulsante(modbus_t *m, int bobina) {

	if (modbus_write_bit(m, bobina, TRUE) != 1) {
		printf("ERRORE DI SCRITTURA:PULSANTE ON");
		return -1;
	}
	sleep(1);

	if (modbus_write_bit(m, bobina, FALSE) != 1) {
		printf("ERRORE DI SCRITTURA:PULSANTE OFF");
		return -1;
	}
	return 0;
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
	}
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

	char PLC_IP[] = "192.168.1.157";
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
	  /* viene chiamata quando si clicca su una bobina*/
	  mbw = modbus_new_tcp(PLC_IP, 502);
	  if (modbus_connect(mbw) == -1) {
	    fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
	    modbus_free(mbw);
	    return -1;
	  }
	  fprintf(stderr, "bobina da accendere: %d\n", atoi((char *)in));
	  pulsante(mbw,atoi((char *)in));
	  modbus_close(mbw);
	  modbus_free(mbw);
	  libwebsocket_callback_on_writable(this,wsi); // richiedi la disponibilità a scrivere
	  break;
	  
	case LWS_CALLBACK_SERVER_WRITEABLE: 


	  // faccio la write per aggiornare lo stato delle spie appena il socket è disponibile. In questo modo implemeto il push dei dati

	  /* 8<------8<------8<------8<------8<------8<------8<------8<------8<------8< */
	  /*
	    n = sprintf((char *)p,"{\"Energia\":{ \"V\":%3.1f,\"I\":%2.1f,\"P\":%1.2f},\"Bar\":%2.1f,\"Bar_pozzo\":%2.1f,\"IO1\":%d,\"IO2\":%d,\
\"Stati\":{\"AUTOCLAVE\":%d,\"POMPA_SOMMERSA\":%d,\"RIEMPIMENTO\":%d,\"LUCI_ESTERNE_SOTTO\":%d,\"CENTR_R8\":%d,\"LUCI_GARAGE_DA_4\":%d,\"LUCI_GARAGE_DA_2\":%d,\"LUCI_TAVERNA_1_di_2\":%d,\"LUCI_TAVERNA_2_di_2\":%d,\"INTERNET\":%d,\"C9912\":%d,"LUCI_CUN_LUN\":%d,\"LUCI_CUN_COR\":%d,\"LUCI_STUDIO_SOTTO\":%d,\"LUCI_ANDRONE_SCALE\":%d,\"GENERALE_AUTOCLAVE\":%d,\"LUCI_CANTINETTA\":%d,"FARETTI\":%d}}",
		      V,I,P,Bar,Bar_pozzo,io1,io2,
		      read_single_state((uint16_t)io1,(uint16_t)0),// stato autoclave
		      read_single_state((uint16_t)io1,(uint16_t)1),// stato Pompa pozzo
		      read_single_state((uint16_t)io1,(uint16_t)2),// stato Riempimento serbatorio
		      read_single_state((uint16_t)io1,(uint16_t)3),// stato luci esterne
		      read_single_state((uint16_t)io1,(uint16_t)4),//  stato R8 centralino
		      read_single_state((uint16_t)io1,(uint16_t)5),//  stato luci garage da 4
		      read_single_state((uint16_t)io1,(uint16_t)6),//  stato luci garage da 2
		      read_single_state((uint16_t)io1,(uint16_t)7),//  stato taverna1
		      read_single_state((uint16_t)io1,(uint16_t)8),//  stato taverna2
		      read_single_state((uint16_t)io1,(uint16_t)9),//  stato internet
		      read_single_state((uint16_t)io1,(uint16_t)10),//  stato Centralino 9912 (luci esterne da centralino)
		      read_single_state((uint16_t)io1,(uint16_t)11),//  stato Cunicolo lungo
		      read_single_state((uint16_t)io1,(uint16_t)12),//  stato Cunicolo corto
		      read_single_state((uint16_t)io1,(uint16_t)13),//  stato luci studio sotto
		      read_single_state((uint16_t)io1,(uint16_t)14),//  stato luci androne scale
		      read_single_state((uint16_t)io1,(uint16_t)15),//  stato generale autoclave
		      read_single_state((uint16_t)io2,(uint16_t)0), //  stato luce cantinetta
		      read_single_state((uint16_t)OTBDIN,(uint16_t)11)//  stato luce FARI ESTERNI
	  );
	  */
	  /* 8<------8<------8<------8<------8<------8<------8<------8<------8<------8< */
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
  //  printf("%s\n",gh);  
  node=json_decode(gh);
  //free(gh);
  if (node == NULL) {
    printf("Failed to decode \n");
    exit(1);
  }
  SoBs=json_find_member(node,"SoBs");
  Energia=json_find_member(node,"Energia");
  printf("---------------\n");  
  /*
	json_foreach(En,Energia) 
	{     
		printf("%lf - %lf\n",
		       json_find_member(En,"V")->number_,
		       json_find_member(En,"I")->number_);
	}
  
  *****************************************************/
    printf("---------------\n");  
  
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
	
	
	/* 8<------8<------8<------8<------8<------8<------8<------8<------8<------8< */
	// VALORI DIGITALI (PLC + OTB). Questi valori sono usati nelle callback 
	/* da eliminare dopo la modifica sulla callback */
	io1=tab_reg[0];
	io2=tab_reg[1];
	OTBDIN=tab_reg[9]; // ingressi digitali OTB che il PLC ha passato al PC nella posizione 9 di tab_reg[].
	/* 8<------8<------8<------8<------8<------8<------8<------8<------8<------8< */
	
	/*------------------- PLC IN--------------------------------------*/		  
	in1=tab_reg[0]; /* 65 */
	in2=tab_reg[1]; /* 66 */
	in3=tab_reg[15]; /* Ingressi Modulo esterno del PLC TM2... (80) */
	
	/*------------------- OTB DIGITAL IN------------------------------*/
	otb_din=tab_reg[9];
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
