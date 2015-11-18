#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <libconfig.h>
#include "conf.h"

int main() {










  config_t config;
  
  config_init(&config);
  if (config_read_file(&config, "./sb.conf")!=CONFIG_TRUE) {
    printf("Errore in  config file\n");
  }
  printf("%s\n",spiebobine);
  config_destroy(&config);
  return 0;








}
