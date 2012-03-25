#include "server.h"

static int create_and_bind(long port)
{
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int s, sfd;
	char portStr[8];

	memset(&hints, 0, sizeof (struct addrinfo));
	hints.ai_family = AF_INET;     
	hints.ai_socktype = SOCK_STREAM; 
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;    

	snprintf(portStr, sizeof(portStr), "%ld", port);
	s = getaddrinfo (NULL, portStr, &hints, &result);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		return -1;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1)
			continue;

		s = bind(sfd, rp->ai_addr, rp->ai_addrlen);
		if (s == 0) {
			/* We managed to bind successfully! */
			break;
		}
		close(sfd);
	}

	if (rp == NULL) {
		fprintf(stderr, "Failed to bind\n");
		return -1;
	}

	freeaddrinfo(result);

	return sfd;
}

static int make_socket_non_blocking(int sfd)
{
	int flags, s;

	flags = fcntl(sfd, F_GETFL, 0);
	if (flags == -1) {
		perror ("fcntl");
		return -1;
	}

	flags |= O_NONBLOCK;
	s = fcntl(sfd, F_SETFL, flags);
	if (s == -1) {
		perror("fcntl");
		return -1;
	}

	return 0;
}

int getCRC(client* cur_client, int ind)
{
	unsigned long ulCRC = 0;
	char mode[] = "0600";
	
	long md = strtol(mode, 0, 8);
	if (chmod(cur_client->files[ind].file_name, (int)md) < 0) {
		perror("Failed to chmod file\n");
		return -1;
	}
                    
	int res = fileCRC(cur_client->files[ind].file_name, &ulCRC);
	if(res == -1) {
		fprintf(stderr, "Failed to calculate the checksum\n");
		return -1;
	}
	else {
		fprintf(stdout, "checksum: %lX\n", ulCRC);
		cur_client->files[ind].CRC = ulCRC;
		return 0;
	}
}

int find_client(int fd)
{
	int i;
	for (i = 0; i < total_clients; i++) {
		if(clients[i].fd == fd)
			return i;			
	}
	return -1;	
}
/*
static void *
control_thread(void *arg)
{
	while(1) {		
		char buf[MAX_BUFFER_LEN];
		fgets(buf, MAX_BUFFER_LEN, stdin);

		if (!strcmp(buf, sQUIT)) {
			int res = pthread_cancel(thr);
			if (s != 0)
				handle_error_en(s, "pthread_cancel");
			break;		
		}
	}
	return 0;
}
*/
int add_new_client(int fd)
{
	int file_ind = 0;
	++total_clients;
	clients = (client*)realloc(clients,
					total_clients * sizeof(client));
	if (clients == NULL) {
		fprintf(stderr, "Failed to add client: out of memory\n");
		return -1;					
	}				
	clients[total_clients - 1].total_files = 0;
	clients[total_clients - 1].fd = fd;
	int i;
	for (i = 0; i < NUM_OF_FILES; i++) {
		clients[total_clients - 1].files[file_ind].have_file_name = 0;
		clients[total_clients - 1].files[file_ind].length = 0;
		clients[total_clients - 1].files[file_ind].ffd = -1;
		clients[total_clients - 1].files[file_ind].CRC = 0;
	}
	return 0;
}

void printTimeStamp(struct _IO_FILE * stream)
{
	time_t now;
	struct tm* tm;
	now = time(0);
	tm = localtime(&now);
	fprintf(stream, "<%d:%d:%d>", tm->tm_hour, tm->tm_min, tm->tm_sec);	
}

