#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include<stdbool.h>
#include <poll.h>

#include "common.h"
#include "msg_struct.h"

void fill_infos(char *buff,struct message *msgstruct,char *Salon,char *Fichier);
void file_send_response(int sockfd,char *buff,struct message *msgstruct,struct pollfd *fds);
int Client_bind(char *Serv_Port);


/*int Disconnect( char * msg){
  char *exit="/quit\n";
  if (strcmp(msg,exit)==0){
    return 1;
  }
  return 0;
}*/


                                                /*-------------Traitement de Message--------------*/


enum msg_type Extract_command(char * buff,char *Salon){
    if (buff[0]=='/'){
      char text_aux[MSG_LEN];
      strcpy(text_aux,buff);          // la fonction strtok fait des changements sur la chaine , faur faire un auxiliere

                                        /* tirer la commande de l'envoi */
    char *delim="\n ";
    char *command=strtok(text_aux,delim);

  if (strcmp(command,"/nick")==0){
      return NICKNAME_NEW;
  }
  if (strcmp(command,"/who")==0){
      return NICKNAME_LIST;
  }
  if (strcmp(command,"/whois")==0){
      return NICKNAME_INFOS;
  }
  if (strcmp(command,"/msgall")==0){
      return BROADCAST_SEND;
  }
  if (strcmp(command,"/msg")==0){
      return UNICAST_SEND;
  }
  if (strcmp(command,"/create")==0){
      return MULTICAST_CREATE;
  }
  if (strcmp(command,"/channel_list")==0){
      return MULTICAST_LIST;
  }
  if (strcmp(command,"/join")==0){
      return MULTICAST_JOIN;
  }
  if (strcmp(command,"/send")==0){
      return FILE_REQUEST;
  }

  else if ((strcmp(command,"/quit")==0) && (strcmp(buff,"/quit\n")!=0)){
      return MULTICAST_QUIT;
  }
}
  else if(strcmp(Salon,"")!=0){
      return MULTICAST_SEND;
}

  //Sinon C'est un msg de type ECHO_SEND
  return ECHO_SEND;

}

char * User_to_send(char * buff){

  char text_aux[MSG_LEN];
  strcpy(text_aux,buff);          // la fonction strtok fait des changements sur la chaine , faur faire un auxiliere

  char *delim="\n ";
  char *User=strtok(text_aux,delim);
  strcat(User," ");
  return User;
}

 void Extract_Msg(char *buff){
                                                    /* tirer la message à envoyer */
   char New_msg[MSG_LEN];
   memset(New_msg,0,MSG_LEN);
   strcpy(buff,strchr(buff,' '));

   int index=1;   //pour sauter l'espace et commencer directement au donnée envoyer
   int index_aux=0;

   while(buff[index]!='\n'){
         New_msg[index_aux]=buff[index];
         //printf("New_msg : %c\nBuff: %c\n",New_msg[index_aux],buff[index]);
         index++;
         index_aux++;
       }
   New_msg[index_aux]='\n';  //Ajouter un retour a la fin pour determiner le msg facilement aprés
   memset(buff,0,MSG_LEN); //cleanin memory
   strncpy(buff,New_msg,MSG_LEN); //fill memory
   //free(New_msg);
 }

