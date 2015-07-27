#include <stdio.h>
#include <stdlib.h>
#include <libwebsockets.h>
#include <math.h>
#include <modbus.h>


static int callback_http(struct libwebsocket_context * this,
                         struct libwebsocket *wsi,
                         enum libwebsocket_callback_reasons reason, void *user,
                         void *in, size_t len)
{
  return 0;
}

/* holds random int between 0 and 19 */
static volatile float V,I,P,Bar,Bar_pozzo;;

static int callback_energy(struct libwebsocket_context * this,
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
    printf("connection established\n");
    break;
    
  case LWS_CALLBACK_SERVER_WRITEABLE:
    //n = sprintf((char *)p, "%3.1f", V);
    // n = sprintf(payload, "{Energia:{V:%f,I:%f,P:%f},IO:%ui,Bar:%f,Bar_pozzo:%f}", V,I,P,IO,Bar,Bar_pozzo);
    //{"Energia":{ "V":217.9,"I":3.0,"P":510},"Bar":3.07,"Bar_pozzo":2.97}

    n = sprintf((char *)p,"{\"Energia\":{ \"V\":%3.1f,\"I\":%2.1f,\"P\":%1.2f},\"Bar\":%2.2f,\"Bar_pozzo\":%2.2f}",V,I,P,Bar,Bar_pozzo);
    //    n = sprintf((char *)p, "{Energia:{V:%3.1f,I:%2.1f,P:%4.0f},Bar:%2.2f,Bar_pozzo=%2.2f}", V,I,P,Bar,Bar_pozzo);
    m = libwebsocket_write(wsi, p, n, LWS_WRITE_TEXT);
    if (m < n) {
      lwsl_err("ERROR %d writing to di socket (returned %d)\n", n,m);
      return -1;
    }
    break;
    
  case LWS_CALLBACK_RECEIVE:  // the funny part
    fprintf(stderr, "rx %d\n", (int)len);
    break;
    
    default:
    break;
  }
    
  return 0;
}

enum protocols {
	/* always first */
	PROTOCOL_HTTP = 0,
	PROTOCOL_ENERGY,
	/* always last */
	DEMO_PROTOCOL_COUNT
};

static struct libwebsocket_protocols protocols[] = {
  /* first protocol must always be HTTP handler */
  {
    "http-only",   // name
    callback_http, // callback
    0              // per_session_data_size
  },
  {
    "energy", // protocol name - very important!
    callback_energy,   // callback
    0                  // we don't use any per session data
  },
  {
    NULL,
    NULL,
    0   /* End of list */
  }
};

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
  int port = 81;
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
  // infinite loop, to end this server send SIGTERM. (CTRL+C)
  while (n >= 0) {
    struct timeval tv;
    /* random int between 0 and 19 */
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
      //      printf("V=%f\n%d - %d [%d]\n", V,tab_reg[5]<<16,tab_reg[6],(tab_reg[5]<<16)+tab_reg[6]  );
	V=(float)(tab_reg[6]+(tab_reg[5]<<16))/1000;
	I=(float)(tab_reg[4]+(tab_reg[3]<<16))/1000;
	P=(float)(tab_reg[8]+(tab_reg[7]<<16))/100;
	Bar=(float)(tab_reg[10]*0.002442);;
	Bar_pozzo=(float)(tab_reg[11]*0.002442);
	libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_ENERGY]);
      } else {
	printf("ERR: modbus read registers..\n");
      }
      oldms = ms;
    }
    n = libwebsocket_service(context, 50);
  }
  

  modbus_close(mb);
  modbus_free(mb);
  
  libwebsocket_context_destroy(context);
  lwsl_notice("libwebsockets-test-server exited cleanly\n");
  return 0;
}
