#include "server.h"


char *gh_current;
JsonNode *node=NULL /* contiene TUTTO il JSON */;
JsonNode *SoBs=NULL; /* la parte JSON per Spie o Bobine */
JsonNode *Energia=NULL;
JsonNode *E=NULL; // generico elemento

/*
   Questa è la callback che invia ripetutamente al client le info dei sensori e delle
   spie e/o bobine.
*/

int callback_energy(struct libwebsocket_context * this,
		struct libwebsocket *wsi,
		enum libwebsocket_callback_reasons reason,
		void *user,
		void *in,
		size_t len)

{
  int n=0;
  modbus_t *mbw;
  JsonNode *Elem,*S;
  int gh_current_len;
  int tosent;
  struct per_session_data_fraggle *pse = user; //per session energy
  int write_mode=LWS_WRITE_CONTINUATION;
  // char PLC_IP[] = "192.168.1.157";
  //	char OTB_IP[] = "192.168.1.11";
  
  unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING+16000+LWS_SEND_BUFFER_POST_PADDING];
  unsigned char *p = &buf[LWS_SEND_BUFFER_PRE_PADDING];
  
  switch (reason) {
    
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
      } else {/* è un array: il primo valore è il registro,il secondo il bit del registro da acc/spe */
	reg=json_find_member(Elem,"rele")->children.head->number_;
	bobina=json_find_member(Elem,"rele")->children.tail->number_;	      
      }
      /* viene chiamata quando si clicca su una bobina */
      mbw = modbus_new_tcp(ip, 502);
      if (modbus_connect(mbw) == -1) {
	fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
	modbus_free(mbw);
	return -1;
      }
      //pulsante(mbw,(int)json_find_member(Elem,"rele")->number_);
      printf("registro=%lf - bobina=%lf\n",reg,bobina);
      attiva(mbw,type,reg,bobina);
      modbus_close(mbw);
      modbus_free(mbw);
      //libwebsocket_callback_on_writable(this,wsi); // richiedi la disponibilità a scrivere  
    }
    else {
      printf("Ricevuto un comando non riconosciuto");
    }
    
    break;

  case LWS_CALLBACK_ESTABLISHED: // just log message that someone is connecting
    fprintf(stderr,"**** connection established for protocol*energy*\n");
    // comincia tutto il processo richiedendo la disponibilità a scrivere sul socket
    pse->state=START;
    libwebsocket_callback_on_writable(this,wsi);
    break;
    
  case LWS_CALLBACK_SERVER_WRITEABLE: 
    
    switch (pse->state) {
    case START:
      gh_current_len=strlen(gh_current); /* vien calcolato di volta in volta perchè contiene i valori correnti */
      pse->packets_left= (gh_current_len / BUFFER) + 1 ; // numero intero di frammenti kunghi BUFFER
      pse->leftover=(gh_current_len % BUFFER ); // sfrido 
      fprintf(stderr, "To be sent %d  fragments\n",pse->packets_left);	          
      write_mode = LWS_WRITE_TEXT;
      pse->state = IN_BETWEEN;
      pse->sum=0; 
      printf("------\n");
      /* fallthru */
      
    case IN_BETWEEN:   
      // faccio la write per aggiornare lo stato delle spie appena il socket è disponibile. 
      // In questo modo implemeto il push dei dati
      strncpy((char *)p,gh_current+pse->sum,BUFFER);
      pse->packets_left--;
      if (pse->packets_left) {
	write_mode |= LWS_WRITE_NO_FIN;
	tosent=BUFFER;
      } else {
	pse->state = END;
	tosent=pse->leftover;
      }
      n = libwebsocket_write(wsi, p, tosent, write_mode);
      pse->sum += n;
      
      //m = libwebsocket_write(wsi, p, n, LWS_WRITE_TEXT);
      if (n < 0) {
	return -1;
      }
      if (n<tosent) {
	lwsl_err("Partial write\n");
	return -1;
      }
      libwebsocket_callback_on_writable(this,wsi);
      break;
      
    case END:
      //
      usleep((useconds_t)50000); // 50 ms di pausa altrimenti il browser non gli sta dietro e si rallenta
      pse->state=START;
      libwebsocket_callback_on_writable(this,wsi);// richiedi di nuovo la disponibilità a scrivere
      break;
    }
    
  case LWS_CALLBACK_CLOSED:
    fprintf(stderr,"connection *CLOSED* for energy\n");
    break;
    
  default:
    break;
  }
  
  return 0;
}
