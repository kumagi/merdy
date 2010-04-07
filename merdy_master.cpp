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

static const char interrupt[] = {-1,-12,-1,-3,6};

muli<SocketBuffer> mi;
address_list AddressList;

class Manager{
private:
	typedef serializable_int operation;
public:
	inline void operator()(int fd,SocketBuffer* sb){
		operation op;
		*sb << buffer("ok") << endl;
		*sb >> op;
		printf("data:%d\n",op.get());
	}
};

class Accepter{
public:
	inline void operator()(int fd,SocketBuffer*)const{
		struct sockaddr_in addr;
		socklen_t addrlen;
		int newfd = accept(fd,(sockaddr*)(&addr),&addrlen);
		mi.add(newfd,Manager());
		AddressList.set_socket(address(addr.sin_addr.s_addr,addr.sin_port),newfd);
		DEBUG_OUT("accept:%d\n",newfd);
	}
};

int main(void){
	int accepter = create_tcpsocket();
	set_reuse(accepter);
	bind_inaddr_any(accepter,11011);
	set_reuse(accepter);
	
	listen(accepter,102);
	
	mi.add(accepter,Accepter());
	mi.run();
	return 0;
}
