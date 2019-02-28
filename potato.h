#ifndef __POTATO_H__
#define __POTATO_H__

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>

typedef struct _potato_t{
  int num_hops;
  char IDlist[1024];
}potato_t;

#endif
