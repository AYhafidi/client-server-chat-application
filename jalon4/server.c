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
#include <time.h>

#include "common.h"
#include "msg_struct.h"

                                                              /*I tried to put them in common.h but it didn't work*/

void Message_Server(int *sockfd,struct info_client *Clients,struct message *msgstruct,char *buff,struct salon *Salon,struct pollfd *fds);
void Send_Server(int sockfd,struct message *msgstruct,char* buff);
void Broadcast_send(int sockfd,struct info_client *Clients,char *buff,struct message *msgstruct);
void Create_salon(struct salon *Salon,char * Name,struct info_client *Client);
void liste_salon(struct salon *Salon,char *buffer);
struct salon *find_salon_name(struct salon *Salon, char* Name);
void remove_salon(struct salon *Salon,struct salon *Salon_actuel);
void Multicast_send(struct info_client * Clients,char * buff, struct message *msgstruct,struct info_client *Client);
void set_nickname(int *sockfd,struct info_client *Clients,struct message *msgstruct,char *buff);
void Sallon_Create(int *sockfd,struct info_client *Clients,struct message *msgstruct,char *buff,struct salon *Salon);
void remove_client(int sockfd,struct info_client *Clients);
void move_client_list(struct info_client *Clients);
void move_salon_list(struct salon *Salon);
void Client_date(char *Date);


                                                        /*Deconnecte un client*/
void disconnect_client( struct info_client *Client,char * msg,struct pollfd *fds,struct info_client *Clients){
  char *exit="/quit\n";
  int i;
  if (strcmp(msg,exit)==0){
      close(Client->fd);
      for (i=0;i<NCONN;i++){
        if (fds[i].fd==Client->fd){
          fds[i].fd=-1;
          fds[i].events=0;
          fds[i].revents=0;
        }
      }
      remove_client(Client->fd,Clients);
  }
}

                                                            /*  Verifier si le pseudo est valide */
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