void fill_infos(char *buff,struct message *msgstruct,char *Salon,char *Fichier){
  switch (msgstruct->type) {
    case NICKNAME_NEW:
          Extract_Msg(buff);
          strcpy(msgstruct->infos,buff);
          break;
    case NICKNAME_LIST :
          memset(msgstruct->infos,0,INFOS_LEN);
          strcpy(buff,"\n");
          break;
    case NICKNAME_INFOS :
          Extract_Msg(buff);
          strcpy(msgstruct->infos,User_to_send(buff));
          break;
    case UNICAST_SEND :
          memset(msgstruct->infos,0,INFOS_LEN);
          Extract_Msg(buff);
          strcpy(msgstruct->infos,User_to_send(buff));
          Extract_Msg(buff);
          break;
    case BROADCAST_SEND :
          memset(msgstruct->infos,0,INFOS_LEN);
          Extract_Msg(buff);
          break;
    case MULTICAST_CREATE :
          Extract_Msg(buff);
          strcpy(msgstruct->infos,buff);
          break;
    case MULTICAST_LIST :
          memset(msgstruct->infos,0,INFOS_LEN);

          break;
    case MULTICAST_JOIN :
          Extract_Msg(buff);
          memset(msgstruct->infos,0,INFOS_LEN);
          buff[strlen(buff)-1]=' ';
          strcpy(msgstruct->infos,buff);

          break;
    case MULTICAST_SEND :
          strcpy(msgstruct->infos,Salon);
          break;
    case MULTICAST_QUIT :
      Extract_Msg(buff);
      memset(msgstruct->infos,0,INFOS_LEN);
      buff[strlen(buff)-1]=' ';  //Rempalcer le '\n'
      strcpy(msgstruct->infos,buff);
          break;
    case FILE_REQUEST:
      memset(msgstruct->infos,0,INFOS_LEN);
      Extract_Msg(buff);
      strcpy(msgstruct->infos,User_to_send(buff));
      Extract_Msg(buff);
      strcpy(Fichier,buff);
          break;
    default :
          memset(msgstruct->infos,0,INFOS_LEN);
          break;
  }

}


                                                /*-------------Send & Recieve Data--------------*/

void Recv_file(int sockfd){
   char data[MSG_LEN] = {0};
   //struct message msgstruct;
  /*while(1){
            // Receiving structure
    if (recv(sockfd, &msgstruct, sizeof(struct message), 0) <= 0) {
          break;
          }
          printf("longueur : %d\n",msgstruct.pld_len);
            // Receiving message
    if (recv(sockfd, data , msgstruct.pld_len, 0) <= 0) {
            break;
          }
          break;
    }*/

    char *File="recv/Hello.txt";
    strcat(File,data);
    printf("Fichier : %s\n",File);
    FILE *fp=fopen(File,"w");
    if (fp==NULL){
      perror("Opening the File");
      exit(EXIT_FAILURE);
    }
    while (recv(sockfd,data,MSG_LEN,0)>0){
      fputs(data,fp);
      memset(data,0,MSG_LEN);
    }

    fclose(fp);
}

void Send_File(int sockfd,char *Fichier,struct message *msgstruct){
 char data[MSG_LEN] = {0};
 strcpy(data,Fichier);
 char *filename=strtok(Fichier,"\n");
 FILE* fp=fopen(filename,"r");
 if (fp==NULL){
   perror("Opening the File");
   exit(EXIT_FAILURE);
 }
 msgstruct->type=FILE_SEND;
 msgstruct->pld_len=strlen(data);
 while(1){
   if (send(sockfd, msgstruct, sizeof(struct message), 0) <= 0) {
           break;
   }
             // Sending message (ECHO)
   if (send(sockfd, data, msgstruct->pld_len, 0) <= 0) {
           break;
         }

   break;
}
  memset(data,0,MSG_LEN);
 while(fgets(data, MSG_LEN,fp) != NULL) {
   if (send(sockfd, data, sizeof(data), 0) == -1) {
     perror("Couldn't send the file");
     exit(EXIT_FAILURE);
   }
   memset(data,0,MSG_LEN);
 }
 fclose(fp);
}

void File_Action(int sockfd,struct message *msgstruct,char *buff,char *Fichier){
  int sfd;
    switch (msgstruct->type) {
      case FILE_ACCEPT:
            sfd=handle_connect("127.0.0.1","8085");
            if (-1==sfd){
              perror("Could Not Connect");
              exit(EXIT_FAILURE);
            }
            Send_File(sfd,Fichier,msgstruct);

            break;
      default:
            memset(Fichier,0,MSG_LEN);
            break;
    }
}

