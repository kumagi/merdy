#include "socket_buffer.hpp"


SocketBuffer::SocketBuffer(const int socket):mSocket(socket),mRead(0),mChecked(0),mLeft(BUFFSIZE),mSize(BUFFSIZE),mBuff((char*)malloc(BUFFSIZE)),
											 sendLength(0),sendBuffSize(128),sendBuff((char*)malloc(128)){
	set_nonblock(socket);
}
SocketBuffer::~SocketBuffer(void){
		free(mBuff);
	}
int SocketBuffer::read_max(void){
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
int SocketBuffer::deserialize(void* buff,unsigned int length){
	assert(length < mLeft - mChecked);
	memcpy(buff,&mBuff[mChecked],length);
	mChecked += length;
	return length;
}
int SocketBuffer::isLeft(void) const{
	return mChecked == mRead;
}
char* SocketBuffer::get_ptr() const {
	return &mBuff[mChecked];
}
void SocketBuffer::gain_ptr(const int delta){
	mChecked += delta;
}
void SocketBuffer::back_ptr(const int delta){
		mChecked -= delta;
}
bool SocketBuffer::is_end(void)const{
	return mRead == mChecked;
}
void SocketBuffer::dump()const{
	for(unsigned int i=0;i<mChecked;i++){
		printf("%d,",mBuff[i]);
	}
	printf("[%d],",mBuff[mChecked]);
	for(unsigned int i=mChecked+1;i<mRead;i++){
		printf("%d,",mBuff[i]);
	}
}
	
SocketBuffer& SocketBuffer::operator<<(const serializable& data){
	assert(sendBuffSize != 0);
	while(sendBuffSize - sendLength < data.getLength()){
		sendBuffSize *=2;
		sendBuff = (char*)realloc(sendBuff,sendBuffSize);
	}
	sendLength += data.serialize(&sendBuff[sendLength]);
	return *this;
}
SocketBuffer& SocketBuffer::operator<<(const std::string& data){
	while(sendBuffSize - sendLength < data.length()){
		sendBuffSize *=2;
		sendBuff = (char*)realloc(sendBuff,sendBuffSize);
	}
	memcpy(&sendBuff[sendLength],data.c_str(),data.length());
	sendLength += data.length();
	return *this;
}

SocketBuffer& SocketBuffer::operator<<(const do_send&){
	write(mSocket, sendBuff, sendLength);
	sendBuff = (char*)realloc(sendBuff,128);
	sendBuffSize = 128;
	sendLength = 0;
	return *this;
}
SocketBuffer& SocketBuffer::operator>>(serializable& buff){
	mChecked += buff.deserialize(&mBuff[mChecked]);
	return *this;
}
SocketBuffer& SocketBuffer::operator>>(std::string& data){
	int length = *(int*)&mBuff[mChecked];
	mChecked += 4;
	data.assign(mBuff,mChecked,mChecked);
	mChecked += data.length();
	return *this;
}

SocketBuffer::do_send endl;
