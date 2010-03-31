#include <sys/epoll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <unistd.h>
#include <list>
#include <assert.h>
#include <errno.h>
#include <functional>

#include "minilib.h"
#include "item_pool.hpp"
#include "tcp_wrap.h"

template<typename T>
class atomic_vector{
private:
	std::vector<T>  items;
	pthread_mutex_t vector_mutex;
	
	class lock{
		pthread_mutex_t& mutex;
	public:
		lock(pthread_mutex_t& _mutex):mutex(_mutex){
			pthread_mutex_lock(&mutex);
		}
		~lock(){
			pthread_mutex_unlock(&mutex);
		}
	};
public:
	atomic_vector(const int length):items(length),vector_mutex(PTHREAD_MUTEX_INITIALIZER){
	}
	~atomic_vector(){
		items.clear();
		pthread_mutex_destroy(&vector_mutex);
	}
	T& operator[](int index){
		return items[index];
	}
	T& push_back(const T& item){
		lock start(vector_mutex); // it locks!
		items.push_back(item);
	}
	void reserve(const int size){
		lock start(vector_mutex); // it locks!
		items.reserve(size);
	}
	void clear(){
		items.clear();
	}
	int size() const { return items.size();}
	bool empty() const { return items.empty(); }
};

class poller{
public:
	enum event{
		READ = EPOLLIN,
		WRITE = EPOLLOUT,
	};
	poller():epollfd(epoll_create(15)),ListSize(0),TableSize(256),FdList(256),FdTable(256){
		epollfd = epoll_create(10);
		if (epollfd == -1) {
			perror("epoll_create");
			exit(1);
		}
	}
	void add(const int fd, const event ev){
		if(fd > TableSize){
			FdTable.reserve(cut_up(fd));
		}
		FdTable[fd] = 1;
		epoll_event tmp;
		tmp.events = static_cast<int>(ev) | EPOLLET;
		tmp.data.fd = fd;
		if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &tmp) == -1) {
			perror("epoll_ctl: add fd.");
			exit(1);
		}
	}
	void remove(const int fd){
		FdTable[fd] = 0;
		struct epoll_event tmp;
		tmp.data.fd = fd;
		if (epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &tmp) == -1) {
			perror("epoll_ctl: remove fd.");
			exit(1);
		}
	}
	void add_oneshot(const int fd, const event ev){
		add(fd, static_cast<event>(static_cast<int>(ev) | EPOLLONESHOT));
	}
	void reset_oneshot(const int fd, const event ev){
		FdTable[fd] = 1;
		epoll_event tmp;
		tmp.events = static_cast<int>(ev) | EPOLLET | EPOLLONESHOT;
		tmp.data.fd = fd;
		if (epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &tmp) == -1) {
			perror("epoll_ctl: add fd.");
			exit(1);
		}
	}
	
	int wait(int maxevent, std::vector<int>* fds){
		epoll_event events[maxevent]; 
	awaked:
		const int nfds = epoll_wait(epollfd, events, maxevent, -1);
		if (nfds == -1) {
			perror("epoll_wait");
			exit(1);
		}
		for(int i = 0;i < nfds; i++){
			int dummy;
			if(events[i].data.fd == awaker[0]){
				read(awaker[0],&dummy,1);
				if(fds->empty()) {goto awaked;}
			}else{
				fds->push_back(events[i].data.fd);
			}
		}
		return nfds;
	}
private:
	int epollfd,ListSize,TableSize;
	atomic_vector<int> FdList;
	atomic_vector<int> FdTable;
	int awaker[2];
};


template<typename T>
class muli{
public:
	typedef std::function<void (int, T&)> cb_func;
	muli():poll(),poll_mutex(PTHREAD_MUTEX_INITIALIZER),items(),multi_fds(){};
	void run(){
		int WorkFd = 0xdeadbeef;
		while(1){
			pthread_mutex_lock(&poll_mutex);
			if(!multi_fds.empty()){
				WorkFd = multi_fds.back();
				multi_fds.pop_back();
			}else{
				poll.wait(16,&multi_fds);
				WorkFd = multi_fds.back();
				multi_fds.pop_back();
			}
			pthread_mutex_unlock(&poll_mutex);
			
			cb_item* it = items.get(WorkFd);
			it->func(WorkFd,it->item); // do work
		
			poll.reset_oneshot(WorkFd,poller::READ);
		}
	}
	void add(const int fd,cb_func func){
		lock start(&poll_mutex);
		cb_item* it = items.add(fd,new cb_item(fd,func));
		it->func = func;
		poll.add_oneshot(fd,poller::READ);
	}
	void remove(const int fd){
		lock start(&poll_mutex);
		poll.remove(fd);
		items.remove(fd);
	}
	~muli(){
		pthread_mutex_destroy(&poll_mutex);
		multi_fds.clear();
	}
private:
	class cb_item{
	public:
		T item;
		cb_func func;
		cb_item(const T& _item,const cb_func& _cb_func):item(_item),func(_cb_func){};
	private:
		cb_item():item(),func(){};
		cb_item(const cb_item&);
		cb_item& operator=(const cb_item&);
	};
	class lock{
		pthread_mutex_t* l;
	public:
		lock(pthread_mutex_t* _l):l(_l){ pthread_mutex_lock(l);}
		~lock(){ pthread_mutex_unlock(l);}
	};
	
