#include <modbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int main() {
  modbus_t *mb;
  uint8_t *b=NULL;
  int nbits=1;
  
  b=(uint8_t *)malloc(nbits*sizeof(uint8_t));
  memset(b, 0, nbits * sizeof(uint8_t));
  mb = modbus_new_tcp("192.168.1.157", 502);
  modbus_connect(mb);
  
  /* Read 5 registers from the address 0 */
  
  modbus_read_bits(mb, 7, 1, b);
  if (b[0]==TRUE) {printf("ON\n");} else {printf("OFF\n");};
  printf("bit=%0X\n",b[0]);
  //  fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));

  free(b);
  modbus_close(mb);
  modbus_free(mb);
  return 1;
}
