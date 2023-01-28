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
#include <ctype.h>

#include "common.h"
#include "msg_struct.h"

void Message_Server(int *sockfd,struct info_client *Clients,struct message *msgstruct,char *buff,struct salon *Salon);
void Send_Server(int sockfd,struct message *msgstruct,char* buff);
void Broadcast_send(int sockfd,struct info_client *Clients,char *buff,struct message *msgstruct);
void Create_salon(struct salon *Salon,char * Name,struct info_client *Client);
void liste_salon(struct salon *Salon,char *buffer);
struct salon *find_salon_name(struct salon *Salon, char* Name);
void remove_salon(struct salon *Salon,struct salon *Salon_actuel);
void Multicast_send(struct info_client * Clients,char * buff, struct message *msgstruct,struct info_client *Client);
void set_nickname(int *sockfd,struct info_client *Clients,struct message *msgstruct,char *buff);
void set_salon_name(int *sockfd,struct info_client *Clients,struct message *msgstruct,char *buff,struct salon *Salon);
void remove_client(int sockfd,struct info_client *Clients);

int Disconnect( char * msg){
  char *exit="/quit\n";
  if (strcmp(msg,exit)==0){
    return 1;
  }
  return 0;
}

int check_name(char *buff){
  int index=0;
  while ((buff[index]!='\n')&&(index<NICK_LEN)) {
    if ( (!isalpha(buff[index])) && (!isdigit(buff[index])) ){
          return EXIT_FAILURE;
    }
    index++;
  }
  buff[index]=' ';
  return EXIT_SUCCESS;
}



void Read_server(int sockfd,struct info_client *Clients,struct salon *Salon) {
	struct message msgstruct;
	char buff[MSG_LEN];
	while (1) {
		// Cleaning memory
		memset(&msgstruct, 0, sizeof(struct message));
		memset(buff, 0, MSG_LEN);

		// Receiving structure
		if (recv(sockfd, &msgstruct, sizeof(struct message), 0) <= 0) {
			break;
		}

    // Receiving message
		if (recv(sockfd, buff, msgstruct.pld_len, 0) <= 0) {
			break;
		}

    if (Disconnect(buff)==1){
      remove_client(sockfd,Clients);
      close(sockfd);
    }
    //msg_type(&msgstruct,Clients,sockfd);  // ECHO_SEND ---> MULTICAST_SEND si l'utilisateur appartient a un salon

    //printf("Pseudo : %s\nInfos : %s\ntype : %s\n",msgstruct.nick_sender,msgstruct.infos,msg_type_str[msgstruct.type]);

    Message_Server(&sockfd,Clients,&msgstruct,buff,Salon);

    msgstruct.pld_len = strlen(buff);

    switch (msgstruct.type) {
      case BROADCAST_SEND:
          Broadcast_send(sockfd,Clients,buff,&msgstruct);
          break;
      case MULTICAST_SEND:
      case MULTICAST_JOIN:
      case MULTICAST_QUIT:
          Multicast_send(Clients,buff,&msgstruct,find_client_socket(sockfd,Clients));
          break;
      default :
          Send_Server(sockfd,&msgstruct,buff);
            break;
    }

		break;
	}
}


              /*%%%%%%%%%%%%%%%%%%%%%%%% ENVOI DE MSG % %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

void Send_Server(int sockfd,struct message *msgstruct,char* buff){
  while(1){
  if (send(sockfd, msgstruct, sizeof(*msgstruct), 0) <= 0) {
    break;
  }
  // Sending message (ECHO)
  if (send(sockfd, buff, msgstruct->pld_len, 0) <= 0) {
    break;
    }
  break;
  }
}


void Broadcast_send(int sockfd,struct info_client *Clients,char *buff,struct message *msgstruct){
  struct info_client *tmp=Clients;
  while(tmp!=NULL){
      if (tmp->fd!=sockfd){
          Send_Server(tmp->fd,msgstruct,buff);
      }
      tmp=tmp->next;
  }

}

void Multicast_send(struct info_client * Clients,char * buff, struct message *msgstruct,struct info_client *Client){
    struct info_client *tmp;
    char tmp_aux[MSG_LEN];
    switch (msgstruct->type) {
      case MULTICAST_SEND:
            tmp=Clients;
            while(tmp!=NULL){
                if (strcmp(tmp->salon,msgstruct->infos)==0){
                    Send_Server(tmp->fd,msgstruct,buff);
                }
            tmp=tmp->next;
          }
          break;
      case MULTICAST_JOIN :
            tmp=Clients;
            while(tmp!=NULL){

                if ((strcmp(tmp->salon,msgstruct->infos)==0)&& (Client!=tmp)){
                    Send_Server(tmp->fd,msgstruct,buff);
                  }
                  tmp=tmp->next;
                }
                memset(buff,0,MSG_LEN);
                strcpy(buff,"[ SERVER ] You have joined ");
                strcat(buff,msgstruct->infos);
                msgstruct->pld_len=strlen(buff);
                Send_Server(Client->fd,msgstruct,buff);
          break;
      default :
                tmp=Clients;
                memset(tmp_aux, 0, MSG_LEN);
                strcpy(tmp_aux,"[ ");
                strcat(tmp_aux,msgstruct->infos);
                strcat(tmp_aux,"] INFO >");
                strcat(tmp_aux,Client->nickname);
                strcat(tmp_aux,"has quit ");
                strcat(tmp_aux,msgstruct->infos);
                                          //MSG a envyer au client qui va quitter
                msgstruct->pld_len=strlen(buff);
                Send_Server(Client->fd,msgstruct,buff);


                msgstruct->pld_len=strlen(tmp_aux);
                while(tmp!=NULL){
                      if ((strcmp(tmp->salon,msgstruct->infos)==0)&& (Client!=tmp)){

                              Send_Server(tmp->fd,msgstruct,tmp_aux);
                            }
                      tmp=tmp->next;
                    }

                      break;

          break;
    }


}


      /*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% Message a envoyer par le serveur %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*void msg_type(struct message *msgstruct,struct info_client *Clients,int sockfd){
            if (msgstruct->type==ECHO_SEND){
                    struct info_client *tmp=find_client_socket(sockfd,Clients);
                    if (strcmp(tmp->salon,"")!=0){
                            msgstruct->type=MULTICAST_SEND;
                        }
                      }
    }*/

