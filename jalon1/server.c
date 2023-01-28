#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>

#include "common.h"
#define NCONN 4
void echo_server(int sockfd) {
	char buff[MSG_LEN];
	char *exit="/quit";
	while (1) {
		// Cleaning memory
		memset(buff, 0, MSG_LEN);
		// Receiving message
		if (recv(sockfd, buff, MSG_LEN, 0) <= 0) {
			break;
		}
		printf("le message reçu est :%s le message de sortie est :%s la differnce est : %d \n ",buff,exit,strcmp(buff,exit));
		if (strcmp(buff,exit)==0){
			close(sockfd);
			break;
		}
		printf("Received: %s", buff);
		// Sending message (ECHO)
		if (send(sockfd, buff, strlen(buff), 0) <= 0) {
			break;
		}
		printf("Message sent!\n");
		break;
	}
}

int handle_bind(char *argv[]) {
	struct addrinfo hints, *result, *rp;
	int sfd;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo(NULL, argv[1], &hints, &result) != 0) {
		perror("getaddrinfo()");
		exit(EXIT_FAILURE);
	}
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype,
		rp->ai_protocol);
		if (sfd == -1) {
			continue;
		}
		if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0) {
			break;
		}
		close(sfd);
	}
	if (rp == NULL) {
		fprintf(stderr, "Could not bind\n");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(result);
	return sfd;
}

int main(int argc,char *argv[]) {
	struct sockaddr_in client;
	int sfd, connfd;
	int C_connected=0;
	socklen_t len;
	sfd = handle_bind(argv);
	if ((listen(sfd, SOMAXCONN)) != 0) {
		perror("listen()\n");
		exit(EXIT_FAILURE);
	}
                                                              /* Gérer Plusieurs connexion */

			struct pollfd fds[NCONN];
			fds[0].fd=sfd;
			fds[0].events=POLLIN;
			fds[0].revents=0;
			int i;
	for (i=1;i<NCONN;i++){
				fds[i].fd=-1;
				fds[i].events=0;
				fds[i].revents=0;
	}

	while(1){
			// call pollfd
			int r=poll(fds,NCONN, -1);
			if (-1 ==r){
				perror("Poll :");
				exit(EXIT_FAILURE);
			}
			for (i=0;i<NCONN;i++){
					//listen fd

				if ((fds[i].revents & POLLIN) && (fds[i].fd==fds[0].fd)){
									// accept and fill fds[] array
									int connfd=accept(fds[0].fd,(struct sockaddr *)&client,&len);
									if (-1 == connfd){
										perror("Accept");
										exit(EXIT_FAILURE);
									}
									int j=1;
									while(fds[j].fd!=-1 && j<NCONN) {
										j++;
									}
									fds[j].fd=connfd;
									fds[j].events=POLLIN;
									fds[j].revents=0;
				}

				else if ((fds[i].revents & POLLIN) && fds[i].fd!=fds[0].fd){
									// read data from socket
									echo_server(fds[i].fd);
									fds[i].revents=0;
			}

			}
		}

	/*len=sizeof(cli);
	if ((connfd=accept(sfd,(struct sockaddr*) &cli,&len))<0){
		perror("accept()\n");
		exit(EXIT_FAILURE);
	}
	*/
	close(sfd);
	return EXIT_SUCCESS;
}
