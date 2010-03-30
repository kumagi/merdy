#include "muli.cpp"
#include <sys/socket.h>
#include <sys/types.h>


muli<SocketBuffer> mi;

// TODO address list


class MainEvent{
public:
	inline void operator()(const int fd,SocketBuffer* sb)const{
		printf("dump( ");
		sb->dump();
		printf(")\n");
	}
};

class Accepter{
public:
	inline void operator()(int fd,SocketBuffer*)const{
		struct sockaddr_in addr;
		socklen_t addrlen;
		int newfd = accept(fd,(sockaddr*)(&addr),&addrlen);
		mi.add(newfd,MainEvent());
		printf("accept\n");
	}

};

int main(void){
	
	int accepter = create_tcpsocket();
	bind_inaddr_any(accepter,11011);
	set_reuse(accepter);
	listen(accepter,102);
	
	mi.add(accepter,Accepter());
	
	mi.run();
	
	return 0;
}