int receive_file(int ind, int file_ind, int existance, char* tmp_name)
{
	if (existance) {
		fprintf(stdout, "The file %s is exist... Rename\n", tmp_name);
		srand(time(NULL));
		// generate random seq
		int seq = rand() % 1000 + 1;
		char str_seq[5];
		sprintf(str_seq, "%d", seq);
		strncat(tmp_name, str_seq, 255 - strlen(tmp_name));										
		strncpy(clients[ind].files[file_ind].file_name, tmp_name, 255);
		clients[ind].files[file_ind].file_name[255] = 0;
	}
	else {
		fprintf(stdout, "The file %s doesn't exist\n", tmp_name);				
		clients[ind].files[file_ind].have_file_name = 1;
		strncpy(clients[ind].files[file_ind].file_name, tmp_name, 255);
		clients[ind].files[file_ind].file_name[255] = 0;			
	}
	clients[ind].files[file_ind].have_file_name = 1;
	fprintf(stdout, "[*] ");
	printTimeStamp(stdout);
	fprintf(stdout, " Receive file: %s from %d\n", 
		clients[ind].files[file_ind].file_name, clients[ind].fd);							
	clients[ind].files[file_ind].ffd = open(clients[ind].files[file_ind].file_name, O_WRONLY | O_CREAT);
	if (clients[ind].files[file_ind].ffd == -1) {
		perror("Failed to create file");
		return -1;
	}
	return 0;
}

int check_existance(char* tmp_name)
{				
	struct stat stat_buf;
	int res = stat(tmp_name, &stat_buf);
	return res;
}

int send_info(int ind, int file_ind, char* buf, ssize_t count)
{	
	buf[count] = 0xD;
	buf[count + 1] = 0xA;
	count = write(clients[ind].fd, buf, count + 2);	
	return count;
}