void Read_server(int sockfd,struct info_client *Clients,struct salon *Salon,struct pollfd *fds) {
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
    Message_Server(&sockfd,Clients,&msgstruct,buff,Salon,fds);

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
                                            //MSG a envoyer Au client qui va quitter

                msgstruct->pld_len=strlen(buff);
                Send_Server(Client->fd,msgstruct,buff);
                if (strcmp(Client->salon,msgstruct->infos)==0){

                tmp=Clients;
                memset(tmp_aux, 0, MSG_LEN);
                strcpy(tmp_aux,"[ ");
                strcat(tmp_aux,msgstruct->infos);
                strcat(tmp_aux,"] INFO >");
                strcat(tmp_aux,Client->nickname);
                strcat(tmp_aux,"has quit ");
                strcat(tmp_aux,msgstruct->infos);


                msgstruct->pld_len=strlen(tmp_aux);
                while(tmp!=NULL){
                      if ((strcmp(tmp->salon,msgstruct->infos)==0)&& (Client!=tmp)){

                              Send_Server(tmp->fd,msgstruct,tmp_aux);
                            }
                      tmp=tmp->next;
                    }
                  }
                      break;

          break;
    }


}


      /*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% Message a envoyer par le serveur %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

void Message_Server(int *sockfd,struct info_client *Clients,struct message *msgstruct,char *buff,struct salon *Salon,struct pollfd *fds){
  struct info_client *tmp;
  struct salon *tmp_Salon;
  char tmp_aux[MSG_LEN];
  memset(tmp_aux,0,MSG_LEN);
  char Port[10];
  tmp=find_client_socket(*sockfd,Clients);
  if (strcmp(tmp->nickname,"")==0 && msgstruct->type!=NICKNAME_NEW){
      memset(buff, 0, MSG_LEN);
      strcpy(buff,"[ SERVER ] : please login with /nick <your pseudo>");
  }
  else {
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
          sprintf(Port,"%d",tmp->port);
          strcat(buff,msgstruct->infos);
          strcat(buff,"connected since ");
          strcat(buff,tmp->Date);
          strcat(buff," with IP adresse ");
          strcat(buff,tmp->adresse);
          strcat(buff," and port number ");
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
          tmp=find_client_socket(*sockfd,Clients);
          disconnect_client(tmp,buff,fds,Clients);
          strcpy(tmp_aux,buff);
          memset(buff,0,MSG_LEN);
          strcat(buff,"[ SERVER ] ");
          strcat(buff,tmp_aux);

          break;
    case(MULTICAST_CREATE):
          Sallon_Create(sockfd,Clients,msgstruct,buff,Salon);
          break;
    case(MULTICAST_LIST):
          //tmp=find_client_socket(*sockfd,Clients);
          memset(buff, 0, MSG_LEN);
          strcpy(buff,"[ SERVER ] Channels : ");
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
                  tmp_Salon=find_salon_name(Salon,tmp->salon);
                  strcpy(tmp->salon,msgstruct->infos);
                  if(--tmp_Salon->Nutilisateurs==0){
                      remove_salon(Salon,tmp_Salon);
                  }
                  strcpy(buff,"[ ");
                  strcat(buff,msgstruct->infos);
                  strcat(buff,"] INFO > ");
                  strcat(buff,tmp->nickname);
                  strcat(buff,"has joined ");
                  strcat(buff,msgstruct->infos);
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
        if (strcmp(tmp->salon,msgstruct->infos)!=0){
            memset(buff,0,MSG_LEN);
            strcpy(buff,"[ SERVER ] You are not a member in this channel");
        }
        else {
        strcpy(tmp->salon,"");
        memset(buff,0,MSG_LEN);
        strcpy(buff,"[ ");
        strcat(buff,msgstruct->infos);
        strcat(buff,"] You have left .\n");
        strcat(buff,msgstruct->infos);
        if(--tmp_Salon->Nutilisateurs==0){
            remove_salon(Salon,tmp_Salon);
            strcat(buff,"\n[ SERVER ] You were the last user in this channel,");
            strcat(buff,msgstruct->infos);
            strcat(buff,"has been destroyed");
        }
      }
          break;

    case (FILE_REQUEST):
        tmp=find_client_nickname(msgstruct->infos,Clients);
        if (tmp==NULL){
              memset(buff, 0, MSG_LEN);
              strcpy(buff,"[ SERVER ] USER ");
              strcat(buff,msgstruct->infos);
              strcat(buff,"does not exist");
            }
        else{
              strcpy(tmp_aux,buff);
              memset(buff, 0, MSG_LEN);

              strcat(buff,"[ ");
              strcat(buff,msgstruct->nick_sender);
              strcat(buff,"] ");
              strcat(buff,"wants you to accept the transfer of the file named \"");
              tmp_aux[strlen(tmp_aux)-1]='"';
              strcat(buff,tmp_aux);
              strcat(buff,". Do you accept? [Y/N]");
              *sockfd=tmp->fd;
            }
          break;
      case FILE_ACCEPT:
            tmp=find_client_nickname(msgstruct->infos,Clients);
            sprintf(Port,"%d",tmp->port);
            memset(buff, 0, MSG_LEN);
            strcpy(buff,tmp->adresse);
            strcat(buff,":");
            strcat(buff,Port);
            *sockfd=tmp->fd;
            break;
      case FILE_REJECT:
            tmp=find_client_nickname(msgstruct->infos,Clients);
            memset(buff, 0, MSG_LEN);
            strcpy(buff,"[ SERVER ] ");
            strcat(buff,msgstruct->nick_sender);
            strcat(buff, "cancelled file transfer.");
            *sockfd=tmp->fd;
            break ;
    default:
        break;
      }
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
    case EXIT_FAILURE: // Pseudo contient des caracteres speciaux
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
void fill_liste(struct info_client * Clients,int sockfd,struct sockaddr_in *info_client){
    if (Clients->fd==-1){
      Clients->fd=sockfd;
      Clients->port=ntohs(info_client->sin_port);
      Client_date(Clients->Date);
      strcpy(Clients->adresse,inet_ntoa(info_client->sin_addr));
    }
    else{
      struct info_client * tmp=Clients;
      while(tmp->next!=NULL){
        tmp=tmp->next;
      }
      struct info_client *new_client=(struct info_client *)malloc(sizeof(struct info_client ));
      new_client->fd=sockfd;
      new_client->port=ntohs(info_client->sin_port);
      strcpy(new_client->adresse,inet_ntoa(info_client->sin_addr));
      strcpy(new_client->nickname,"");
      strcpy(new_client->salon,"");
      Client_date(new_client->Date);
      new_client->next=NULL;
      tmp->next=new_client;
    }

}
                                                /*Connection time*/