void Message_Server(int *sockfd,struct info_client *Clients,struct message *msgstruct,char *buff,struct salon *Salon){
  struct info_client *tmp;
  struct salon *tmp_Salon;
  char tmp_aux[MSG_LEN];
  memset(tmp_aux,0,MSG_LEN);
  switch (msgstruct->type) {
    case (NICKNAME_NEW):
        set_nickname(sockfd,Clients,msgstruct,buff);
        break;
    case (NICKNAME_LIST) :
          tmp=find_client_socket(*sockfd,Clients);
          memset(buff, 0, MSG_LEN);
          strcpy(buff,"[ SERVER ] Online clients are : \n");
          liste_client(Clients,buff,tmp);
          break;
    case (NICKNAME_INFOS) :
          memset(buff, 0, MSG_LEN);

          strcpy(buff,"[ SERVER ] ");
          tmp=find_client_nickname(msgstruct->infos,Clients);
          if (tmp==NULL){
            strcat(buff,"Le client demandé n'existe pas");
            break;
          }
          char Port[10];
          sprintf(Port,"%d",tmp->port);
          strcat(buff,msgstruct->infos);
          strcat(buff," :\n\t\t\t IP@ : ");
          strcat(buff,tmp->adresse);
          strcat(buff,"\n\t\t\t Port : ");
          strcat(buff,Port);
          break;
    case (UNICAST_SEND) :
          tmp=find_client_nickname(msgstruct->infos,Clients);
          if (tmp==NULL){
            memset(buff, 0, MSG_LEN);
            strcpy(buff,"[ SERVER ] USER ");
            strcat(buff,msgstruct->infos);
            strcat(buff,"does not exist\n");
          }
          else{
            strcpy(tmp_aux,buff);
            memset(buff, 0, MSG_LEN);

            strcat(buff,"[ ");
            strcat(buff,msgstruct->nick_sender);
            strcat(buff,"] ");
            strcat(buff,tmp_aux);
            *sockfd=tmp->fd;
        }
          break;
    case (BROADCAST_SEND) :

            strcat(tmp_aux,"[ ");
            strcat(tmp_aux,msgstruct->nick_sender);
            strcat(tmp_aux,"] ");
            strcat(tmp_aux,buff);
            memset(buff, 0, MSG_LEN);
            strcpy(buff,tmp_aux);
          break;
    case (ECHO_SEND) :
          strcpy(tmp_aux,buff);
          memset(buff,0,MSG_LEN);
          strcat(buff,"[ SERVER ] ");
          strcat(buff,tmp_aux);

          break;
    case(MULTICAST_CREATE):
          set_salon_name(sockfd,Clients,msgstruct,buff,Salon);

          break;
    case(MULTICAST_LIST):
          //tmp=find_client_socket(*sockfd,Clients);
          memset(buff, 0, MSG_LEN);
          strcpy(buff,"[ SERVER ] Channels : \n");
          liste_salon(Salon,buff);

          break;
    case(MULTICAST_JOIN):
          tmp=find_client_socket(*sockfd,Clients);
          tmp_Salon=find_salon_name(Salon,buff);
          if(tmp_Salon==NULL){
            memset(buff, 0, MSG_LEN);
            strcpy(buff,"[ SERVER ] channel ");
            strcat(buff,msgstruct->infos);
            strcat(buff ,"does not exist");
            strcpy(msgstruct->infos,"");
          }
          else{

                if (strcmp(tmp->salon,msgstruct->infos)==0){
                  memset(buff, 0, MSG_LEN);
                  strcpy(buff,"[ SERVER ] You are already a member in this channel");
                }
                else if((strcmp(tmp->salon,msgstruct->infos)!=0) && (strcmp(tmp->salon,"")!=0)){
                  memset(buff, 0, MSG_LEN);
                  strcpy(buff,"[ SERVER ] Quit the actual channel before moving to another one");
                  strcpy(msgstruct->infos,tmp->salon);
                }
                else {
                strcpy(tmp->salon,msgstruct->infos);
                tmp_Salon->Nutilisateurs++;
                memset(buff, 0, MSG_LEN);
                strcpy(buff,"[ ");
                strcat(buff,msgstruct->infos);
                strcat(buff,"] INFO > ");
                strcat(buff,tmp->nickname);
                strcat(buff,"has joined ");
                strcat(buff,msgstruct->infos);

              }
          }
          break;

    case(MULTICAST_SEND):
          tmp=find_client_socket(*sockfd,Clients);
          strcpy(tmp_aux,buff);
          memset(buff, 0, MSG_LEN);
          strcat(buff,"[ ");
          strcat(buff,tmp->salon);
          strcat(buff,"] ");
          strcat(buff,msgstruct->nick_sender);
          strcat(buff,":");
          strcat(buff,tmp_aux);
          break;
    case(MULTICAST_QUIT):
        tmp=find_client_socket(*sockfd,Clients);
        tmp_Salon=find_salon_name(Salon,msgstruct->infos);
        strcpy(tmp->salon,"");
        memset(buff,0,MSG_LEN);
        strcpy(buff,"[ ");
        strcat(buff,msgstruct->infos);
        strcat(buff,"] You have left ");
        strcat(buff,msgstruct->infos);
        if(--tmp_Salon->Nutilisateurs==0){
            remove_salon(Salon,tmp_Salon);
            strcat(buff,"\n[ SERVER ] You were the last user in this channel,");
            strcat(buff,msgstruct->infos);
            strcat(buff,"has been destroyed");
        }
          break;

    default:
        break;
  }
}


