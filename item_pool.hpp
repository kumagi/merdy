
#include <vector>
#include <list>
#include <assert.h>

template<typename T,int bufsize=32>
class item_pool{
	std::list<T*> buffers;
	std::vector<T*> table;
	
public:
	item_pool(void):buffers(),table(bufsize,NULL){
		buffers.push_back(NULL);
	}
	T* add(const int socket, T* item){
		T* it; 
		while(! (socket < (int)table.size())){
			table.resize(table.size() * 2, NULL);
		}
		if(table[socket] != NULL){
			it = table[socket];
		}else{
			it = item;
			buffers.push_back(it);
			table[socket] = it;
		}
		return it;
	}
	T* get(const int socket){
		T* it; 
		if(table[socket] != NULL){
			it = table[socket];
		}else{
			assert(!"not exist item");
		}
		return it;
	}
	void remove(const unsigned int socket){
		if(table[socket]){
			buffers.remove(table[socket]);
			delete table[socket];
			table[socket] = NULL;
		}else {
			assert(!"unexist item");
		}
		table[socket] = 0;
	}
};
/* test code

#include <stdlib.h>
#include <stdio.h>
class item{
public:
	int a;
	int b;
	item(int _a):a(_a),b(rand()%255){};
	void print(void) const{
		printf("%d:%d\n",a,b);
	}
};

#define MAX 129
int main(void){
	const int times = MAX;
	int buff[MAX];
		
	item_pool<item,8> ip;
	for(int i = 0;i<times;i++){
		item* a = ip.get(i);
		//a->print();
		buff[i] = a->b;
	}
	for(int i=times-1;i>=0;i--){
		item* a = ip.get(i);
		//a->print();
		assert(buff[i] == a->b);
	}
	printf("item_pool test ok.\n");
}

//*/
