#ifndef CLIENT_HPP_INCLUDED
#define CLIENT_HPP_INCLUDED

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <fcntl.h>
#include <cerrno>

class Client
{
public:
    Client(void);
    ~Client(void);
    int init(void);
    int sendToSrv(void);
    int parseInput(char *buf, int len);
    int sendfile(char fileName[]);    

private:
    int sock;
    struct sockaddr_in srvAddr;

};
#endif // CLIENT_HPP_INCLUDED