int handle_bind(char *Serv_Port) {
	struct addrinfo hints, *result, *rp;
	int sfd;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo(NULL, Serv_Port, &hints, &result) != 0) {
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

              /* %%%%%%%%%%%%%%%%%%% Liste chainée Clients %%%%%%%%%%%%%%%%%%%%%% */
                              /*Donner un nickname au utilisateur*/
void set_nickname(int *sockfd,struct info_client *Clients,struct message *msgstruct,char *buff){
  struct info_client *tmp,*tmp_name;

  switch (check_name(msgstruct->infos)) {
    case EXIT_SUCCESS:
    tmp=find_client_socket(*sockfd,Clients);
    tmp_name=find_client_nickname(msgstruct->infos,Clients);
    if (tmp_name==NULL || tmp_name==tmp){ // Cas ou le pseudo n'est pas deja utiliser
        if (strcmp(tmp->nickname,"")==0){  //Un nouveu pseudo
            strcpy(tmp->nickname,msgstruct->infos);
            memset(buff, 0, MSG_LEN);
            strcpy(buff,"[ SERVER ] Welcome to chat : ");
            strcat(buff,tmp->nickname);
        }

        else{
          strcpy(tmp->nickname,msgstruct->infos);
          memset(buff, 0, MSG_LEN);
          strcpy(buff,"[ SERVER ] Nickname Changed !  ");
          strcat(buff,msgstruct->nick_sender);
          strcat(buff," ==> ");
          strcat(buff,msgstruct->infos);
        }
    }
    else if (tmp_name!=NULL){ // Cas ou le pseudo est deja utiliser
        memset(buff, 0, MSG_LEN);
        strcpy(buff,"[ SERVER ]  Nickname Already in use Please choose another !");
        strcpy(msgstruct->infos,msgstruct->nick_sender);
    }

        break;
    case EXIT_FAILURE:
        memset(buff, 0, MSG_LEN);
        strcpy(buff,"[ SERVER ] Nickname can contain only alphabets and numbers .");
        strcat(msgstruct->infos,msgstruct->nick_sender);
      break;
  }

}

struct info_client * init_liste(){  //Initialisation

    struct info_client *new_client=(struct info_client *)malloc(sizeof(struct info_client ));
    new_client->fd=-1;
    new_client->port=0;
    new_client->next=NULL;
    strcpy(new_client->nickname,"");
    strcpy(new_client->salon,"");
    return new_client;
}

                                            /* Remplir la liste chainée */
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
      strcpy(new_client->salon,"");
      new_client->next=NULL;
      Client->next=new_client;
      return Client->next;
    }

}
                                                /* Trouver un client par nickname*/

