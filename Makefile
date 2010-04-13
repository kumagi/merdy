CC=g++44 
OPTS=-Wall -W -std=c++0x -fexceptions  -march=x86-64 -g
WARNS = -Wextra -Wformat=2 -Wstrict-aliasing=2 -Wcast-qual -Wcast-align \
	-Wwrite-strings -Wfloat-equal -Wpointer-arith -Wswitch-enum
OBJS=socket_buffer.o tcp_wrap.o

#target:poller_test
#target:muli_test
target:merdy_master
target:setter

merdy_master:merdy_master.o socket_buffer.o muli.o tcp_wrap.o
	$(CC) merdy_master.o $(OBJS) -pthread -o merdy_master -lboost_program_options $(OPTS) $(WARNS)

setter:setter.o socket_buffer.o tcp_wrap.o
	$(CC) setter.o $(OBJS) -pthread -o setter -lboost_program_options $(OPTS) $(WARNS)

setter.o:setter.cpp
	$(CC) setter.cpp -o setter.o -c $(OPTS) $(WARNS)

merdy_master.o:merdy_master.cpp socket_buffer.hpp
	$(CC) merdy_master.cpp  -o merdy_master.o -c $(OPTS) $(WARNS)

muli.o:muli.cpp #muli.hpp 
	$(CC) muli.cpp -o muli.o -c $(OPTS) $(WARNS)

socket_buffer.o:socket_buffer.hpp socket_buffer.cpp
	$(CC) socket_buffer.cpp -o socket_buffer.o -c $(OPTS) $(WARNS)

tcp_wrap.o:tcp_wrap.h tcp_wrap.cpp
	$(CC) tcp_wrap.cpp -o tcp_wrap.o -c $(OPTS) $(WARNS)


poller_test:muli.cpp poller_test.cpp 
	$(CC) poller_test.cpp -pthread -o poller_test $(OPTS) $(WARNS)
muli_test:muli.cpp muli_test.cpp buffer.hpp socket_buffer.hpp
	$(CC) muli_test.cpp -pthread -o muli_test $(OPTS) $(WARNS)