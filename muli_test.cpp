#include "muli.cpp"
#include <sys/socket.h>
#include <sys/types.h>
#include <map>
#include "tcp_wrap.h"
#include <signal.h>

static const char interrupt[] = {-1,-12,-1,-3,6};

muli<SocketBuffer> mi;

// TODO address list
class address{
private:
	int ip;
	unsigned short port;
public:
	address():ip(aton("127.0.0.1")),port(11211){};
	address(const int _ip,const unsigned short _port):ip(_ip),port(_port){};
	address(const address& ad):ip(ad.ip),port(ad.port){}
	bool operator<(const address& rhs)const{
		return ip < rhs.ip;
	}
	int get_ip()const {return ip;}
	void dump(void)const{
		printf("[%s:%d]", ntoa(ip),port);
	}
	unsigned short get_port()const {return port;}
};

class address_list{
private:
	std::map<address,int> addmap;
	pthread_mutex_t mut;
	class lock{
		pthread_mutex_t* l;
	public:
		lock(pthread_mutex_t* _l):l(_l){ pthread_mutex_lock(l);}
		~lock(){ pthread_mutex_unlock(l);}
	};
public:
	int get_socket(const address& ad) const {
		std::map<address,int>::const_iterator it = addmap.find(ad);
		if(it == addmap.end()){
			return 0;
		}
		return it->second;
	}
	int get_socket_force(const address& ad){
		lock start(&mut);
		std::map<address,int>::const_iterator it = addmap.find(ad);
		if(it == addmap.end()){
			int newsocket = create_tcpsocket();
			int result = connect_port_ip(newsocket,ad.get_ip(),ad.get_port());
			if(result) {assert(!"connect error.");}
			addmap.insert(std::pair<address,int>(ad,newsocket));
		}
	}
	void set_socket(const address& ad,int socket) {
		lock start(&mut);
		addmap.insert(std::pair<address,int>(ad,socket));
	}
	void dump(void)const{
		std::map<address,int>::const_iterator it = addmap.begin();
		while(it != addmap.end()){
			it->first.dump();
			printf("fd:(%d) ",it->second);
			++it;
		}
	}
};
address_list AddressList;



class MainEvent{
public:
	inline void operator()(const int fd,SocketBuffer* sb)const{
		AddressList.dump();
		printf("dump( ");
		sb->dump();
		printf(")\n");
		
		//char* head = sb->get_ptr();
		
		
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
