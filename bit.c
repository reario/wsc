
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>


void usage() {
printf("use: operate [on|off|invert|read]\n");
}

uint16_t read_state(uint16_t reg, uint16_t q) {
/*legge q-esomo bit di reg*/
return (reg & (1<<q));
}

uint16_t read_single_state(uint16_t reg, uint16_t q) {
  /*legge q-esomo bit di reg*/
  if (q<16) {
    uint16_t i;
    i=(1<<q); /* 2^q */
    if (reg & i) {return 1;} else {return 0;};
  } else { 
    return -1; 
  }
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
}


int main(void) {
  printbitssimple((uint16_t)7);
  printf("\n");
  printbitssimple(invert_state((uint16_t)7,(uint16_t)3));
  printf("\n");
  printf("%d\n",read_state((uint16_t)7,(uint16_t)1));
  
  printf("%d\n",read_single_state((uint16_t)7,(uint16_t)6)) ;
  printf("\n");
  return 0;
}
