#ifndef SOCKETBUFFER
#define SOCKETBUFFER
#include "buffer.hpp"


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
	class do_send{
	};

	SocketBuffer(const int socket):mSocket(socket),mRead(0),mChecked(0),mLeft(BUFFSIZE),mSize(BUFFSIZE),mBuff((char*)malloc(BUFFSIZE)),
								   sendLength(0),sendBuffSize(128),sendBuff((char*)malloc(128)){
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
	
	SocketBuffer& operator<<(const serializable& data){
		while(sendBuffSize - sendLength > data.getLength()){
			sendBuffSize *=2;
			sendBuff = (char*)realloc(sendBuff,sendBuffSize);
		}
		data.serialize(&sendBuff[sendLength]);
		sendLength += data.getLength();
		return *this;
	}
	SocketBuffer& operator<<(const do_send&){
		write(mSocket, sendBuff, sendLength);
		sendBuff = (char*)realloc(sendBuff,128);
		sendBuffSize = 128;
		sendLength = 0;
		return *this;
	}
	SocketBuffer& operator>>(serializable& buff){
		mChecked += buff.deserialize(&mBuff[mChecked]);
		return *this;
	}
};
static SocketBuffer::do_send endl;
#endif
