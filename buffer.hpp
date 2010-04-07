#include <string>

// interface
class serializable{
public:
	virtual unsigned int serialize(char* const ptr) const = 0;
	virtual unsigned int deserialize(const char* const ptr) = 0;
	virtual unsigned int getLength(void) const = 0;
};



// buffer format
// | length | data |
// | 4bytes | [length] bytes |
class buffer :public serializable{
private:
	int length;
	char* data;
	
	inline int copy(const int _length,const char* const _data){
		length = _length;
		if(data){
			free(data);
		}
		data = (char*)malloc(length);
		memcpy(data,_data,length);
		return length;
	}
public:
	buffer(void):length(0),data(NULL){}
	buffer(const int _length, char* const _data):length(_length),data(0){
		copy(_length,_data);
	}
	buffer(const buffer& _buffer):length(_buffer.length),data(0){
		copy(length,_buffer.data);
	}
	buffer(const std::string str):length(str.length()),data(0){
		copy(length, str.data());
	}
	unsigned int serialize(char* const ptr) const {
		unsigned int* const intptr = (unsigned int*)ptr;
		*intptr = length;
		memcpy(ptr+4, data, length);
		return length;
	}
	unsigned int deserialize(const char* const ptr){ // give data pointer, it reads buffer
		copy(*(int*)ptr, ptr+4);
		return *(int*)ptr;
	}
	unsigned int getLength()const{ return length+4; }
	buffer& operator=(const buffer& rhs){
		copy(rhs.length, rhs.data);
		return *this;
	}
	~buffer(){
		if(data){
			free(data);
		}
	}
};

class serializable_int : public serializable{
	int data;
public:
	unsigned int serialize(char* const ptr) const {
		int* intp = (int*)ptr;
		*intp = data;
		return 4;
	}
	unsigned int deserialize(const char* const ptr){
		data = *(int*)ptr;
		return 4;
	}
	unsigned int getLength()const{
		return 4;
	}
	serializable_int& operator=(int _data){
		data = _data;
		return *this;
	}
	serializable_int& operator=(serializable_int _data){
		data = _data.data;
		return *this;
	}
	int get()const{
		return data;
	}
};

class serializable_string : public serializable{
	std::string str;
	
public:
	serializable_string(std::string _str):str(_str){ }
	serializable_string& operator=(std::string _str){
		str = _str;
		return *this;
	}
	serializable_string& operator=(serializable_string _str){
		str = _str.str;
		return *this;
	}
	unsigned int serialize(char* const ptr) const {
		for(unsigned int i=0;i<str.length();i++){
			*ptr = str.data()[i];
		}
		return str.length();
	}
	unsigned int deserialize(const char* const ptr){
		int i;
		for(;ptr[i]!='\0';i++);
		str.assign(ptr,0,i);
		return i;
	}
	unsigned int getLength()const{
		return str.length();
	}
};
