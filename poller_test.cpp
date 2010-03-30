#include "muli.cpp"

//#include "mytcplib.cpp"

int main(void){
	poller p;
	
	int accepter = create_tcpsocket();
	bind_inaddr_any(accepter,11011);
	listen(accepter,102);
	
	char buf[256];
	
	p.add_oneshot(accepter,poller::READ);
	
	std::vector<int> fds;
    p.wait(10,&fds);
	
	int length = read(accepter,buf,128);
	perror("read");
	
	printf("%d byte received, errno=%d\n",length,errno);
	for(int i=0;i<length;i++){
		printf("%d,",buf[i]);
	}
	printf("\n");
	
	return 0;
}
