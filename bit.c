#include "server.h"

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

void printbitssimple(uint16_t n) {
  /*dato l'intero n stampa la rappresentazione binaria*/
  unsigned int i;
  i = 1<<(sizeof(n) * 8 - 1); /* 2^n */
  while (i > 0) {
    if (n & i)
      printf("1");
    else
      printf("0");
    i >>= 1;
  }
  printf("\n");
}


uint64_t set_state64(uint64_t reg, uint16_t q) {
  /*set del bit q-esimo*/
  return reg | ((uint64_t)1<<q);
}

uint64_t reset_state64(uint64_t reg, uint16_t q) {
  /*azzera il q-esimo bit*/
  return reg & ~((uint64_t)1<<q);
}

uint64_t invert_state64(uint64_t reg, uint16_t q) {
  /*inverte il q-esimo bit*/
  return reg^((uint64_t)1<<q);
}

uint16_t set_state(uint16_t reg, uint16_t q) {
  /*set del bit q-esimo*/
  return reg | (1<<q);
}

uint16_t reset_state(uint16_t reg, uint16_t q) {
  /*azzera il q-esimo bit*/
  return reg & ~(1<<q);
}

uint16_t invert_state(uint16_t reg, uint16_t q) {
  /*inverte il q-esimo bit*/
  return reg^(1<<q);
}


uint64_t place64(uint64_t dest, uint16_t source, uint16_t pos){

  /* places the string of bit represented by source in position pos (starting from right) into dest */
 
  uint64_t temp=0;

  temp=temp|source;
  temp=temp<<pos;
  dest=dest|temp;
  return dest;

}
