


class dy_data : public serializable{
	// it stores int or string
	char int_or_string; // 0 = int, 1 = string
	int int_data;
	std::string str_data;
	friend long long hash_value(const dy_data& data);
public:
	dy_data():int_or_string(TYPE_OTHER),int_data(0),str_data(){}
	dy_data(int _int_data):int_or_string(TYPE_INT),int_data(_int_data),str_data(){}
	dy_data(const std::string& _str_data):int_or_string(TYPE_STR),int_data(0),str_data(_str_data){};
	dy_data(const dy_data& _org):int_or_string(_org.int_or_string),int_data(_org.int_or_string),str_data(_org.str_data){}
	
	dy_data& operator=(const dy_data& _org){
		int_or_string = _org.int_or_string;
		int_data = _org.int_data;
		str_data = _org.str_data;
		return *this;
	}
	bool operator<(const dy_data& rhs)const{
		if(int_or_string < rhs.int_or_string){return true;}
		if(int_or_string > rhs.int_or_string){return false;}
		if(int_or_string){return str_data < rhs.str_data;}
		else{return int_data < rhs.int_data;}
	}
	bool operator==(const dy_data& rhs)const{
		return int_or_string == rhs.int_or_string
			&& int_data == rhs.int_data
			&& str_data == rhs.str_data;
	}
	unsigned int serialize(char* ptr) const{
		*ptr++ = int_or_string;
		if(int_or_string == TYPE_INT){
			*ptr = int_data;
			return 5;
		}else{
			*ptr = str_data.length();
			ptr += 4;
			memcpy(ptr,str_data.c_str(),str_data.length());
			return 1 + 4 + str_data.length();
		}
	}
	unsigned int deserialize(const char* ptr){
		int_or_string = *ptr++;
		if(int_or_string == TYPE_INT){
			int_data = *(int*)ptr;
			str_data.clear();
			return 5;
		}else{
			int length = *(int*)ptr;
			int_data = 0;
			str_data.assign(ptr,4,length);
			return 1 + 4 + length;
		}
	}
	unsigned int getLength(void) const{
		if(int_or_string == TYPE_INT){
			return 5;
		}else{
			return 1 + 4 + str_data.length();
		}
	}	
};

long long hash_value(const dy_data& data){
	if(data.int_or_string == TYPE_INT){
		return hash_value(data.int_data);
	}else{
		return hash_value(data.str_data);
	}
}
