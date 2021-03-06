#include "muli.cpp"
#include <sys/socket.h>
#include <sys/types.h>
#include <map>
#include "tcp_wrap.h"
#include <signal.h>
#include "debug_mode.h"
#include "socket_buffer.hpp"
#include "address_list.hpp"
#include "muli_sb.hpp"
;

static const char interrupt[] = {-1,-12,-1,-3,6};

muli<SocketBuffer> mi;
address_list AddressList;

class MainEvent{
public:
	inline void operator()(const int fd,SocketBuffer* sb)const{
		AddressList.dump();
		printf("dump( ");
		sb->dump();
		printf(")\n");
		
		if(sb->is_end()){
			fprintf(stderr,"socket closed.\n");
			close(fd);
			mi.remove(fd);
			return;
		}
		buffer hello("hello\n");
		*sb << hello << endl;
	}
};


class Accepter{
public:
	inline void operator()(int fd,SocketBuffer*)const{
		struct sockaddr_in addr;
		socklen_t addrlen;
		int newfd = accept(fd,(sockaddr*)(&addr),&addrlen);
		mi.add(newfd,MainEvent());
		AddressList.set_socket(address(addr.sin_addr.s_addr,addr.sin_port),newfd);
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
