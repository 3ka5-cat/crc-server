#ifndef CLIENT_H
#define CLIENT_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int init(char* serverAddress, long port);
int parseInput(char * buf, int len);
int send_file(char *fileName);
int send_command(char* command);
int receive_info(char* buf);

#endif // CLIENT_H
