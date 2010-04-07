CC=g++44 
OPTS=-Wall -W -std=c++0x -march=x86-64 -g
WARNS = -Wextra -Wformat=2 -Wstrict-aliasing=2 -Wcast-qual -Wcast-align \
	-Wwrite-strings -Wfloat-equal -Wpointer-arith -Wswitch-enum

target:poller_test
target:muli_test
target:merdy_master

merdy_master:merdy_master.cpp muli.cpp tcp_wrap.h debug_mode.h socket_buffer.hpp address_list.hpp muli_sb.hpp
	$(CC) merdy_master.cpp -pthread -o merdy_master $(OPTS) $(WARNS)
poller_test:muli.cpp poller_test.cpp
	$(CC) poller_test.cpp -pthread -o poller_test $(OPTS) $(WARNS)
muli_test:muli.cpp muli_test.cpp buffer.hpp socket_buffer.hpp
	$(CC) muli_test.cpp -pthread -o muli_test $(OPTS) $(WARNS)
