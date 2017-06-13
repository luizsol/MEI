/* -----------------------------------------------------------------------------
 * Title:       web
 * File:        web.c
 * Author:      Gabriel Crabbé
 * Version:     0.0 (2017-06-12)
 * Date:        2017-06-12
 * Description: Exercício 5 de PSI2653.
 * -----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "web.h"
#include "serverlib.h"
#include "queue.h"


struct queue requestQueue;


/**
 * Thread para processamento de request TCP ao servidor.
 */
void *worker(void *arg)
{
	struct config *sconf = (struct config *) arg;
	int E, len, status;
	struct request  req;
	struct response res;

	strcpy(res.base, sconf->base);

	for(;;)
	{
		// Receive
		E = removeQueue(&requestQueue);
		status = read(E, req.msg, sizeof(req.msg));
		if(status < 0)
			perror("Error reading from TCP stream");
		else if(status > 0){
			printf("%s\n", req.msg);
			// Parse request
			parseRequest(&req);

			// Build response
			len = buildResponse(&req, &res);

			printf("%s\n", res.msg);
			status = write(E, res.msg, len);
			if(status <= 0)
				perror("Error writing to TCP stream");

			// Close
			status = close(E);
			if(status)
				perror("Error closing socket");
		} else {
			printf("Connection closed\n");
		}


	}
	return NULL;
}


/**
 * Thread principal de servidor.
 */
void *webserver(void *arg)
{
	// Socket descriptor
	int sd = socket(PF_INET, SOCK_STREAM, 0);
	if(sd == -1)
	{
		perror("Error getting socket descriptor");
		exit(1);
	}

	// Get config
	int status;
	struct config *sconf = (struct config *) arg;
	struct sockaddr_in saddr;

	// Bind
	saddr.sin_family      = AF_INET;
	saddr.sin_addr.s_addr = INADDR_ANY;
	saddr.sin_port        = htons(sconf->port);

	status = bind(sd, (struct sockaddr *) &saddr, sizeof(saddr));
	if(status < 0)
	{
		perror("Error binding socket");
		exit(1);
	}

	// Listen
	status = listen(sd, QUEUE_LENGTH);
	if(status)
	{
		perror("Error listening to socket");
		exit(1);
	}

	// Queue
	initQueue(&requestQueue, 10);

	// Create threads
	pthread_t myworkers[WORKER_THREADS];
	printf("Launching worker threads\n");
	for(int i = 1; i <= WORKER_THREADS; i++)
		pthread_create(&myworkers[i], NULL, worker, (void *) sconf);

	// Accept
	for(;;)
	{
		int size, newsd;
		struct sockaddr_in caddr;

		newsd=accept(sd,(struct sockaddr*) &caddr,(socklen_t *) &size);
		if(newsd < 0)
			perror("Error accepting connection");

		insertQueue(&requestQueue, newsd);
	}

	// Close
	status = close(sd);
	if(status)
		perror("Error closing socket");
}

/**
 * Inicializa a struct de controle do webserver.
 */
int initDriver(struct webDriver *d, struct webConfig *c)
{
	d->config = c;
	sem_init(&d->mutex, 0, 1);

	return 0;
}
