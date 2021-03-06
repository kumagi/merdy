#include "tcp_wrap.h"
#include <boost/functional/hash.hpp>

class address : public serializable {
private:
	int ip;
	unsigned short port;
	friend size_t hash_value(const address& ad);
public:
	address():ip(aton("127.0.0.1")),port(11211){};
	address(const int _ip,const unsigned short _port):ip(_ip),port(_port){};
	address(const address& ad):ip(ad.ip),port(ad.port){}
	bool operator<(const address& rhs)const{
		return ip < rhs.ip && port < rhs.port;
	}
	int get_ip()const {return ip;}
	void dump(void)const{
		printf("[%s:%d]", ntoa(ip),port);
	}
	unsigned short get_port()const {return port;}
	
	unsigned int serialize(char* const ptr) const{
		int* int_ptr = (int*)ptr;
		*int_ptr = ip;
		unsigned short* short_ptr = (unsigned short*)(ptr+4);
		*short_ptr = port;
		return 6;
	}
	unsigned int deserialize(const char* const ptr){
		ip = *(int*)ptr;
		port = *(unsigned short*)(ptr+2);
		return 6;
	}
	unsigned int getLength()const {return 6;}
	bool operator==(const address& rhs)const {
		return ip == rhs.ip && port == rhs.port;
	}
	bool operator!=(const address& rhs)const{
		return ip != rhs.ip || port != rhs.port;
	}
};

size_t hash_value(const address& ad){
	size_t h = 0;
	boost::hash_combine(h, ad.ip);
	boost::hash_combine(h, ad.port);
	return h;
}


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
		int socket;
		std::map<address,int>::const_iterator it = addmap.find(ad);
		if(it == addmap.end()){
			socket = create_tcpsocket();
			int result = connect_ip_port(socket,ad.get_ip(),ad.get_port());
			if(result) {assert(!"connect error.");}
			addmap.insert(std::pair<address,int>(ad,socket));
		}else{
			socket = it->second;
		}
		return socket;
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
