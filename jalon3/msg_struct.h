#define NICK_LEN 128
#define INFOS_LEN 128


enum msg_type {
	NICKNAME_NEW,
	NICKNAME_LIST,
	NICKNAME_INFOS,
	ECHO_SEND,
	UNICAST_SEND,
	BROADCAST_SEND,
	MULTICAST_CREATE,
	MULTICAST_LIST,
	MULTICAST_JOIN,
	MULTICAST_SEND,
	MULTICAST_QUIT,
	FILE_REQUEST,
	FILE_ACCEPT,
	FILE_REJECT,
	FILE_SEND,
	FILE_ACK
};

struct message {
	int pld_len;
	char nick_sender[NICK_LEN];
	enum msg_type type;
	char infos[INFOS_LEN];
};

struct salon{
	char nom_salon[NICK_LEN];
	int Nutilisateurs;
	struct salon *next;

};

static char* msg_type_str[] = {
	"NICKNAME_NEW",
	"NICKNAME_LIST",
	"NICKNAME_INFOS",
	"ECHO_SEND",
	"UNICAST_SEND",
	"BROADCAST_SEND",
	"MULTICAST_CREATE",
	"MULTICAST_LIST",
	"MULTICAST_JOIN",
	"MULTICAST_SEND",
	"MULTICAST_QUIT",
	"FILE_REQUEST",
	"FILE_ACCEPT",
	"FILE_REJECT",
	"FILE_SEND",
	"FILE_ACK"
};
