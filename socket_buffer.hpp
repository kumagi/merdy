#ifndef SOCKETBUFFER
#define SOCKETBUFFER
#include "buffer.hpp"
#include "tcp_wrap.h"


class SocketBuffer{
private:
	enum defaults{
		BUFFSIZE = 1024,
		FACTOR = 3,
	};
	const int mSocket;
	// receive buffer
	unsigned int mRead,mChecked,mLeft,mSize;
	char* mBuff;

	// send buffer
	unsigned int sendLength,sendBuffSize;
	char* sendBuff;
	
	SocketBuffer& operator=(const SocketBuffer&);
	SocketBuffer(const SocketBuffer&);
public:
	class do_send{};

	SocketBuffer(const int socket);
	~SocketBuffer(void);
	int read_max(void);
	int deserialize(void* buff,unsigned int length);
	int isLeft(void) const;
	char* get_ptr() const;
	void gain_ptr(const int delta);
	void back_ptr(const int delta);
	bool is_end(void)const;
	void dump()const;
	
	template<typename obj> 
	SocketBuffer& operator<<(const obj& data){
		assert(sendBuffSize != 0);
		while(sendBuffSize - sendLength < sizeof(obj)){
			sendBuffSize *=2;
			sendBuff = (char*)realloc(sendBuff,sendBuffSize);
		}
		obj* objp = (obj*)sendBuff[sendLength];
		*objp = data;
		sendLength += sizeof(obj);
		return *this;
	}
	SocketBuffer& operator<<(const do_send&);
	SocketBuffer& operator<<(const std::string& data);
	SocketBuffer& operator<<(const serializable& data);
	
	template<typename obj> 
	SocketBuffer& operator>>(obj& data){
		data = *(obj*)&mBuff[mChecked];
		mChecked += sizeof(obj);
		return *this;
	}
	SocketBuffer& operator>>(std::string& data);
	SocketBuffer& operator>>(serializable& buff);
	
};
class write_poller;
	//class write_poller wp;

extern SocketBuffer::do_send endl;

#endif
