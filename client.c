#include "client.h"

#define MAX_BUFFER_LEN 512

#define sQUIT "q"
#define sPRINT "pr"
#define sRESEND "res"

int sock;

int main(int argc, char *argv[])
{	
	char *serverAddress;
	long port;
	fprintf(stdout, "CRC32 Client by 3ka5_cat\n");
	
	if (argc == 4) {		
		char *r;
		char *addy[5];
		
		addy[0] = addy[1] = addy[2] = addy[3] /*= addy[4]*/ = NULL;
		addy[0] = r = argv[1];
		serverAddress = (char*)malloc(strlen(argv[1] + 1));
		strcpy(serverAddress, argv[1]);
		
		int i=0;

		while(*r) {
			if ((*r == '.' /*|| *r == ':'*/ ) && ++i < 4) {
				*r = '\0';
				addy[i] = r + 1;
			}
			else if (!isdigit((int) (unsigned char) *r)) {			
					fprintf(stderr, "Invalid character in Server IP address\n");
					free(serverAddress);
					exit(EXIT_FAILURE);
				}
			*r++;
		}
		if (i != 3) {
			fprintf(stderr, "Invalid Server IP address\n");
			free(serverAddress);
			exit(EXIT_FAILURE);
		}	
		
		for (i = 0; i < 4; i++) {
			int num = atoi(addy[i]);
			if (num < 0 || num > 255) {
				fprintf(stderr, "Invalid Server IP address\n");
				free(serverAddress);
				exit(EXIT_FAILURE);
			}			
		}
		
		r = argv[2];
		while(*r) {			
			if (!isdigit((int) (unsigned char) *r)) {			
					fprintf(stderr, "Invalid character in Server port\n");
					free(serverAddress);
					exit(EXIT_FAILURE);					
				}
			*r++;
		}
		port = strtol(argv[2], NULL, 10);
		if (port < 1024 || port > 65535) {
			fprintf(stderr, "Invalid port\n");
			free(serverAddress);
			exit(EXIT_FAILURE);
		}
					
		//printf("%s", serverAddress);
	}	
	else {
		fprintf(stderr, "Usage: %s ServerIPAdress port fileName\n", argv[0]);
		exit(EXIT_FAILURE);
	}
		
	if (init(serverAddress, port) == -1)
		exit(EXIT_FAILURE);
	
	send_file(argv[3]);
    
	while(1) {
		fprintf(stdout, "~:");		
		char buf[MAX_BUFFER_LEN];	
		memset(buf, 0, MAX_BUFFER_LEN);	
		fgets(buf, MAX_BUFFER_LEN, stdin);		
		
		buf[strlen(buf) - 1] = 0;

		if (!strcmp(buf, sQUIT))
			break;

		if (parseInput(buf, strlen(buf) + 1) != 0)
			break;		
	}
    	
	close(sock);
    return 0;
}

int init(char *serverAddress, long port) 
{
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0) {
        perror("socket");
        free(serverAddress);
        return -1;
    }

	struct sockaddr_in srvAddr;
    srvAddr.sin_family = AF_INET;
    srvAddr.sin_port = htons((short)port);
    inet_pton(AF_INET, serverAddress, &srvAddr.sin_addr);
    if(connect(sock, (struct sockaddr *)&srvAddr, sizeof(srvAddr)) < 0) {
        perror("connect");
        free(serverAddress);
        return -1;
    }
    free(serverAddress);
    return 0;
}

int parseInput(char * buf, int len)
{
	int count = 0;
	int i;
	for (i = 0; i < len; i++) {
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
	
	if (!strcmp(argv[0], sPRINT)) {
		if (j == 1) {
			char command[257];		
			sprintf(command, "%s", sPRINT);
			if (send_command(command) != -1) {
				if (receive_info(buf) != -1)
					fprintf(stdout, "server: %s\n", buf);			
			}
		}
		else
			fprintf(stderr, "Invalid command. Usage: pr\n");
	}
	else if (!strcmp(argv[0], sRESEND)) {
		if (j == 2)
			send_file(argv[1]);
		else
			fprintf(stderr, "Invalid command. Usage: res fileName\n");	
	}	
	else {
		fprintf(stderr, "Unknown command\n");		
	}
	
	for (i = 0; i < j; i++)
		free(argv[i]);
	free(argv);
	
	return 0;
}

int send_command(char* command)
{
	int bytesSend = send(sock, command, strlen(command) + 1, 0);
	if (bytesSend == -1) {
		perror("send");		
	}
	return bytesSend;	
}

int receive_info(char* buf)
{
	int bytesRecv = 0;
	memset(buf, 0, MAX_BUFFER_LEN);
	int total = 0;
	int done = 0;
	while ((bytesRecv = recv(sock, buf + total, MAX_BUFFER_LEN - total, 0)) > 0) {
		int i;
		for (i=total; i < total + bytesRecv; i++ ) {				
			if ((buf[i] == 0xD) && (buf[i+1] == 0xA)) {					
				buf[i] = 0;
				done = 1;			
				break;
			}				
		}			
		if (done)
			break;
		total += bytesRecv;			
	}
			
	if (bytesRecv == -1) {
		perror("recv");		
	}	
	return bytesRecv;
}

int send_file(char *fileName)
{
	char buf[512];
    struct stat stat_buf;

    int ffd = open(fileName, O_RDONLY);
    if (ffd == -1) {
        perror("open");
        return -1;
    }    
    if (fstat(ffd, &stat_buf) == -1) {
          perror("fstat");
          return -1;
    }	
    ssize_t len = stat_buf.st_size;
    printf("len = %d\n", len);

	memset(buf, 0, 257);
    buf[0] = 'n';
    if (strlen(fileName) > 255) {
		fprintf(stderr, "Too big filename\n");
		return -1;
	}
	else {
		sprintf(buf+1,"%s",fileName);
		if (send_command(buf) != -1)
			fprintf(stdout, "name send\n");	
		else
			return -1;
	}
	
    memset(buf, 0, 257);
    buf[0] = 'l';
    sprintf(buf+1,"%d", len);
    if (send_command(buf) != -1)
		fprintf(stdout, "len send\n");	
	else
		return -1;
           
    while (len > 0) {		
        ssize_t count = read(ffd, buf, MAX_BUFFER_LEN);
        ssize_t res = write(sock, buf, count);
        if (res == -1)
            if (errno != EINTR) {
                perror("send file");
                close(ffd);
                return -1;
            }
        len -= count;
    }
    close(ffd);
    return 0;
}
