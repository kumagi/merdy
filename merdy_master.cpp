#include "muli.cpp"
#include <sys/socket.h>
#include <sys/types.h>
#include <map>
#include <vector>
#include <list>
#include <boost/program_options.hpp>
#include <boost/random.hpp>
#include <iostream>
#include "tcp_wrap.h"
#include <signal.h>
#include "debug_mode.h"
#include "socket_buffer.hpp"
#include "address_list.hpp"
#include "muli_sb.hpp"
#include "merdy_operations.h"
#include "dy_data.hpp"

#include "MurmurHash2_64.cpp" // MIT Licence

#if __x86_64__ || _WIN64
#define _64BIT
#define _ptr long long
#else
#define _32BIT
#define _ptr int
#endif


// hash function
#ifdef _64BIT
template<typename T> 
long long hash_value(const T& obj,unsigned int seed=0){
	return MurmurHash64A(&obj, sizeof(obj), seed);
}
long long hash_value(const std::string& data, unsigned int seed=0){
	return MurmurHash64A(data.c_str(),data.length(), seed);
}
#else
template<typename T> 
long long hash_value(const T& obj,unsigned int seed=0){
	return MurmurHash64B(&obj, sizeof(obj), seed);
}
long long hash_value(const std::string& data, unsigned int seed=0){
	return MurmurHash64B(data.c_str(),data.length(), seed);
}
#endif


static const char interrupt[] = {-1,-12,-1,-3,6};

static struct settings{
	int verbose;
	unsigned short myport,masterport;
	int myip,masterip;
	int i_am_master;
	settings():verbose(10),myport(11011),masterport(11011),myip(get_myip()),masterip(aton("127.0.0.1")),i_am_master(1){}
}settings;

// init randoms
boost::mt19937 gen(static_cast<unsigned long>(time(NULL)));
boost::uniform_smallint<> dst(0,INT_MAX);
boost::variate_generator<boost::mt19937&, boost::uniform_smallint<> > mt_rand(gen,dst);
inline long long rand64(void){
	long long tmp = mt_rand();
	tmp <<= 32;
	tmp |= mt_rand();
	return tmp;
}


std::map<dy_data,dy_data> dy_map;

	
muli<SocketBuffer> mi;
address_list AddressList;

std::set<address> dy_address;
//std::list<address> mer_address;

enum dynamo_statics{
	DY_TOKENS = 256,
};

std::map<long long,const address*> consistent_hash;

class main_machine{
private:
	typedef serializable_int operation;
public:
	inline void operator()(int fd,SocketBuffer* sb){
		int op;
		if(sb->is_end()){
			fprintf(stderr,"closed\n");
			throw "fd closed\n";
		}
		
		*sb >> op; // deserialize operation
		switch(op){
			// manager node operations
		case op::SEND_DY_LIST:{
			int op = op::UPDATE_HASHES;
			std::map<long long, const address*>::iterator it = consistent_hash.begin();
			*sb << op << consistent_hash.size();
			while(it != consistent_hash.end()){
				*sb << it->first << it->second;;
				++it;
			}
			*sb << endl; // do send.
			break;
		}
			
		case op::ADD_ME_DY:{
			address newadd;
			*sb >> newadd;
			
			const std::pair<std::set<address>::iterator,bool> it = dy_address.insert(newadd);
			for(int i=0;i<8;i++){
				consistent_hash.insert(std::pair<long long,const address*>(rand64(),&*(it.first)));
			}
			
			*sb << op::OK_ADD_ME_DY << endl;
			break;
		}
			
			// dynamo node operations
		case op::UPDATE_HASHES:{
			int nums;
			consistent_hash.clear();
			*sb >> nums;
			for(int i=0; i < nums; i++){
				address ad;
				long long hash;
				*sb >> ad >> hash;
				const std::pair<std::set<address>::iterator,bool> it = dy_address.insert(ad);
				consistent_hash.insert(std::pair<long long,const address*>(hash, &*(it.first)));
			}
			break;
		}
			
		case op::OK_ADD_ME_DY:{
			*sb << op::SEND_DY_LIST << endl;
			break;
		}
			
		case op::SET_DY:{
			dy_data key,value;
			*sb >> key >> value;
			long long hash = hash_value(key) & ~((DY_TOKENS<<1) - 1);
			std::map<long long, const address*>::const_iterator it = consistent_hash.upper_bound(hash);
			const address* coordinator = it->second;
			int fd = AddressList.get_socket(*coordinator);
			if(fd == 0){
				fd = AddressList.get_socket_force(*coordinator);
			}
			SocketBuffer cdnt(fd);
			cdnt << op::SET_COORDINATE << key << value << endl;
			break;
		}
		case op::PUT_DY:{
			dy_data key,value;
			*sb >> key >> value;
			dy_map.insert(std::pair<dy_data,dy_data>(key,value));
			*sb << op::OK_PUT_DY << endl;
			break;
		}
			
		case op::GET_DY:{
			
			break;
		}
			
		case op::DEL_DY:{
			
			break;
		}
			
		case op::SET_COORDINATE:{
			dy_data key,value;
			*sb >> key >> value;
			dy_map.insert(std::pair<dy_data,dy_data>(key,value));
			address putter[3];
			putter[0] = address(settings.myip,settings.myport);
			long long hash = hash_value(key);
			std::map<long long, const address*>::const_iterator it = consistent_hash.upper_bound(hash);
			assert(*it->second == address(settings.myip,settings.myport));
			int putters = 0;
			for(int i=1;i<3;){
				for(int j=0;j<i;++j){
					if(*it->second == putter[j])
						goto overlap;
				}
				if(*it->second != putter[i]){
					putter[i] = *it->second;
					++i;
				}
			overlap:
				++it;
				++putters;
				if(it == consistent_hash.end()){
					break;
				}
			}
			for(int i=1;i<putters;++i){
				int fd = AddressList.get_socket(putter[i]);
				if(fd == 0){
					fd = AddressList.get_socket_force(putter[i]);
				}
				SocketBuffer put(fd);
				put << op::PUT_DY << key << value << endl;
			}
			break;
		}
		}
	}
};

