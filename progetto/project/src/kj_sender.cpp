#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ros/ros.h"
#include "project/Input.h"

#define ADDRESS "192.168.0.41"	// server address
//#define ADDRESS "127.0.0.4"
#define PORT 49152 	// tcp listening port
#define BACKLOG 10 	// dimensione della coda di connessioni
#define BUFSIZE 7 	// dimensione del buffer di messaggio
#define FAILURE 3 	// valore di errore di ritorno in caso di errori delle sockets api

double angular;
double linear;
int Vr, Vl;
int l = 10;
bool keepRunning = true;

void compute(double w, double v)
{
	printf("w = %f\n", w);
	printf("v = %f\n", v);
	
	if (v == 0.0)
	{
		Vl = 50;
		Vr = 50;
	}
	else
	{	
		if (w <= 1 && w >= -1)
		{
			if (w >= -0.1 && w <= 0.1)
			{
				Vl = 14;
				Vr = 14;
			}
			else if (w > 0.1 && w <= 0.5)
			{
				Vl = 11;
				Vr = 17;
			}
			else if (w > 0.5 && w <= 1)
			{
				Vl = 8;
				Vr = 20;
			}
			else if (w >= -0.5 && w < -0.1)
			{
				Vl = 17;
				Vr = 11;
			}
			else if (w >= -1 && w < -0.5)
			{
				Vl = 20;
				Vr = 8;
			}
		}
	}
}

void callback(const project::Input &msg)
{
	ros::Rate loop_rate(100);
	loop_rate.sleep();
	angular = msg.w;
	linear = msg.v;
	
	compute(angular, linear);
}

void signal_handler_fn(int signo)
{
	keepRunning = false;
	std::cout << "Catched" << std::endl;
	exit(0);
}

int
main(int argc, char *argv[])
{
	//SOCKET STUFF
	signal(SIGINT, signal_handler_fn);

	// valore di ritorno delle sockets api	
	int res = 0;

	// connection socket, serve per la comunicazione con il server	
	int sockfd = 0;

	// server address senza hostname resolution + dimensione della struttura di indirizzo	
	struct sockaddr_in server;
	socklen_t len = sizeof(server);
	
	// azzera la struttura dati + specifica l'address family
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	
	// specifica la porta del server da contattare per avviare il 3WHS
	server.sin_port = htons(PORT);
	
	printf("\tTCP Client app connecting to TCP server at '%s'\n", ADDRESS);
		
	// converte l'indirizzo dotted decimal
	res = inet_pton(AF_INET, ADDRESS, &(server.sin_addr));
	if (res == 1)
	{
		//ha memorizzato l'indirizzo IPv4 del server
	}
	else if (res == -1)
	{
		perror("inet_pton() error: ");
		return FAILURE;
	}
	else if (res == 0)
	{
		//not a valid IPv4 dotted-decimal string
		return FAILURE;
	}
		
	//connessione al server
	// apre il socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		perror("socket() error: ");
		return FAILURE;
	}
	
	// avvia il 3WHS
	res = connect(sockfd, (struct sockaddr *)&server, len);
	if (res != 0)
	{
		perror("connect() error: ");
		close(sockfd);
		return FAILURE;
	}
	
	// messaggio da inviare al server
	char msg[BUFSIZE] = "";
	char vr[BUFSIZE] = "";
	char vl[BUFSIZE] = "";
	ssize_t n = 0;
	
	// ROS STUFF
	ros::init(argc, argv, "kj_sender");
	ros::NodeHandle node;

	std::cout <<"Starting Connection to ROS Environment."<< std::endl;
	
	// This node will subscribe the messages onto ViconTopic   queue_size
	ros::Subscriber sub = node.subscribe("SenderTopic", 1000, callback);

	// This node will subscribe messages at 100 Hz
	ros::Rate loop_rate(100);

	while(keepRunning)
	{
	
		if (Vr != 0 && Vl != 0)
		{
			if (Vr == 50 && Vl == 50)
			{
				Vr = 0;
				Vl = 0;
			}
			
			printf("Vr = %d, Vl = %d\n", Vr, Vl);
			
			snprintf(vr, BUFSIZE-1, "%d", Vr);
			snprintf(vl, BUFSIZE-1, "%d", Vl);

			char msg[BUFSIZE] = "";
			printf("msg: %s", msg);
			printf("\tvr: %s", vr);
			printf("\tvl: %s\n", vl);

			strncat(msg, vr, 3);
			strncat(msg, " ", 1);
			strncat(msg, vl, 3);

			printf("msg: %s\n", msg);
	
			n = send(sockfd, msg, strlen(msg), 0);
			printf("\t\t\tsent %ld\n", n);
			if (n == -1)
			{
				perror("send() error: ");
				close(sockfd);
				return FAILURE;
			}
		}
	
		ros::spinOnce();
	}
	
	// chiude il socket
	close(sockfd);

	return 0;
}