struct info_client *find_client_nickname(char *Nickname,struct info_client *Clients){
  while (Clients!=NULL) {
    if (strcmp(Clients->nickname,Nickname)==0){
      break;
    }
    Clients=Clients->next;
  }
  return Clients;
}
                                    /* Trouver un client par socket*/
struct info_client *find_client_socket(int sockfd,struct info_client *Clients){
  if (Clients->fd==sockfd){
    return Clients;
  }
  return find_client_socket(sockfd,Clients->next);
}
                                /* Remplir la liste des clients commande "/who" */
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
                                      /*Liberer la memoir*/
void free_liste(struct info_client *Clients){
  if(Clients->next==NULL){
    free(Clients);
  }
  free_liste(Clients->next);
  free(Clients);
}

void remove_client(int sockfd,struct info_client *Clients){
  struct info_client *tmp,*previous,*Client;
  Client=find_client_socket(sockfd,Clients);
  if (Client==Clients){
    tmp=Clients->next;
    free(Clients);
    Clients=tmp;
  }
  else{
    tmp=Clients;
    while(tmp!=Client){
      previous=tmp;
      tmp=tmp->next;
    }
    previous->next=tmp->next;
    free(tmp);
  }

}


              /* %%%%%%%%%%%%%%%%%%% Liste chainée Salons %%%%%%%%%%%%%%%%%%%%%% */

struct salon * init_liste_salon(){  //Initialisation
    struct salon * Salon=(struct salon *)malloc(sizeof(struct salon));
    strcpy(Salon->nom_salon,"%");
    Salon->Nutilisateurs = 0;
    Salon->next=NULL;
    return Salon;
}


                            /* Remplir la lise des salons */
void Create_salon(struct salon *Salon,char * Name,struct info_client *Client){
  struct salon *tmp=Salon;
  if(strcmp(Salon->nom_salon,"%")==0){ //Caractere special qui ne peut pas etre commme nom pour un salon
      strcpy(Salon->nom_salon,Name);
      strcpy(Client->salon,Name);
      Salon->Nutilisateurs=1;

  }
  else{
    while(tmp->next!=NULL){
      tmp=tmp->next;
    }
    tmp->next=(struct salon*)malloc(sizeof(struct salon));
    tmp=tmp->next;
    strcpy(tmp->nom_salon,Name);
    strcpy(Client->salon,Name);
    tmp->Nutilisateurs=1;
    tmp->next=NULL;
  }
}

                                        /* trouver un salon par nom */