int worker(int ind)
{
	int done = 0;
	int disc = 0;
	int file_ind = clients[ind].total_files;
	char buf[512];	
	ssize_t count = 0;
				
	while (1) {
		memset(buf, 0, 512);					
		count = read(clients[ind].fd, buf, 
							sizeof buf);
		if (count == -1) {
		/* If errno == EAGAIN, that means we have read all
			data. */
			if (errno != EAGAIN) {
				perror ("read");
				done = 1;
			}
			break;
		}
		else if (count == 0) {
			/* End of file. The remote has closed the
			connection. */ 	
			fprintf(stderr, "[-] ");
			printTimeStamp(stderr);
			fprintf(stderr, " %d disconnected\n",  clients[ind].fd);			
			disc = 1;		
			break;
		}		
		else if (!strcmp(buf,"pr")) {
				fprintf(stdout, "[*] ");
				printTimeStamp(stdout);
				fprintf(stdout, " Print all checksums for %d\n", 
					clients[ind].fd);
				memset(buf, 0, 512);
				int i;
				int total = 0;
				for (i = 0; i < clients[ind].total_files; i++) {
					count = snprintf(buf + total, 510 - total, "%lX ", clients[ind].files[i].CRC);
					total += count;
				}
				count = send_info(ind, file_ind, buf, total);			
				if (count == -1) {
					perror("Failed to send request");
					break;
				}				
				break;
			}		
			
			else if (buf[0] == 'n' 
						&& !(clients[ind].files[file_ind].have_file_name)) {			
				char tmp_name[256];	
				strncpy(tmp_name, buf + 1, 255);				
				tmp_name[255] = 0;	
				int offs = strlen(tmp_name) + 2;
				int res = check_existance(tmp_name);
				if (res == -1 && errno == ENOENT) {
					if (receive_file(ind, file_ind, 0, tmp_name) == -1)
						break;				
				}
				else if (res == 0) {
					if (receive_file(ind, file_ind, 1, tmp_name) == -1)
						break;				
				}
				else {
					perror("Failed to get stat about file");
				}
										
				if (buf[offs] == 'l' && (clients[ind].files[file_ind].length == 0)) {
					unsigned long ul = -1;
					ul = strtoul (buf + offs + 1, NULL, 10);	
					printf("len = %lu\n", ul);					
					if (ul > 0)
						clients[ind].files[file_ind].length = ul;
					else {
						fprintf(stderr, "wrong size\n");
						break;
					}							
					for (++offs; offs < 512; offs++) {
						if (buf[offs] == 0) {
							offs++;									
							break;
						}									
					}
					if (offs < 512) {					
						count = write(clients[ind].files[file_ind].ffd, buf + offs, 512 - offs);				
						if (count == -1) {
							perror("Failed to write file");
							close(clients[ind].files[file_ind].ffd);
							clients[ind].files[file_ind].ffd = 0;
							break;
						}					
						clients[ind].files[file_ind].length -= count;
						if (clients[ind].files[file_ind].length == 0) {
							close(clients[ind].files[file_ind].ffd);
							clients[ind].files[file_ind].ffd = 0;
							done = 1;
							break;
						}								
					}
				}									
			}
			
			else if (buf[0] == 'l' 
						&& (clients[ind].files[file_ind].length == 0)) {
							unsigned long ul = -1;
							ul = strtoul (buf + 1, NULL, 10);	
							printf("len = %lu\n", ul);					
							if (ul > 0)
								clients[ind].files[file_ind].length = ul;
							else {
								fprintf(stderr, "wrong size\n");
								break;
							}
			}
			
			else if (clients[ind].files[file_ind].ffd) {			
				count = write(clients[ind].files[file_ind].ffd, buf, count);				
				if (count == -1) {
					perror("Failed to write file");
					close(clients[ind].files[file_ind].ffd);
					clients[ind].files[file_ind].ffd = 0;
					break;
				}
				clients[ind].files[file_ind].length -= count;
				if (clients[ind].files[file_ind].length == 0) {
					close(clients[ind].files[file_ind].ffd);
					clients[ind].files[file_ind].ffd = 0;
					done = 1;
					break;
				}						
			}
									
	}	
	if (disc) {
		close(clients[ind].fd);
		clients[ind].fd = 0;
		if (clients[ind].files[file_ind].ffd) {
			close(clients[ind].files[file_ind].ffd);
			clients[ind].files[file_ind].ffd = 0;
		}
			
		total_clients--;
				
		memmove(clients + ind, clients + ind + 1, 
				total_clients * sizeof(client));
				
		clients = (client*)realloc(clients,
					total_clients * sizeof(client));
		if (clients == NULL) {
			if (total_clients > 0) {
				fprintf(stderr, "Failed to free client\n");
				return -1;					
			}
			else {
				fprintf(stdout, "Empty clients\n");
			}
		}
										
	}	             			
	else if (done) {		
		if (getCRC(clients + ind, file_ind) == -1) {
			memset(buf, 0, 512);			
			count = snprintf(buf, 510, "Internal server error!");
			count = send_info(ind, file_ind, buf, count);									
			if (count == -1) {
				perror("Failed to send error message");				
			}				
		}
		clients[ind].total_files++;	
		return 0;
		/*
		fprintf(stdout, "Closed connection on descriptor %d\n",
				events[i].data.fd);

		// Closing the descriptor will make epoll remove it
		//	from the set of descriptors which are monitored.
		close(events[i].data.fd);
		*/
	}
}