class Accepter{
public:
	inline void operator()(int fd,SocketBuffer*)const{
		struct sockaddr_in addr;
		socklen_t addrlen;
		int newfd = accept(fd,(sockaddr*)(&addr),&addrlen);
		mi.add(newfd,main_machine());
		AddressList.set_socket(address(addr.sin_addr.s_addr,addr.sin_port),newfd);
		DEBUG_OUT("accept:%d\n",newfd);
	}
};

namespace po = boost::program_options;


int main(int argc, char* argv[]){
	
	// parse options
	po::options_description opt("options");
	std::string master;
	opt.add_options()
		("help,h", "view help")
		("verbose,v", "verbose mode")
		("port,p",po::value<unsigned short>(&settings.myport)->default_value(11011), "my port number")
		("address,a",po::value<std::string>(&master)->default_value("127.0.0.1"), "master's address")
		("mport,P",po::value<unsigned short>(&settings.masterport)->default_value(11011), "master's port");
	
	po::variables_map vm;
	store(parse_command_line(argc,argv,opt), vm);
	notify(vm);
	if(vm.count("help")){
		std::cout << opt << std::endl;
		return 0;
	}
	
	// set options
	if(vm.count("verbose")){
		settings.verbose++;
	}
	settings.masterip = aton(master.c_str());
	if(settings.masterip != aton("127.0.0.1") || settings.myport != settings.masterport){
		settings.i_am_master = 0;
	}
	
	// view options
	printf("verbose:%d\naddress:[%s:%d]\n",
		   settings.verbose,ntoa(settings.myip),settings.myport);
	printf("master:[%s:%d] %s\n"
		   ,ntoa(settings.masterip),settings.masterport,settings.i_am_master?"self!":"");
	
	address my_address(settings.myip,settings.myport);
	
	// connect to master
	if(!settings.i_am_master){
		address master_address(settings.masterip,settings.masterport);
		int master_socket = AddressList.get_socket_force(master_address);
		SocketBuffer to_master(master_socket);
		to_master << op::ADD_ME_DY << my_address << endl;
		to_master << op::SEND_MER_LIST << endl;
		mi.add(master_socket,main_machine());
	}
	
	// create listen socket
	int accepter = create_tcpsocket();
	set_reuse(accepter);
	bind_inaddr_any(accepter,11011);
	set_reuse(accepter);
	listen(accepter,102);
	
	// start
	mi.add(accepter,Accepter());
	mi.run();
	return 0;
}
