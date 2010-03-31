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


class memcached{
public:
	inline void operator()(const int fd,SocketBuffer* sb){
		AddressList.dump();
		printf("dump( ");
		sb->dump();
		printf(")\n");
		
		//char* head = sb->get_ptr();
		
		switch(mState){
		case _free:
		case _continue:
			if(sb->is_end()){ 
				mState = _close;
				break;
			}
			
			//fprintf(stderr,"before[%d]\n",mChecked);
			while(strncmp(sb->get_ptr(),"\r\n",2) != 0 && !sb->is_end()){
				sb->gain_ptr(1);
			}
			
			//fprintf(stderr,"after[%d]\n",mChecked);
			if(sb->is_end()){
				sb->back_ptr(5);
				if(strncmp(sb->get_ptr(),interrupt,5)){
					mState = _close;
					break;
				}
				fprintf(stderr,"interpret:%d\n ",*sb->get_ptr());
				mState = _continue;
				break;
			}
			*sb->get_ptr() = '\0';
			*(sb->get_ptr()+1) = '\0';
			
			//fprintf(stderr,"length:%d,[%s]\n",strlen(&mBuff[mStart]),&mBuff[mStart]);
			parse(sb->get_ptr());
			sb->gain_ptr(2);
		
			if(mState != _value){
				break;
			}
		case _value:
			//fprintf(stderr,"state value\n");
		
			if(strncmp(&mBuff[mStart + moreread],"\r\n",2) != 0 ){
				string_write("CLIENT_ERROR bad data chunk");
				mState = _error;
				while(strncmp(&mBuff[mChecked],"\r\n",2) != 0 && mChecked < mRead){
					mChecked++;
				}
				mChecked += 2;
				break;
			}
			tokennum++;
			mBuff[mStart + moreread] = '\0';
			mChecked += moreread + 2;
			tokens[SET_VALUE].str = &mBuff[mStart];
			tokens[SET_VALUE].length = moreread;
			tokens[SET_VALUE].str[moreread] = '\0';
			mStart += moreread + 2;
			mState = _set;
			break;
		default :
			mRead = mChecked;
			mState = _free;
			break;
		}
		//fprintf(stderr,"next state:%d\n",mState);
		return tokennum;
		
	}
private:
 	enum state {
		_free,
		_set,
		_get,
		_rget,
		_delete,
		_stats, // [stats] command 
		_value, // wait until n byte receive
		_continue, // not all data received
		_close,
		_error,
	};
	int mState;
	memcached():mState(_free){
		printf("created");
	}
	inline int parse(char* start){
		int cnt = 0;
		//fprintf(stderr,"parse start[%s]\n",start);
		if(strncmp(start,"set ",4) == 0){ // set [key] <flags> <exptime> <length>
			start += 3;
			mState = _value;
			cnt = read_tokens(start,4);
			if(cnt < 4){
				mState = _error;
				return cnt;
			}
			moreread = natoi(tokens[SET_LENGTH].str,tokens[SET_LENGTH].length);
			//fprintf(stderr,"waiting for value for %d length\n",moreread);
		}else if(strncmp(start,"get ",4) == 0){ // get [key] ([key] ([key] ([key].......)))
			mState = state_get;
			start += 3;
			cnt = read_tokens(start,30);
		}else if(strncmp(start,"rget ",5) == 0){ // rget [beginkey] [endkey] [left_closed] [right_closed]
			mState = state_rget;
			start += 4;
			cnt = read_tokens(start,4);
		}else if(strncmp(start,"delete ",7) == 0){ // delete [key] ([key] ([key] ([key].......)))
			mState = state_delete;
			start += 6;
			cnt = read_tokens(start,8);
		}else if(strncmp(start,"stats",4) == 0){ // stats
			mState = state_stats;
		}else if(strncmp(start,"quit",4) == 0){ // quit
			mState = state_close;
		}else{
			fprintf(stderr,"operation:%s\n",start);
			mState = state_error;
			//assert(!"invalid operation\n");
		}
		return cnt;
	}
	int natoi(char* str,int length){
		int ans = 0;
		while(length > 0){
			//fprintf(stderr,"%d:%d,",length,*str);
			assert('0' <= *str && *str <= '9' );
			ans = ans * 10 + *str - '0';
			str++;
			length--;
		}
		return ans;
	}
};

class Accepter{
public:
	inline void operator()(int fd,SocketBuffer*)const{
		struct sockaddr_in addr;
		socklen_t addrlen;
		int newfd = accept(fd,(sockaddr*)(&addr),&addrlen);
		mi.add(newfd,memcached());
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