void Send_Client(int sockfd,char *buff,char *Nickname ,char* Salon,char *Fichier){
  struct message msgstruct;
  memset(&msgstruct, 0, sizeof(struct message));
  msgstruct.type = Extract_command(buff,Salon);
  strcpy(msgstruct.nick_sender,Nickname);
  fill_infos(buff,&msgstruct,Salon,Fichier);
  msgstruct.pld_len = strlen(buff);
  while(1){
                  // Sending structure
        if (send(sockfd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
                break;
        }
                  // Sending message (ECHO)
        if (send(sockfd, buff, msgstruct.pld_len, 0) <= 0) {
                break;
              }

        break;
      }
}




void action_client(struct message msgstruct,char*Nickname,char*Salon){
    switch (msgstruct.type) {
      case NICKNAME_NEW:
            strcpy(Nickname,msgstruct.infos);

      break;
      case MULTICAST_JOIN:
      case MULTICAST_CREATE:
            strcpy(Salon,msgstruct.infos);
      break;
      default:
          break;
    }
}

void Recieve_Client(int sockfd,char *buff, char* Nickname, char* Salon,struct pollfd *fds,char *Fichier){
    struct message msgstruct;
		switch (sockfd) {
			case STDIN_FILENO:
					read(sockfd, buff, MSG_LEN);
					break;

			default:
            while(1){
              memset(&msgstruct, 0, sizeof(struct message));
              // Receiving structure
              if (recv(sockfd, &msgstruct, sizeof(struct message), 0) <= 0) {
                    break;
                    }
                        /*Saving Nickname or the Channel name that the client joined*/
              action_client(msgstruct,Nickname,Salon);

                        // Receiving message
              if (recv(sockfd, buff , msgstruct.pld_len, 0) <= 0) {
                      break;
                    }

              printf("%s\n",buff);


          switch (msgstruct.type) {
                  case FILE_REQUEST:
                        file_send_response(sockfd,buff,&msgstruct,fds);
                        break;

                  case FILE_ACCEPT:
                        File_Action(sockfd,&msgstruct,buff,Fichier);

                        break;

                    default:
                break;
              }


					break;
				}
}
}


void file_send_response(int sockfd,char *buff,struct message *msgstruct,struct pollfd *fds){
  char tmp[NICK_LEN];
  strcpy(tmp,msgstruct->nick_sender);
  strcpy(msgstruct->nick_sender,msgstruct->infos);
  strcpy(msgstruct->infos,tmp);
  int n;
  while (1){
    memset(buff,0,MSG_LEN);
    n=0;
    while ((buff[n++] = getchar()) != '\n') {}
    if (strcmp(buff,"Y\n")==0 || strcmp(buff,"y\n")==0){ // Accepter l'envoi de fichier
        msgstruct->type=FILE_ACCEPT;
        fds[2].fd=Client_bind("8085");
        fds[2].events=POLLIN;
      break;
    }
    else if (strcmp(buff,"N\n")==0 || strcmp(buff,"n\n")==0){
      msgstruct->type=FILE_REJECT;
      break;
    }
    printf("Please Answer by Y or N\n");
}

  while(1){
      // Sending structure
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

/*void echo_client(int sockfd) {
	struct message msgstruct;
  char Nickname[NICK_LEN]="";
	char buff[MSG_LEN];
	int n;
	while (1) {
		// Cleaning memory
		memset(&msgstruct, 0, sizeof(struct message));
    strcpy(msgstruct.nick_sender,Nickname);
		memset(buff, 0, MSG_LEN);
		// Getting message from client
		printf("Message: ");
		n = 0;
		while ((buff[n++] = getchar()) != '\n'){} // trailing '\n' will be sent
		// Filling structure

		msgstruct.type = Extract_command(buff);
    fill_infos(buff,&msgstruct,Nickname);
    msgstruct.pld_len = strlen(buff);
    //printf("MSG SENT : %s\n",buff);
		// Sending structure
		if (send(sockfd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
			break;
		}
		// Sending message (ECHO)
    if (send(sockfd, buff, msgstruct.pld_len, 0) <= 0) {
			break;
		}
		printf("Message sent!\n");
		// Cleaning memory
		memset(&msgstruct, 0, sizeof(struct message));
		memset(buff, 0, MSG_LEN);

		if (recv(sockfd, &msgstruct, sizeof(struct message), 0) <= 0) {
			break;
		}
		// Receiving message
    if (recv(sockfd, buff , msgstruct.pld_len, 0) <= 0) {
      break;
    }
		printf("pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
		printf("Received: %s", buff);

    if (Disconnect(buff)==1){
      close(sockfd);
      break;
  }

	}
}*/
                                                /*-------------Poll Client--------------*/
void Client_Server(int sockfd){
	int i;
	char buff[MSG_LEN];
  char Nickname[NICK_LEN]="";
  char Salon[NICK_LEN]="";
  char Fichier[NICK_LEN]="";
	struct pollfd fds[3];

	fds[0].fd=sockfd;
	fds[0].events=POLLIN;
	fds[0].revents=0;

	fds[1].fd=STDIN_FILENO;
	fds[1].events=POLLIN;
	fds[1].revents=0;

  fds[2].fd=-1;
	fds[2].events=0;
	fds[2].revents=0;

	  while(1){
	// call pollfd
	  int r=poll(fds,3, -1);
	  if (-1 ==r){
	      perror("Poll :");
	      exit(EXIT_FAILURE);
	    }
	  for (i=0;i<3;i++){


	            if ((fds[i].revents & POLLIN) && (fds[i].fd==fds[0].fd)){
	                                                  // Recieving Message
												memset(buff,0,MSG_LEN);
												Recieve_Client(fds[i].fd,buff,Nickname,Salon,fds,Fichier);
												fds[0].revents=0;
	             }

	             else if ((fds[i].revents & POLLIN) && (fds[i].fd==fds[1].fd)) {
	                                                          // Sending Message
													memset(buff,0,MSG_LEN);
													Recieve_Client(fds[i].fd,buff,Nickname,Salon,fds,Fichier);
													Send_Client(fds[0].fd,buff,Nickname,Salon,Fichier);
													fds[1].revents=0;
	                  }
                else if ((fds[i].revents & POLLIN) && (fds[i].fd==fds[2].fd)){
                          memset(buff,0,MSG_LEN);
                          Recv_file(fds[2].fd);
                          fds[2].revents=0;

                }
	               }

	        }

}

                                                /*-------------Connection Client--------------*/

int Client_bind(char *Serv_Port) {
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
    if ((listen(sfd,1)) != 0) {
  		perror("listen()\n");
  		exit(EXIT_FAILURE);
  	}
    return sfd;
}

void send_informations(int sockfd,struct sockaddr_in *Client){
  if (send(sockfd,&Client->sin_addr,sizeof(struct in_addr),0)<=0){
    perror("Send :");
  }
}

int handle_connect(char *Serv_addr,char *Serv_port) {
	struct addrinfo hints, *result, *rp;
	int sfd;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(Serv_addr, Serv_port , &hints, &result) != 0) {
		perror("getaddrinfo()");
		exit(EXIT_FAILURE);
	}
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype,rp->ai_protocol);
		if (sfd == -1) {
			continue;
		}
		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
			break;
		}
		close(sfd);
	}
	if (rp == NULL) {
		fprintf(stderr, "Could not connect\n");
		exit(EXIT_FAILURE);
	}
  send_informations(sfd,(struct sockaddr_in *)rp->ai_addr);
	freeaddrinfo(result);
	return sfd;
}

int main(int argc,char *argv[]) {
  if (argc!=3){
    printf("Erreur d'utilisation : ./client <@IP_client> <Numero_Port>\n");
    exit(EXIT_FAILURE);
  }
	int sfd;
	sfd = handle_connect(argv[1],argv[2]);
	Client_Server(sfd);
	close(sfd);
	exit(EXIT_SUCCESS);
}