int main(int argc, char *argv[])
{
	long port = 3333;
	int maxevents = 64;
	
	fprintf(stdout, "CRC32 Server by 3ka5_cat\n");
	
	if (argc == 1) {
		fprintf(stdout, "Empty args. Use standart port: %ld\n", port);
    }
	else if (argc == 2) {		
		port = strtol(argv[1], NULL, 10);
		if (port < 1024 || (port > 65535 )) {
				fprintf (stderr, "Wrong port: %ld\n", port);
				exit(EXIT_FAILURE);
			}
		else
			fprintf(stdout, "User-defined port: %ld\n", port);
	}
	else {
		fprintf(stderr, "Usage: %s [port]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	int res = 0;
	/*
	pthread_attr_t attr;
	res = pthread_attr_init(&attr);
    if (res != 0) {
        perror("control thread  attr init");
        abort();
	}
	
	pthread_t control_thread_id;
	res = pthread_create(&control_thread_id, &attr,
                           &control_thread, NULL);
	if (res != 0) {
		perror("control thread create");
		abort();
	}	
	fprintf(stdout, "Control thread created...\n");	
	*/
	
	int listener = create_and_bind(port);
	if (listener == -1)
		abort();
	fprintf(stdout, "Listen socket created...\n");
	res = make_socket_non_blocking(listener);
	if (res == -1) {		
		abort();
	}
	fprintf(stdout, "Listen socket initialised...\n");	
	res = listen(listener, SOMAXCONN);
	if (res == -1) {
		perror("listen");		
		abort();
    }
    fprintf(stdout, "Listen socket started...\n");	
    
	int efd = epoll_create1(0);
	if (efd == -1) {		
		perror("epoll_create");
		abort();
    }
	fprintf(stdout, "Epoll created...\n");	
	
	struct epoll_event event;
	struct epoll_event *events = NULL;
	
	event.data.fd = listener;
	event.events = EPOLLIN | EPOLLET;
	res = epoll_ctl(efd, EPOLL_CTL_ADD, listener, &event);
	if (res == -1) {
		perror ("epoll_ctl");      
		abort();
    }
    fprintf(stdout, "Listen socket added to epoll...\n");	
	
	/* Buffer where events are returned */
	events = (struct epoll_event*)calloc(maxevents, sizeof event);
	if (events == NULL) {
		fprintf(stderr, "Failed to allocate events for epoll: out of memory\n");
		abort();		
	}
	fprintf(stdout, "Buffer for events allocated...\n");	
	
	crc32_init();
	fprintf(stdout, "CRC32 kernel initialised...\n");
	
	int client_ind = -1;	
	int i;	
	while (1) {
		int n = epoll_wait(efd, events, maxevents, -1);
		if (n < 0) {
			perror("epoll_wait");
			abort();	
		}		
		for (i = 0; i < n; i++) {
			if (listener == events[i].data.fd) {
			/* We have a notification on the listening socket, which
				means one or more incoming connections. */
				while (1) {
					struct sockaddr in_addr;
					socklen_t in_len;
					int infd;
					char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];					

					in_len = sizeof in_addr;
					infd = accept (listener, &in_addr, &in_len);
					if (infd == -1) {
						if ((errno == EAGAIN) ||
							(errno == EWOULDBLOCK)) {
							/* We have processed all incoming
							connections. */
							break;
                        }
						else {
							perror ("accept connection");
							break;
						}
                    }

					res = getnameinfo (&in_addr, in_len,
							hbuf, sizeof hbuf,
							sbuf, sizeof sbuf,
							NI_NUMERICHOST | NI_NUMERICSERV);
					if (res == 0) {
						fprintf(stdout, "[+] ");
						printTimeStamp(stdout);
						fprintf(stdout, " Accepted connection on descriptor %d "
							"(host=%s, port=%s)\n", infd, hbuf, sbuf);
                    }

					/* Make the incoming socket non-blocking and add it to the
					list of fds to monitor. */
					res = make_socket_non_blocking (infd);
					if (res == -1)
						abort ();
					
					event.data.fd = infd;
					event.events = EPOLLIN | EPOLLET ;//| EPOLLOUT;
					res = epoll_ctl (efd, EPOLL_CTL_ADD, infd, &event);
					if (res == -1) {
						perror ("epoll_ctl");
						abort ();
                    }
				}
				continue;
			}
			else if ((client_ind = find_client(events[i].data.fd)) != -1) {
					worker(client_ind);	
				}				
			else {
				fprintf(stdout, "Adding new client...\n");					
				add_new_client(events[i].data.fd);
				worker(total_clients - 1);
			}
		}
	}	
	//for (i = 0; i < total_clients - 1; i++)
		//close(clients[i].fd);	
	free(clients);
	close(listener);
	free(events);	
    close(efd);

	return EXIT_SUCCESS;	
}