	poller poll;
	pthread_mutex_t poll_mutex;
	item_pool<cb_item,32> items;
	std::vector<int> multi_fds;
	
};



class SocketBuffer{
private:
	enum defaults{
		BUFFSIZE = 1024,
		FACTOR = 3,
	};
	const int mSocket;
	unsigned int mRead,mChecked,mLeft,mSize;
	char* mBuff;
	
	SocketBuffer& operator=(const SocketBuffer&);
	SocketBuffer(const SocketBuffer&);
public:
	SocketBuffer(const int socket):mSocket(socket),mRead(0),mChecked(0),mLeft(BUFFSIZE),mSize(BUFFSIZE),mBuff((char*)malloc(BUFFSIZE)){
		set_nonblock(socket);
	}
	~SocketBuffer(void){
		close(mSocket);
		free(mBuff);
	}
	int read_max(void){
		int readsize;
		while(1){
			if(mLeft == 0){
				mBuff = (char*)realloc(mBuff,mSize*FACTOR);
				mLeft = mSize * (FACTOR - 1);
				mSize *= FACTOR;
			}
			readsize = read(mSocket,&mBuff[mRead],mLeft);
			if(readsize > 0){
				mLeft -= readsize;
				mRead += readsize;
			}
			if(errno == EAGAIN || errno == EWOULDBLOCK || readsize <= 0){
				break;
			}
		}
		return readsize;
	}
	int deserialize(void* buff,unsigned int length){
		assert(length < mLeft - mChecked);
		memcpy(buff,&mBuff[mChecked],length);
		mChecked += length;
		return length;
	}
	int isLeft(void) const{
		return mChecked == mRead;
	}
	char* get_ptr() const {
		return &mBuff[mChecked];
	}
	void gain_ptr(const int delta){
		mChecked += delta;
	}
	void back_ptr(const int delta){
		mChecked -= delta;
	}
	bool is_end(void)const{
		return mRead == mChecked;
	}
	void dump()const{
		for(unsigned int i=0;i<mChecked;i++){
			printf("%d,",mBuff[i]);
		}
		printf("[%d],",mBuff[mChecked]);
		for(unsigned int i=mChecked+1;i<mRead;i++){
			printf("%d,",mBuff[i]);
		}
		
	}
};


template <>
class muli<SocketBuffer>{
public:
	typedef std::function<void (int, SocketBuffer*)> cb_func;
	muli():poll(),poll_mutex(PTHREAD_MUTEX_INITIALIZER),items(),multi_fds(){};
	void run(){
		int WorkFd = 0xdeadbeef;
		while(1){
			pthread_mutex_lock(&poll_mutex);
			if(!multi_fds.empty()){
				WorkFd = multi_fds.back();
				multi_fds.pop_back();
			}else{
				poll.wait(16,&multi_fds);
				WorkFd = multi_fds.back();
				multi_fds.pop_back();
			}
			pthread_mutex_unlock(&poll_mutex);
			
			cb_item* it = items.get(WorkFd);
			
			it->item.read_max();
			
			it->func(WorkFd,&it->item); // do work
			
			poll.reset_oneshot(WorkFd,poller::READ);
		}
	}
	void add(const int fd,cb_func func){
		cb_item* it = items.add(fd,new cb_item(fd,func));
		poll.add_oneshot(fd,poller::READ);
		it->func = func;
	}
	void remove(const int fd){
		items.remove(fd);
		poll.remove(fd);
	}
private:
	class cb_item{
	public:
		SocketBuffer item;
		cb_func func;
		cb_item(const int fd,const cb_func& _cb_func):item(fd),func(_cb_func){};
	private:
		//cb_item():item(),func(){};
		cb_item(const cb_item&);
		cb_item& operator=(const cb_item&);
	};
	
	poller poll;
	pthread_mutex_t poll_mutex;
	item_pool<cb_item,32> items;
	std::vector<int> multi_fds;
	
};
