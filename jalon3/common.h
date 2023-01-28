#define MSG_LEN 1024
#define SERV_PORT "8080"
#define SERV_ADDR "127.0.0.1"
#define NCONN 15
#define NICK_LEN 128

char Nickname_Demand[MSG_LEN]="[ SERVER ] : please login with /nick <your pseudo>";

struct info_client{
  int fd;
  int port;
  char adresse[INET_ADDRSTRLEN];
  char nickname[NICK_LEN]; //128->NICK_LEN
  char salon[NICK_LEN];
  struct info_client *next;
};

void liste_client(struct info_client *Clients,char *buffer,struct info_client *tmp);
int Disconnect( char * msg);
enum msg_type Extract_command(char * buff,char *Salon);
void Extract_Msg(char *buff);
int handle_connect(char *Serv_addr,char *Serv_port);
void echo_client(int sockfd);
struct info_client *find_client_socket(int sockfd,struct info_client *Clients);
struct info_client *find_client_nickname(char *Nickname,struct info_client *Clients);
