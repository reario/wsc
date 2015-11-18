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
*/







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
