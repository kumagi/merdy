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
#include <stdio.h>
#include "merdy_operations.h"
#include "dy_data.hpp"


int main(int argc, char** argv){
	if(argc != 2){
		fprintf(stderr,"set argument [ip] [port]\n");
		exit(0);
	}
	int fd = create_tcpsocket();
	connect_ip_port(fd,aton(argv[1]),atoi(argv[2]));
	SocketBuffer target(fd);
	
	target << op::SET_DY << dy_data("hoge") << dy_data("fuga") << endl;
	
}



