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

		const int nfds = epoll_wait(epollfd, events, maxevent, -1);
		if (nfds == -1) {
			perror("epoll_wait");
			exit(1);
		}
		for(int i = 0;i < nfds; i++){
			fds->push_back(events[i].data.fd);
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
	typedef std::function<void (int, T&)> cb_func;
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
public:
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
			try{
				it->func(WorkFd,it->item); // do work
				poll.reset_oneshot(WorkFd,poller::READ);
			}catch(...){
				items.remove(WorkFd);
			}
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
};
