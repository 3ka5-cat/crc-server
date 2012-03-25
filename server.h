#ifndef SERVER_H
#define SERVER_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "checksum.h"

#define sQUIT "q"

#define NUM_OF_FILES 5

typedef struct {
		//int error;
		unsigned long CRC;		
		int ffd;
		char file_name[256];
		short have_file_name;
		unsigned long length;
	} file;
	
typedef struct {
		int fd;
		file files[NUM_OF_FILES];
		int total_files;
		int check;	
	} client;

int total_clients = 0;
client *clients = NULL;	

#endif //SERVER_H
