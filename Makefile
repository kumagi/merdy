CC=g++44 
OPTS=-Wall -W -std=c++0x -march=x86-64 -g
WARNS = -Wextra -Wformat=2 -Wstrict-aliasing=2 -Wcast-qual -Wcast-align \
	-Wwrite-strings -Wfloat-equal -Wpointer-arith -Wswitch-enum

target:poller_test
target:muli_test

poller_test:muli.cpp poller_test.cpp
	$(CC) poller_test.cpp -pthread -o poller_test $(OPTS) $(WARNS)
muli_test:muli.cpp muli_test.cpp
	$(CC) muli_test.cpp -pthread -o muli_test $(OPTS) $(WARNS)
