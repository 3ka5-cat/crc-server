#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <time.h>

#include "checksum.h"

#define sQUIT "q"

typedef struct {
		unsigned long CRC;		
		int ffd;
		char file_name[256];
		short have_file_name;
		unsigned long length;
	} file;
	
typedef struct {
		int fd;
		file files[2];	
		int check;	
	} client;

int total_clients = 0;
client *clients = NULL;	