void Client_date(char *Date){
  time_t rawtime;
  time( &rawtime );
  struct tm *date_info = localtime( &rawtime );
  char Day[3];
  char Month[3];
  char Year[5];
  char Hour[3];
  char Minute[3];
  sprintf(Minute,"%02d",date_info->tm_min);
  sprintf(Hour,"%02d",date_info->tm_hour);
  sprintf(Day,"%02d",date_info->tm_mday);
  date_info->tm_mon=date_info->tm_mon+1;
  sprintf(Month,"%02d",date_info->tm_mon);
  date_info->tm_year=date_info->tm_year+1900;
  sprintf(Year,"%04d",date_info->tm_year);
  strcpy(Date,Year);
  strcat(Date,"/");
  strcat(Date,Month);
  strcat(Date,"/");
  strcat(Date,Day);
  strcat(Date,"@");
  strcat(Date,Hour);
  strcat(Date,":");
  strcat(Date,Minute);
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
    if (strcmp(Clients->nickname,"")!=0){
    strcat(buffer,"\t\t\t - ");
    strcat(buffer,Clients->nickname);
    strcat(buffer,"\n");
    }
    buffer[strlen(buffer)-1]=' ';
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
    if (Client->next==NULL){
        Client->fd=-1;
        Client->next=NULL;
        strcpy(Client->nickname,"");
        strcpy(Client->salon,"");
        strcpy(Client->Date,"");
    }
    else{
        move_client_list(Clients);
  }
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

void move_client_list(struct info_client *Clients){
    struct info_client *tmp,*Previous;
    tmp=Clients;
    while(tmp->next!=NULL){
      Previous=tmp;
      tmp=tmp->next;
      Previous->fd=tmp->fd;
      Previous->port=tmp->port;
      strcpy(Previous->adresse,tmp->adresse);
      strcpy(Previous->nickname,tmp->nickname);
      strcpy(Previous->salon,tmp->salon);
      strcpy(Previous->Date,tmp->Date);
      Previous->next=tmp;
    }
    Previous->next=NULL;
    free(tmp);
}


              /* %%%%%%%%%%%%%%%%%%% Liste chainée Salons %%%%%%%%%%%%%%%%%%%%%% */

struct salon * init_liste_salon(){  //Initialisation
    struct salon * Salon=(struct salon *)malloc(sizeof(struct salon));
    strcpy(Salon->nom_salon,"");
    Salon->Nutilisateurs = 0;
    Salon->next=NULL;
    return Salon;
}


                            /* Remplir la lise des salons */
void Create_salon(struct salon *Salon,char * Name,struct info_client *Client){
  struct salon *tmp=Salon;
  if((Salon->next==NULL)&&(strcmp(Salon->nom_salon,"")==0)){
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
  if ((tmp->next==NULL) && (strcmp(tmp->nom_salon,"")==0)){
      strcat(buffer,"There is no channels");
  }
  else{
  while (tmp!=NULL) {
    strcat(buffer,"\n\t\t\t -");
    strcat(buffer,tmp->nom_salon);
    //strcat(buffer,"\n");
    tmp=tmp->next;
    }
  }
}
                                /*Supprimer les salons avec N_utilisateur 0*/
void remove_salon(struct salon *Salon,struct salon *Salon_actuel){
    struct salon *tmp,*previous;
    if (Salon==Salon_actuel){
      if (Salon->next==NULL){
          strcpy(Salon->nom_salon,"");
      }
      else {
          move_salon_list(Salon);
      }
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

void Sallon_Create(int *sockfd,struct info_client *Clients,struct message *msgstruct,char *buff,struct salon *Salon){
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
                if(strcmp(tmp->salon,msgstruct->infos)!=0 && strcmp(tmp->salon,"")!=0){
                    tmp_Salon=find_salon_name(Salon,tmp->salon);
                    if(--tmp_Salon->Nutilisateurs==0){
                        remove_salon(Salon,tmp_Salon);
                      }
                    }
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

void move_salon_list(struct salon *Salon){
    struct salon *tmp,*Previous;
    tmp=Salon;
    while(tmp->next!=NULL){
      Previous=tmp;
      tmp=tmp->next;
      strcpy(Previous->nom_salon,tmp->nom_salon);
      Previous->Nutilisateurs=tmp->Nutilisateurs;
      Previous->next=tmp;
    }
    Previous->next=NULL;
    free(tmp);
}

void free_liste_salon(struct salon * Salon){
  if(Salon->next==NULL){
    free(Salon);
  }
  free_liste_salon(Salon->next);
  free(Salon);
}




                                        /* Demander le pseudo du client */
void nickname_alerte(int sockfd){
    struct message msgstruct;
    char *Nickname_Demand="[ SERVER ] : please login with /nick <your pseudo>";
    memset(&msgstruct,0,sizeof(struct message));
    while(1){
      msgstruct.pld_len=strlen(Nickname_Demand);
      if(send(sockfd,&msgstruct,sizeof(struct message),0)<0){
        break;
      }

      if (send(sockfd,Nickname_Demand,msgstruct.pld_len,0)<=0){
        break;
      }
      break;
    }
}

void Recieve_informations(int sockfd,struct sockaddr_in *Client){
  if(recv(sockfd,&Client->sin_addr,sizeof(struct in_addr),0)<=0){
    perror("Recieve :");
  }
}


void server_client(int sfd,struct info_client * Clients){
  int i;
  struct salon *Salon=init_liste_salon();
  struct sockaddr_in client;
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

                nickname_alerte(connfd);
                int j=1;
                while(fds[j].fd!=-1 && j<NCONN) {
                    j++;
                }

                fds[j].fd=connfd;
                fds[j].events=POLLIN;
                fds[j].revents=0;
								fds[0].revents=0;

                Recieve_informations(connfd,&client);
                                                  /*Remplir les infos du client*/
               fill_liste(Clients,connfd,&client);


             }

             else if ((fds[i].revents & POLLIN) && fds[i].fd!=fds[0].fd){
                                                          // read data from socket
                    Read_server(fds[i].fd,Clients,Salon,fds);
                    fds[i].revents=0;

                  }

                }

        }

        free_liste(Clients);
        free_liste_salon(Salon);

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
