#include "server.h"

/*==============================ATTIVAZIONI===========================================================*/
uint16_t attiva(modbus_t *m, char *t, double registro, double bit) {
  // NOTA: le attivazioni non coinvolgono il PLC-PC da cui il programma acquisisce i dati
  // Tutte le operazioni modbus di read e write in questa funzione sono fatte usando 
  // gli IP dei singoli dispositivi remoti (OTB e TWIDO)

  /*
    Questa funzione Attiva i relè dei dispositivi remoti identificati dentro il parametro (modbus *m).
    I relè possono essere singole Coil come per esempio %M10 del PLC, o singoli bit dentro una word del
    PLC o di un OTB (per esempio %MW).
    Il numero del relè è passato nel parametro (double bit).
    Per un relè di tipo Coil, il valore del parametro (double registro) è posto a -1.

    Esempio:
    registro = 100
    bit=19
    Identifica il bit 19 del registro 100

    registro = -1
    bit 61
    Identifica il Coil 61
    
    I Coil sono settabili direttamente dalla chiamata di libreria (modbus_write_bit), mentre i 
    relè nei registri prevedono la lettura dell'intera word, la modifica del valore e quindi la 
    sua successiva scrittura. Quindi due operazioni (modbus_read_registers() e modbus_read_register() ).

    Ogni relè (sia esso Coil o bit di una word) può essere azionato come monostabile (pulsante) o bistabile 
    (interruttore). In funzionamento monostabile, cioè come ad esempio un pulsante, il relè viene 
    settato ad 1 per 1 secondo, e quindi rimesso a 0. Per il funzionamento bistabile, cioè come interruttore,
    lo stato viene invertito (se era 1 è messo a 0 e viceversa).
    
    Il tipo di funzionamento desiderato è impostato nel valore del parametro (char *t) che vale "P" per i
    pulsanti e "I" per gli interruttori.

    I tipi double dei parametri vengono dal parser JSON che usa questo tipo per i numeri.

   */



  /* per la conversione double-->int vedi:  http://c-faq.com/fp/round.html C FAQ num 14.6*/
  int b=(int)(bit+0.5);
  int r=(int)(registro +0.5);

  ///////////////////////////////////////////////////////
  /* servono per leggere lo stato del bit remoto */
  uint8_t *bits=NULL;
  bits=(uint8_t *)malloc(1*sizeof(uint8_t));
  memset(bits, 0, 1 * sizeof(uint8_t));
  /***********************************************/


  ///////////////////////////////////////////////////////
  /* serve per leggere il registrot remoto */
  uint16_t regs[1];
  /***********************************************/


  //////////////// BOBINE Coil///////////////////////////////
  ///////////////////////////////////////////////////////
  // Si richiede di aggiornare una Bobina come Pulsante
  if ( (registro<0) && (strncmp(t,"P",(size_t)1)==0)) { 
    printf("attivo la bobina %i come pulsante (%s)\n",b,t); 
    // la metto a ON
    if (modbus_write_bit(m, b, TRUE) != 1) {
      lwsl_err("ERRORE DI SCRITTURA:PULSANTE ON\n");
      return -1;
    }
    sleep(1);
    // la metto a OFF
    if (modbus_write_bit(m, b, FALSE) != 1) {
      lwsl_err("ERRORE DI SCRITTURA:PULSANTE OFF\n");
      return -1;
    }
    return 1; // ho finito di attivare la bobina come pulsante
  }
  //////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////
  // Si richiede di aggiornare una Bobina come Interruttore
  // La funzione si ottiene invertendo lo stato della bobina
  // prima leggo lo stato. Poi lo inverto.
  // a differenza della funzione pulsante che devo in ogni caso passare per una attivazione 
  // (pulsante premuto, vedi valore TRUE nel caso sopra) e quindi una disattivazione
  // (pulsante rilasciato, vedi valore FALSE nel caso sopra).)
    if ( (registro<0) && (strncmp(t,"I",(size_t)1)==0)) {    
      // leggo i bit remoto il cui valore si trova in bits[0]
      printf("attivo la bobina %i come interruttore (%s)\n",b,t);
      if (modbus_read_bits(m, b, 1, bits) >0 ) {
	// bits[0] è il byte che contiene true o false (0000001 o 000000000)
	// vedi documentazione di modbus_read_bits
	bits[0]^=1<<0; // l'operazione inverte lo stato
	
	if (modbus_write_bit(m,b,bits[0]) > 0) {
	  return 1;
	} else {
	  lwsl_err("ERRORE DURANTE LA SCRITTURA DELLO STATO DELL'INTERRUTTORE\n");  
	  return -1; 
	}
      } else { 
	lwsl_err("ERRORE DURANTE LA LETTURA DELLO STATO DELL'INTERRUTTORE\n");
	return -1; 
      }
    }  
  //////////////////////////////////////////////////////////


  //////////////// BOBINE CONTENUTE IN UN REGISTRO (P.E. OTB) ////////////
  ////////////////////////////////////////////////////////////////////////
  // Si richiede di aggiornare una Bobina nel registro come Pulsante
  if ( (registro>=0) && (strncmp(t,"P",(size_t)1)==0)) 
    {
      // leggo il registro
      printf("1.1 Leggo il registro\n");
      if (modbus_read_registers(m,r,1,regs)>0) 
	{
	  printf("2.1 Metto ad on il suo bit\n");
	  // Metto a ON
	  //if ( (regs[0] & (1<<b)) == 0) // non è a zero
	  regs[0]|=(1<<b); 
	  // Lo riscrivo. Per compattezza controllo se è andato male
	  printf("3.1 Riscrivo il registro\n");
	  if (modbus_write_register(m,r,regs[0])<0) 
	    {
	      lwsl_err("ERRORE DI SCRITTURA DEL REGISTRO NELLA FASE ON: PULSANTE\n");
	      return -1;
	    }	  
	} // read register
      else 
	{ 
	  lwsl_err("ERRORE DI LETTURA DEL REGISTRO NELLA FASE ON: PULSANTE\n");
	  return -1;
	}
      // pausa di 1 secondo
      printf("---- Pausa...1 sec ----\n");
      sleep(1);
      // ripeto come sopra ma mettendo a OFF il bit del registro
      printf("2.1 Leggo il registro\n");
      if (modbus_read_registers(m,r,1,regs)>0) 
	{
	  printf("2.2 Metto ad off il suo bit\n");
	  // Metto a OFF
	  regs[0]&=~(1<<b);
	  // Lo riscrivo. Per compattezza controllo se è andato male
	  printf("3.2 Riscrivo il registro\n");
	  if (modbus_write_register(m,r,regs[0])<0) 
	    {
	      lwsl_err("ERRORE DI SCRITTURA DEL REGISTRO NELLA FASE OFF: PULSANTE\n");
	      return -1;
	    } 
	}
      else 
	{
	  lwsl_err("ERRORE DI LETTURA DEL REGISTRO NELLA FASE OFF: PULSANTE\n");
	  return -1;
	}
      return 1; // ho finito di attivare la bobina del registro come pulsante
    }
  
  ///////////////////////////////////////////////////////
  // Si richiede di aggiornare una Bobina nel registro come INTERRUTTORE
  if ( (registro>=0) && (strncmp(t,"I",(size_t)1)==0)) { 
 
    // leggo i registro remoto il cui valore si trova in regs[0]
    printf("cambio lo stato della bobina nel registro \n");
    if (modbus_read_registers(m, r, 1, regs) >0 ) {
      regs[0]^=1<<b; // regs[0] è il byte che contiene true o false (0000001 o 000000000)
      if (modbus_write_register(m,r,regs[0]) > 0) {
	return 1;
      } else { 
	lwsl_err("ERRORE DI SCRITTURA DEL REGISTRO DURANTE LE OPERAZIONI DI INTERRUTTORE DELLA BOBINA NEL REGISTRO\n");
	return -1; }
    } else 
      {
	lwsl_err("ERRORE DI LETTURA DEL REGISTRO DURANTE LE OPERAZIONI DI INTERRUTTORE PER UNA BOBINA NEL REGISTRO\n");
	return -1; 
      }

    return 1; // ho finito di attivare la bobina del registro come interruttore
  }  

  return 0; // you should never be here!!!
}
