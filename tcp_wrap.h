
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/un.h> // bzero
#include <netinet/tcp.h>

#define MAX_SENDBUF_SIZE (256 * 1024 * 1024)
#define MAX_RECVBUF_SIZE (256 * 1024 * 1024)

static int OK = 1;

int create_tcpsocket(void){
	int fd = socket(AF_INET,SOCK_STREAM, 0);
	if(fd < 0){
		perror("scocket");
	}
	return fd;
}


char* my_ntoa(int ip){
	struct sockaddr_in tmpAddr;
	tmpAddr.sin_addr.s_addr = ip;
	return inet_ntoa(tmpAddr.sin_addr);
}


void bind_inaddr_any(const int socket,const unsigned short port){
	struct sockaddr_in myaddr;
	bzero((char*)&myaddr,sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port=htons(port);
	if(bind(socket,(struct sockaddr*)&myaddr, sizeof(myaddr)) < 0){
		perror("bind");
		close(socket);
		exit (0);
	}
}

void set_nodelay(const int socket){

	int result = setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (void *)&OK, sizeof(OK));
	if(result < 0){
		perror("set nodelay ");
	}
}

int set_nonblock(const int socket) {
	return fcntl(socket, F_SETFL, O_NONBLOCK);
}

void set_reuse(const int socket){
	int result = setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (void *)&OK, sizeof(OK));
	if(result < 0){
		perror("set reuse ");
	}
}
void set_keepalive(const int socket){
	int result = setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, (void *)&OK, sizeof(OK));
	if(result < 0){
		perror("set keepalive ");
	}
}


void socket_maximize_sndbuf(const int socket){
    socklen_t intsize = sizeof(int);
    int last_good = 0;
    int min, max, avg;
    int old_size;

	//*  minimize!
	avg = intsize;
	setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (void *)&avg, intsize);
	return;
	//*
	
    if (getsockopt(socket, SOL_SOCKET, SO_SNDBUF, &old_size, &intsize) != 0) {
        return;
    }

    min = old_size;
    max = MAX_SENDBUF_SIZE;

    while (min <= max) {
        avg = ((unsigned int)(min + max)) / 2;
        if (setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (void *)&avg, intsize) == 0) {
            last_good = avg;
            min = avg + 1;
        } else {
            max = avg - 1;
        }
    }
}
void socket_maximize_rcvbuf(const int socket){
    socklen_t intsize = sizeof(int);
    int last_good = 0;
    int min, max, avg;
    int old_size;

    if (getsockopt(socket, SOL_SOCKET, SO_RCVBUF, &old_size, &intsize) != 0) {
        return;
    }

    min = old_size;
    max = MAX_RECVBUF_SIZE;

    while (min <= max) {
        avg = ((unsigned int)(min + max)) / 2;
        if (setsockopt(socket, SOL_SOCKET, SO_RCVBUF, (void *)&avg, intsize) == 0) {
            last_good = avg;
            min = avg + 1;
        } else {
            max = avg - 1;
        }
    }
}