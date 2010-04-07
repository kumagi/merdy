#ifndef MULI_SOCKETBUFFER
#define MULI_SOCKETBUFFER

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
				poll.wait(16,&multi_fds); // polling
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
		items.add(fd,new cb_item(fd,func));
		poll.add_oneshot(fd,poller::READ);
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

#endif
