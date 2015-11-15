#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <libwebsockets.h>
#include <math.h>
#include <modbus.h>
#include <oath.h>
#include "server.h"

static volatile float V, I, P, Bar, Bar_pozzo;
static volatile uint16_t io1, io2, OTBDIN;;
char * tok;

void printbitssimple(uint16_t n) {
	/*dato l'intero n stampa la rappresentazione binaria*/
	unsigned int i;
	i = 1 << (sizeof(n) * 8 - 1); /* 2^n */
	while (i > 0) {
		if (n & i)
			printf("1");
		else
			printf("0");
		i >>= 1;
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

	unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING+1024+LWS_SEND_BUFFER_POST_PADDING];
	unsigned char *p = &buf[LWS_SEND_BUFFER_PRE_PADDING];
	// le tre libwebsocket_callback_on_writable sostituiscono quelle nel main che erano presenti in precedenza
	switch (reason) {

		case LWS_CALLBACK_ESTABLISHED: // just log message that someone is connecting
		fprintf(stderr,"connection established fo *energy*\n");
		libwebsocket_callback_on_writable(this,wsi);// comincia tutto il processo richiedendo la disponibilità a scrivere sul socket
		break;

		case LWS_CALLBACK_RECEIVE:// il browser ha fatto una websocket.send e il valore è nella variabile *in*
		mbw = modbus_new_tcp("192.168.1.157", 502);
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
		  n = sprintf((char *)p,"{\"Energia\":{ \"V\":%3.1f,\"I\":%2.1f,\"P\":%1.2f},\
\"Bar\":%2.1f,\"Bar_pozzo\":%2.1f,\"IO1\":%d,\"IO2\":%d,		\
\"Stati\":{\"AUTOCLAVE\":%d,\"POMPA_SOMMERSA\":%d,\"RIEMPIMENTO\":%d,\"LUCI_ESTERNE_SOTTO\":%d,\"CENTR_R8\":%d,\"LUCI_GARAGE_DA_4\":%d,\"LUCI_GARAGE_DA_2\":%d,\"LUCI_TAVERNA_1_di_2\":%d,\"LUCI_TAVERNA_2_di_2\":%d,\"INTERNET\":%d,\"C9912\":%d, \
\"LUCI_CUN_LUN\":%d,\"LUCI_CUN_COR\":%d,\"LUCI_STUDIO_SOTTO\":%d,\"LUCI_ANDRONE_SCALE\":%d,\"GENERALE_AUTOCLAVE\":%d,\"LUCI_CANTINETTA\":%d, \
\"FARETTI\":%d}}",
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

		m = libwebsocket_write(wsi, p, n, LWS_WRITE_TEXT);
		if (m < n) {
			lwsl_err("ERROR %d writing to di socket (returned %d)\n", n,m);
			return -1;
		}
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

	unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING+1024+LWS_SEND_BUFFER_POST_PADDING];
	unsigned char *p = &buf[LWS_SEND_BUFFER_PRE_PADDING];

	switch (reason) {
		// faccio solo una chiamata a server_writeable perchè invio solo una volta i valori
		case LWS_CALLBACK_ESTABLISHED:// just log message that someone is connecting
		fprintf(stderr,"connection established for spie_bobine\n");
		libwebsocket_callback_on_writable(this,wsi);// chiede la disponibilità a scrivere sul socket
		break;

		case LWS_CALLBACK_SERVER_WRITEABLE:// se c'è un socket aperto e questo è disponibile, invio la lista delle spie e bobine
		n = sprintf((char *)p,spie_bobine);
		m = libwebsocket_write(wsi, p, n, LWS_WRITE_TEXT);
		fprintf(stderr,"constants spie and bobine sent\n");
		if (m < n) {
		  lwsl_err("ERROR %d writing to di socket spie_bobine (returned %d)\n", n,m);
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
		printf("connection established for TOTP\n");

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

/*============================MAIN==============================================*/
int main(void) {

	modbus_t *mb;
	uint16_t tab_reg[15]; // max 15 reg. in realtà ne servono 12

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
}
;
// create libwebsocket context representing this server
context = libwebsocket_create_context(&context_info);

if (context == NULL) {
	fprintf(stderr, "libwebsocket init failed\n");
	return -1;
}

printf("starting server...\n");
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

		if (modbus_read_registers(mb, 65, 12, tab_reg) > 0) {

		  // VALORI DIGITALI (PLC + OTB). Questi valori sono usati nelle callback 
		  io1=tab_reg[0];
		  io2=tab_reg[1];
		  OTBDIN=tab_reg[9]; // ingressi digitali OTB che il PLC ha passato al PC nella posizione 9 di tab_reg[].

		  // VALORI ANALOGICI (PM9 + OTB)
		  I=(float)(tab_reg[4]+(tab_reg[3]<<16))/1000; // PM9        
		  V=(float)(tab_reg[6]+(tab_reg[5]<<16))/1000; // PM9        
		  P=(float)(tab_reg[8]+(tab_reg[7]<<16))/100;  // PM9        
		  Bar=(float)(tab_reg[10]*0.002442); // INPUT ANALOGICO OTB
		  Bar_pozzo=(float)(tab_reg[11]*0.002442); // INPUT ANALOGICO OTB
		  
		} else {
			printf("ERR: modbus read registers..riprovo a creare il context\n");
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
