#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <kj_evo/kj_evo.h>

#define PORT 49152	// listening port tcp
#define BACKLOG 10 	// dimensione della coda di connessioni
#define BUFSIZE 8 	// dimensione del buffer di messaggio
#define FAILURE 3 	// valore di errore di ritorno nel caso di errori delle sockets api

int Vr;
int Vl;

void signal_handler_fn(int signo)
{
	kj_evo_set_motors_speed(0, 0);
	printf("\nCatched.\n");
	exit(1);
}

int
main(void)
{	
	signal(SIGINT, signal_handler_fn);
	
	// initialise kj-evo library
	if((kj_evo_init(0 , NULL)) < 0)
	{
		fprintf(stderr, "Error: Unable to initialize the kj-evo library!\r\n");
		return -1;
	}

	
	// listening socket del server + connected socket del client + valore di ritorno delle sockets
	int sockfd = 0;
	int peerfd = 0;
	int ret = 0;
	
	// apre il listening socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		perror("socket() error: ");
		return FAILURE;
	}
	
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
	{
		perror("setsockopt() error: ");
		return FAILURE;
	}
	
	// definisce un indirizzo su cui mettersi in ascolto
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family= AF_INET;
	
	// qualsiasi ip associato al wildcard address
	//addr.sin_addr.s_addr = htonl("192.168.0.41");
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	// porta su cui mettersi in ascolto
	addr.sin_port = htons(PORT);

	// assegna al socket l'indirizzo memorizzato in addr
	ret = bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
	if (ret == -1)
	{
		perror("bind() error: ");
		close(sockfd);
		return FAILURE;
	}
	
	// abilita il server ad accettare connessioni per questo socket
	ret = listen(sockfd, BACKLOG);
	if (ret == -1)
	{
		perror("listen() error: ");
		close(sockfd);
		return FAILURE;
	}
	
	printf("\n\tServer listening on port %d\n", (int)PORT);
	
	// in peer_addr viene memorizzato l'indirizzo del client connesso
	struct sockaddr_in peer_addr;
	socklen_t len = sizeof(peer_addr);
	
	// regola il loop infinito nel server + regola la gestione della connessione con il client
	int quit = 0;	
	int connected = 0;
	while (!quit) 
	{
		// chiamata bloccante: finché non c'è una connessione completa, il server resta bloccato
		// ritorna il socket connesso della connessione con il client
		peerfd = accept(sockfd, (struct sockaddr *)&peer_addr, &len);
		if (peerfd == -1)
		{
			perror("accept() error: ");
			close(sockfd);
			return FAILURE;
		}
	
		char clientaddr[INET_ADDRSTRLEN] = "";
		inet_ntop(AF_INET, &(peer_addr.sin_addr), clientaddr, INET_ADDRSTRLEN);
		printf("\tAccepted a new TCP connection from %s:%d\n", clientaddr, ntohs(peer_addr.sin_port));
	
		char buf[BUFSIZE];
		ssize_t n = 0;
		connected = 1;

		while (connected)
		{
			n = recv(peerfd, buf, BUFSIZE-1, 0);
			printf("\t\t\treceived %ld\n", n);
			if (n == -1)
			{
				perror("recv() error: ");
			}
			else if (n==0)
			{
				connected = 0;
			}
			else 
			{
				buf[n] ='\0';
				char *token = strtok(buf, " ");
				int i = 0;
				char *speed[2];
				
				while (token != NULL)
				{
					speed[i++] = token;
					token = strtok(NULL, " ");
				}
				
				Vr = (int)atof(speed[0]);
				Vl = (int)atof(speed[1]);
				
				printf("Vr = %d, Vl = %d\n", Vr, Vl);
				
				kj_evo_set_motors_speed(Vl, Vr);
			}
		}
	}

	close(sockfd);
}
