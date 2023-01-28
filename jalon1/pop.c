#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BACKLOG 30

struct bank_account {
	int account_number;
	char name[150];
	int value;
	// char * msg; -> Interdit
};

int main() {
	int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == listen_fd)
		perror("Socket:");

	int value = 1;
	if (-1 == setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(int)))
		perror("Setsockopt:");

	struct sockaddr_in listen_addr;
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_port = htons(33000);
	listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (-1 == bind(listen_fd, (struct sockaddr *)&listen_addr, sizeof(listen_addr)))
		perror("Bind:");

	if (-1 == listen(listen_fd, BACKLOG))
		perror("Listen:");


	//struct sockaddr_in client_addr;
	//socklen_t addr_len = sizeof(client_addr);
	//printf("Waiting...\n");
	//int new_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addr_len);
	//if (-1 == new_fd)
	//	perror("Accept:");
//	printf("New connection and socket fd is %d\n", new_fd);

	// read msg from socket
	char msg[100] = {0};
	int nxt_msgsize = -1;
	// Read next msg_size
	int nbytes = read(new_fd, &nxt_msgsize, sizeof(int));
	if (-1 == nbytes)
		perror("Read size:");
	while (nbytes != sizeof(int)) {
		int r = read(new_fd, (char *)&nxt_msgsize + nbytes, sizeof(int) - nbytes);
		if (-1 == r)
			perror("Read size:");
		nbytes += r;
	}
	printf("New msg size read is %d\n", nxt_msgsize);

	// read new msg
	int to_be_read = nxt_msgsize;
	nbytes = read(new_fd, msg, to_be_read);
	if (-1 == nbytes)
		perror("Read:");
	while (nbytes != to_be_read) {
		int r = read(new_fd, msg + nbytes, to_be_read - nbytes);
		if (-1 == r)
			perror("Read:");
		nbytes += r;
	}
	printf("Msg received from client is \"%s\"\n", msg);
	struct pollfd fds[10];
	fds[0].fd=listen_fd;
	fds[0].events=POLLIN;
	fds[0].revents=0;
	for(int i=1; i<=9;i++){
		fds[i].fd=-1;
		fds[i].events=0;
		fds[i].revents=0;
	}
	while(1){
		//call poll
		int r=poll(fds,10,-1)
		if (r==-1)
		perror("poll");
		for (int i=0;i<10;i++){
			//if listen_fd
			if ((fds[i].revents & POLLIN) && fds[i].fd==fds[0].fd){
				new_fd=accept(fd[i].fd,0,0);
				//fds[i+1].fd=new_fd;
				int j=1;
				while(fds[j].fd!=-1 && j<10)
					j++;
					fds[j].fd=new_fd;
					fds[j].events=POLLIN;
					fds[j].revents=0;
				// accept and fill fds[] array
			}
			if ((fds[i].revents & POLLIN) && fds[i].fd !=fds[0].fd){
				//read data from Socket
				char msg[100] = {0};
				int nxt_msgsize = -1;
				// Read next msg_size
				int nbytes = read(new_fd, &nxt_msgsize, sizeof(int));
				if (-1 == nbytes)
					perror("Read size:");
				while (nbytes != sizeof(int)) {
					int r = read(new_fd, (char *)&nxt_msgsize + nbytes, sizeof(int) - nbytes);
					if (-1 == r)
						perror("Read size:");
					nbytes += r;
				}
				printf("New msg size read is %d\n", nxt_msgsize);

				// read new msg
				int to_be_read = nxt_msgsize;
				nbytes = read(new_fd, msg, to_be_read);
				if (-1 == nbytes)
					perror("Read:");
				while (nbytes != to_be_read) {
					int r = read(new_fd, msg + nbytes, to_be_read - nbytes);
					if (-1 == r)
						perror("Read:");
					nbytes += r;
				}
				printf("Msg received from client is \"%s\"\n", msg);
				//write data to socket

			}
		}
	}
	return 0;
}
