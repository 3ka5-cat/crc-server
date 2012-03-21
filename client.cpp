#include "client.hpp"

using namespace std;

#define MAX_BUFFER_LEN 512
short port = 3333;
char serverAddress[] = "127.0.0.1";

#define sQUIT "q"
#define sPRINT "pr"
#define sRESEND "res"

int main(int argc, char *argv[])
{	
    Client client;

    if (client.init())
        return 1;

    if (client.sendToSrv());
        return 1;

    return 0;
}

Client::Client(void)
{
}

Client::~Client(void)
{
    close(sock);
}

int Client::init(void)
{
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
        perror("socket");
        return 1;
    }

    srvAddr.sin_family = AF_INET;
    srvAddr.sin_port = htons(port);
    inet_pton(AF_INET, serverAddress, &srvAddr.sin_addr);
    if(connect(sock, (struct sockaddr *)&srvAddr, sizeof(srvAddr)) < 0)
    {
        perror("connect");
        return 1;
    }

    return 0;
}


int Client::parseInput(char * buf, int len)
{
	/*
	if (len > MAX_BUFFER_LEN) {		
		memset(buf, 0, MAX_BUFFER_LEN);
		return -1;
	}
	*/
	int count = 0;
	for (int i = 0; i < MAX_BUFFER_LEN; i++) {
		if (buf[i] == ' ')
			count++;
	}
	char **argv = NULL;	
	argv = (char **)malloc(sizeof(char*) * (count + 1));
	if (argv == NULL) {
		fprintf(stderr, "Failed to parse input: out of memory\n");
		return -1;					
	}
	
	char *tmp;
	int j = 0;
	tmp = strtok(buf ," ");
	while (tmp != NULL) {
		*(argv + j) = (char *)malloc(strlen(tmp) + 1);
		if (*(argv + j) == NULL) {
			fprintf(stderr, "Failed to parse input: out of memory\n");
			return -1;					
		}
		strncpy (*(argv + j), tmp, strlen(tmp));
		*(*(argv + j) + strlen(tmp)) = 0;
		j++;
		tmp = strtok(NULL, " ");
	}
	
	if (!strcmp(buf, sPRINT)) {
		//cout << "command: " << sPRINT << endl;
		char command[40];		
		sprintf(command, "%s", sPRINT);


		int bytesSend = send(sock, command, strlen(command) + 1, 0);
		if (bytesSend == -1) {
			perror("send");
			return 1;
		}
		
		bytesSend = 0;
		memset(buf, 0, MAX_BUFFER_LEN);
		int total = 0;
		int done = 0;
		while ((bytesSend = recv(sock, buf + total, MAX_BUFFER_LEN - total, 0)) > 0) {
			for (int i=total; i < total + bytesSend; i++ ) {				
				if ((buf[i] == 0xD) && (buf[i+1] == 0xA)) {					
					buf[i] = 0;
					done = 1;			
					break;
				}				
			}			
			if (done)
				break;
			total += bytesSend;			
		}		
		if (bytesSend == -1) {
			perror("recv");
			return 1;
		}		
		fprintf(stdout, "server: %s\n", buf);		
	}
	else if (!strcmp(buf, sRESEND)) {
		sendfile("npp1.exe");
	}	
	else {
		cout << "Unknown command" << endl;
	}	
	
	return 0;
}

int Client::sendfile(char fileName[])
{
	int bytesSend = 0;
    struct stat stat_buf;

    int ffd = open(fileName, O_RDONLY);
    if (ffd == -1) {
        perror("open");
        return 1;
    }
    
    if (fstat(ffd, &stat_buf) == -1) {
          perror("fstat");
          return 1;
    }
	close(ffd);
    ssize_t len = stat_buf.st_size;

	char mes[40];
    mes[0] = 'n';
    sprintf(mes+1,"%s","npp1.exe");

	cout << "name send" << endl;
    bytesSend = send(sock, mes, strlen(mes) + 1, 0);
    if (bytesSend == -1) {
        perror("send");
        return 1;
    }  
    
    memset(mes,0, 40);
    mes[0] = 'l';
    sprintf(mes+1,"%d", len);
    cout << "len send" << endl;
    bytesSend = send(sock, mes, strlen(mes) + 1, 0);
    if (bytesSend == -1) {
        perror("send");
        return 1;
    }
    
    FILE* fp = fopen(fileName, "r");    
    while (len > 0) {
		char buf[512];
		ssize_t res = 0;
        ssize_t count = fread(buf, sizeof(char), 512, fp);
        res = send(sock, buf, count, 0);
        if (res == -1)
            if (errno != EINTR) {
                perror("send2");
                return 1;
            }
        len -= count;
        /*
        cout << "count: "<< count << endl;
        cout << "res: " << res << endl;
        cout << "len: " << len << endl << endl;
        */
    }
    fclose(fp);    
}

int Client::sendToSrv(void)
{	
    sendfile("npp1.exe");
    
    while(1) {
		cout << "~: ";
		char buf[MAX_BUFFER_LEN];
		cin.getline(buf, MAX_BUFFER_LEN);

		if (!strcmp(buf, sQUIT))
			break;

		if (parseInput(buf, sizeof(buf)) != 0)
			break;		
	}
    return 0;
}
