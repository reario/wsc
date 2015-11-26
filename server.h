#include <modbus.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>


static volatile float V, I, P, Bar, Bar_pozzo;
static volatile uint16_t in1; /*prima parte dei 16 input, quelli da 0 a 15 : registro 65 sul plc master*/
static volatile uint16_t in2; /*prima parte dei 16 input, quelli da 16 a 23 : registro 66 sul plc master*/
static volatile uint16_t in3; /*8 ingressi digitali sul modulo di espansione del PLC*/
static volatile uint16_t otb_din; /* ingressi digitali dell'OTB */
static volatile uint16_t plc_dout; /*  OUT digitali del PLC */
static volatile uint16_t otb_dout; /*  OUT digitali dell'OTB */
static volatile uint64_t inlong=0; /* contiene fino a 64 ingressi digitali concatenazione di in1 in2, in3 e otb_din */




uint16_t attiva(modbus_t *m, char *t, double registro, double bit);













































/*
char spie_bobine[]= "{"spie":{
    "AUTOCLAVE":"Autoclave",
      "POMPA_SOMMERSA":"Pozzo",
      "RIEMPIMENTO":"Serbatoio",
      "LUCI_ESTERNE_SOTTO":"Luci Esterne",
      "CENTR_R8":"R8",
      "LUCI_GARAGE_DA_4":"Garage 4",
      "LUCI_GARAGE_DA_2":"Garage 2",
      "LUCI_TAVERNA_1_di_2":"Taverna 1",
      "LUCI_TAVERNA_2_di_2":"Taverna 2",
      "INTERNET":"Internet",
      "C9912":"C9912",
      "LUCI_CUN_LUN":"Cun.Lungo",
      "LUCI_CUN_COR":"Cun.Corto",
      "LUCI_STUDIO_SOTTO":"Studio",
      "LUCI_ANDRONE_SCALE":"Scale",
      "GENERALE_AUTOCLAVE":"Generale Autoclave",
      "LUCI_CANTINETTA":"Cantinetta"},
    "bobine":{
      "LUCI_ESTERNE_SOTTO":2,
	"LUCI_CUN_LUN":3,
	"LUCI_CUN_COR":4,
	"LUCI_TAVERNA":5,
	"LUCI_GARAGE":6,
	"LUCI_STUDIO_SOTTO":7,
	"LUCI_ANDRONE_SCALE":8,
	"LUCI_CANTINETTA":10,
	"SERRATURA_PORTONE":12,
	"APERTURA_PARZIALE":96,
	"APERTURA_TOTALE" :97,
	"CICALINO_AUTOCLAVE": 60,
	"CICALINO_POMPA_POZZO": 61

	}
}";


struct spie_bobine {
  char  ID[50]; // etichetta
  char  IP[32]; // il modbus slave
  char  type[8]; // se è un pulsante (monostabile) o un interruttore (bistabile)
  int rele; // la bobina del PLC o Ingresso dell'OTB. Se valore negativo, la spia non ha associata bobina direttamente comandabile dall'utente
} spe_bobine;

struct spie_bobine sp[64];





char spie_bobine[]="{\
\"spie\":{\
\"AUTOCLAVE\":\"Autoclave\",\
\"POMPA_SOMMERSA\":\"Pozzo\",\
\"RIEMPIMENTO\":\"Serbatoio\",\
\"LUCI_ESTERNE_SOTTO\":\"Luci Esterne\",\
\"CENTR_R8\":\"R8\",\
\"LUCI_GARAGE_DA_4\":\"Garage 4\",\
\"LUCI_GARAGE_DA_2\":\"Garage 2\",\
\"LUCI_TAVERNA_1_di_2\":\"Taverna 1\",\
\"LUCI_TAVERNA_2_di_2\":\"Taverna 2\",\
\"INTERNET\":\"Internet\",\
\"C9912\":\"C9912\",\
\"LUCI_CUN_LUN\":\"Cun.Lungo\",\
\"LUCI_CUN_COR\":\"Cun.Corto\",\
\"LUCI_STUDIO_SOTTO\":\"Studio\",\
\"LUCI_ANDRONE_SCALE\":\"Scale\",\
\"GENERALE_AUTOCLAVE\":\"Generale Autoclave\",\
\"LUCI_CANTINETTA\":\"Cantinetta\",\
\"FARETTI\":\"Faretti\"},\
\
\"bobine\":{\
\"LUCI_ESTERNE_SOTTO\":2,\
\"LUCI_CUN_LUN\":3,\
\"LUCI_CUN_COR\":4,\
\"LUCI_TAVERNA\":5,\
\"LUCI_GARAGE\":6,\
\"LUCI_STUDIO_SOTTO\":7,\
\"LUCI_ANDRONE_SCALE\":8,\
\"LUCI_CANTINETTA\":10,\
\"SERRATURA_PORTONE\":12,\
\"APERTURA_PARZIALE\":96,\
\"APERTURA_TOTALE\":97,\
\"CICALINO_AUTOCLAVE\":60,\
\"CICALINO_POMPA_POZZO\":61,\
\"FARETTI\":10}}";
*/
