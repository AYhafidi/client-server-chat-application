#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ctype.h>
#include<stdbool.h>
#include <poll.h>

#include "common.h"
#include "msg_struct.h"

void fill_infos(char *buff,struct message *msgstruct,char *Nickname);
void set_nickname(char *buff,char *Nickname );

int Disconnect( char * msg){
  char *exit="/quit\n";
  if (strcmp(msg,exit)==0){
    return 1;
  }
  return 0;
}

enum msg_type Extract_command(char * buff){
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
  else {
    return ECHO_SEND;
  }
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


void fill_infos(char *buff,struct message *msgstruct,char *Nickname){
  switch (msgstruct->type) {
    case (NICKNAME_NEW):
          Extract_Msg(buff);
          if(strcmp(msgstruct->nick_sender,"")==0){
            memset(msgstruct->infos,0,INFOS_LEN);
          }
          else {
            strcpy(msgstruct->infos,msgstruct->nick_sender);
          }
          set_nickname(buff,Nickname);

          break;
    case (NICKNAME_LIST) :
          memset(msgstruct->infos,0,INFOS_LEN);
          strcpy(buff,"\n");
          break;
    case (NICKNAME_INFOS) :
          Extract_Msg(buff);
          strcpy(msgstruct->infos,User_to_send(buff));
          strcpy(buff,"\n");

          break;
    case (UNICAST_SEND) :
          memset(msgstruct->infos,0,INFOS_LEN);
          Extract_Msg(buff);
          strcpy(msgstruct->infos,User_to_send(buff));
          Extract_Msg(buff);
          break;
    case (BROADCAST_SEND) :
          memset(msgstruct->infos,0,INFOS_LEN);
          Extract_Msg(buff);
          break;
    default :
          memset(msgstruct->infos,0,INFOS_LEN);
          break;
  }

}

void set_nickname(char *buff,char * Nickname){
  bool nickname_right=true;
    int index=0;
    while ((buff[index]!='\n')&&nickname_right&&(index<NICK_LEN)) {
      if ( (!isalpha(buff[index])) && (!isdigit(buff[index])) ){
            printf("Error Pseudo : Veuillez utiliser des chiffres ou lettres de l'alphabet \n");
            nickname_right=false;
      }
      index++;
    }
    buff[index]=' ';
    if(nickname_right){
          strcpy(Nickname,buff);
        }
}




void echo_client(int sockfd) {
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
    printf("MSG SENT : %s\n",buff);
		// Sending message (ECHO)
    if (send(sockfd, buff, msgstruct.pld_len, 0) <= 0) {
			break;
		}
		printf("Message sent!\n");
		// Cleaning memory
		memset(&msgstruct, 0, sizeof(struct message));
		memset(buff, 0, MSG_LEN);
		// Receiving structure
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
}


int handle_connect(char *argv[]) {
	struct addrinfo hints, *result, *rp;
	int sfd;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(argv[1], argv[2] , &hints, &result) != 0) {
		perror("getaddrinfo()");
		exit(EXIT_FAILURE);
	}
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype,rp->ai_protocol);
		if (sfd == -1) {
			continue;
		}
		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
      printf("Connection to server ....... Done!\n");
			break;
		}
		close(sfd);
	}
	if (rp == NULL) {
		fprintf(stderr, "Could not connect\n");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(result);
	return sfd;
}

int main(int argc,char *argv[]) {
  if (argc!=3){
    printf("Erreur d'utilisation : ./client <@IP_client> <Numero_Port>\n");
    exit(EXIT_FAILURE);
  }
	int sfd;
	sfd = handle_connect(argv);
	echo_client(sfd);
	close(sfd);
	exit(EXIT_SUCCESS);
}