struct salon *find_salon_name(struct salon *Salon, char* Name){
    struct salon *tmp=Salon;
    while(tmp!=NULL){
            if (strcmp(tmp->nom_salon,Name)==0){
              return tmp;
            }
            tmp=tmp->next;
    }
    return tmp;
}


                                /* Remplir la liste des salons commande "/chainne_liste" */
void liste_salon(struct salon *Salon,char *buffer){
  struct salon* tmp=Salon;
  while (tmp!=NULL) {
    strcat(buffer,"\t\t\t=>");
    strcat(buffer,tmp->nom_salon);
    strcat(buffer,"\n");
    tmp=tmp->next;
  }
}
                                /*Supprimer les salons avec N_utilisateur 0*/
void remove_salon(struct salon *Salon,struct salon *Salon_actuel){
    struct salon *tmp,*previous;
    if (Salon==Salon_actuel){
      tmp=Salon->next;
      free(Salon);
      Salon=tmp;
    }
    else{
    tmp=Salon;
    while (tmp!=Salon_actuel){
        previous=tmp;
        tmp=tmp->next;
    }
    previous->next=tmp->next;
    free(tmp);
  }
}

void set_salon_name(int *sockfd,struct info_client *Clients,struct message *msgstruct,char *buff,struct salon *Salon){
  struct info_client *tmp;
  struct salon *tmp_Salon;
  switch (check_name(msgstruct->infos)) {
      case EXIT_SUCCESS:
            tmp=find_client_socket(*sockfd,Clients);
            tmp_Salon=find_salon_name(Salon,msgstruct->infos);
            if (tmp_Salon!=NULL){
                memset(buff,0,MSG_LEN);
                strcpy(buff,"[ SERVER ] Channel with the same name already existe");
              }
            else{
                Create_salon(Salon,msgstruct->infos,tmp);
                memset(buff,0,MSG_LEN);
                strcpy(buff,"[ SERVER ] You have created a channel : ");
                strcat(buff,msgstruct->infos);
    }
            break;
      case EXIT_FAILURE:
                memset(buff, 0, MSG_LEN);
                strcpy(buff,"[ SERVER ] Channel name can contain only alphabets and numbers .");
                break;
  }
}






/*void nickname_alerte(int sockfd){
    struct message msgstruct;
    memset(&msgstruct,0,sizeof(struct message));
    while(1){
      if(send(sockfd,&msgstruct,sizeof(struct message),0)){
        break;
      }

      if (send(sockfd,Nickname_Demand,MSG_LEN,0)<=0){
        break;
      }
      break;
    }
}*/


void server_client(int sfd,struct info_client * Clients){
  int i;
  struct salon *Salon=init_liste_salon();
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
                //memset(&client,0,sizeof(client));
                connfd=accept(fds[0].fd,(struct sockaddr *)&client,&len);
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
								fds[0].revents=0;


                                                  /*Remplir les infos du client*/
              printf("@IP : %s\n", inet_ntoa(client.sin_addr));
               Client=fill_liste(Client,connfd,&client);


             }

             else if ((fds[i].revents & POLLIN) && fds[i].fd!=fds[0].fd){
                                                          // read data from socket
                    Read_server(fds[i].fd,Clients,Salon);
                    fds[i].revents=0;

                    if (fds[i].revents==POLLHUP){
                      printf("Client s'est deconnecté");
                      close(fds[i].fd);
                      fds[i].fd=-1;
                      fds[i].events=0;
                      fds[i].revents=0;
                    }
                  }

                }

        }

}



int main(int argc,char *argv[]) {
	if (argc!=2){
		printf("Erreur d'utilisation : ./server <Numero_Port> \n");
		exit(EXIT_FAILURE);
	}
	struct info_client *Clients=init_liste();

	int sfd;
	sfd = handle_bind(argv[1]);
	if ((listen(sfd, SOMAXCONN)) != 0) {
		perror("listen()\n");
		exit(EXIT_FAILURE);
	}

	server_client(sfd,Clients);
  free_liste(Clients);
	close(sfd);

	return EXIT_SUCCESS;
}
