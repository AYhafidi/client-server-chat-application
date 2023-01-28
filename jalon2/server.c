#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>

#include "common.h"
#include "msg_struct.h"

int Disconnect( char * msg){
  char *exit="/quit\n";
  if (strcmp(msg,exit)==0){
    return 1;
  }
  return 0;
}



void Read_server(int sockfd,struct info_client *Clients) {
  int sock=sockfd;
	struct message msgstruct;
	char buff[MSG_LEN];
  struct info_client *tmp=NULL;
	while (1) {
		// Cleaning memory
		memset(&msgstruct, 0, sizeof(struct message));
		memset(buff, 0, MSG_LEN);

		// Receiving structure
		if (recv(sock, &msgstruct, sizeof(struct message), 0) <= 0) {
			break;
		}

    // Receiving message
		if (recv(sock, buff, msgstruct.pld_len, 0) <= 0) {
			break;
		}

    switch (msgstruct.type) {
      case (NICKNAME_NEW):
            tmp=find_client_socket(sockfd,Clients);
            strcpy(tmp->nickname,buff);
            memset(buff, 0, MSG_LEN);
            strcpy(buff,"Welcome to chat : ");
            strcat(buff,tmp->nickname);
            strcat(buff,"\n");
            printf("%s\n",buff );
            break;
      case (NICKNAME_LIST) :
            tmp=find_client_socket(sockfd,Clients);
            memset(buff, 0, MSG_LEN);
            strcpy(buff,"Online clients are : \n");
            liste_client(Clients,buff,tmp);
            break;
      case (NICKNAME_INFOS) :
            memset(buff, 0, MSG_LEN);
            tmp=find_client_nickname(msgstruct.infos,Clients);
            if (tmp==NULL){
              strcpy(buff,"Le client demandé n'existe pas\n");
              break;
            }
            char Port[10];
            sprintf(Port,"%d",tmp->port);
            strcat(buff,msgstruct.infos);
            strcat(buff," :\n\t\t\t IP@ : ");
            strcat(buff,tmp->adresse);
            strcat(buff,"\n\t\t\t Port : ");
            strcat(buff,Port);
            strcat(buff,"\n");
            break;
      case (UNICAST_SEND) :
            tmp=find_client_nickname(msgstruct.infos,Clients);
            if (tmp==NULL){
              memset(buff, 0, MSG_LEN);
              strcpy(buff,"user ");
              strcat(buff,msgstruct.infos);
              strcat(buff,"does not exist\n");
            }
            else{
            sock=tmp->fd;
          }
            break;
      case (BROADCAST_SEND) :

            break;
      /*case (ECHO_SEND) :

            break;*/
      default:
          break;
    }

		printf("pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
		printf("Received: %s", buff);

    msgstruct.pld_len = strlen(buff);
		// Sending structure (ECHO)
		if (send(sock, &msgstruct, sizeof(msgstruct), 0) <= 0) {
			break;
		}
		// Sending message (ECHO)
		if (send(sock, buff, msgstruct.pld_len, 0) <= 0) {
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

struct info_client * init_liste(){

    struct info_client *new_client=(struct info_client *)malloc(sizeof(struct info_client ));
    new_client->fd=-1;
    new_client->port=0;
    new_client->next=NULL;
    strcpy(new_client->nickname,"");

    return new_client;
}

struct info_client * fill_liste(struct info_client * Client,int sockfd,struct sockaddr_in *info_client){
    if (Client->fd==-1){
      Client->fd=sockfd;
      Client->port=ntohs(info_client->sin_port);
      strcpy(Client->adresse,inet_ntoa(info_client->sin_addr));
      return Client;
    }
    else{
      struct info_client *new_client=(struct info_client *)malloc(sizeof(struct info_client ));
      new_client->fd=sockfd;
      new_client->port=ntohs(info_client->sin_port);
      strcpy(new_client->adresse,inet_ntoa(info_client->sin_addr));
      strcpy(new_client->nickname,"");
      new_client->next=NULL;
      Client->next=new_client;
      return Client->next;
    }

}
struct info_client *find_client_nickname(char *Nickname,struct info_client *Clients){
  while (Clients!=NULL) {
    if (strcmp(Clients->nickname,Nickname)==0){
      break;
    }
    Clients=Clients->next;
  }
  return Clients;
}

struct info_client *find_client_socket(int sockfd,struct info_client *Clients){
  if (Clients->fd==sockfd){
    return Clients;
  }
  return find_client_socket(sockfd,Clients->next);
}

void liste_client(struct info_client *Clients,char *buffer,struct info_client *tmp){
  while (Clients!=NULL) {
    if ((strcmp(Clients->nickname,"")!=0)&&(strcmp(Clients->nickname,tmp->nickname)!=0)){
    strcat(buffer,"\t\t\t=>");
    strcat(buffer,Clients->nickname);
    strcat(buffer,"\n");
    }
    Clients=Clients->next;
  }
}

/*void Broadcast_client(struct info_client *Clients,struct info_client *tmp){
    while(Clients!=NULL){
      if(strcmp(Clients->nickname,tmp->nickname)!=0)
          send(Clients->fd,buff,strlen(buff),0);

    }

}*/


void server_client(int sfd,struct info_client * Clients){
  int i;
  struct sockaddr_in client;
  struct info_client *Client=Clients;
  socklen_t len=sizeof(client);
  int connfd;
                                                  /* Gérer Plusieurs connexion */

    struct pollfd fds[NCONN];
    fds[0].fd=sfd;
    fds[0].events=POLLIN;
    fds[0].revents=0;

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
                connfd=accept(fds[0].fd,(struct sockaddr *)&client,&len);
                if (-1 == connfd){
                    perror("Accept");
                    exit(EXIT_FAILURE);
                  }

                int j=1;
                while(fds[j].fd!=-1 && j<NCONN) {
                    j++;/* value */
                }

                fds[j].fd=connfd;
                fds[j].events=POLLIN;
                fds[j].revents=0;
								fds[0].revents=0;



                                                  /*Remplir les infos du client*/

               Client=fill_liste(Client,connfd,&client);

             }

             else if ((fds[i].revents & POLLIN) && fds[i].fd!=fds[0].fd){
                                                          // read data from socket
                    Read_server(fds[i].fd,Clients);
                    fds[i].revents=0;
                    //liste_client(Clients);
                  }

                }

        }
}

void free_liste(struct info_client *Clients){
  if(Clients->next==NULL){
    free(Clients);
  }
  free_liste(Clients->next);
  free(Clients);
}


int main(int argc,char *argv[]) {
	if (argc!=2){
		printf("Erreur d'utilisation : ./server <Numero_Port> \n");
		exit(EXIT_FAILURE);
	}
	struct info_client *Clients=init_liste();

	int sfd;
	sfd = handle_bind(argv);
	if ((listen(sfd, SOMAXCONN)) != 0) {
		perror("listen()\n");
		exit(EXIT_FAILURE);
	}

	server_client(sfd,Clients);
  free_liste(Clients);
	close(sfd);

	return EXIT_SUCCESS;
}
