
//unsigned int cut_up(unsigned int i) __attribute__((always_inline));

unsigned int cut_up(unsigned int i){
	if(i == 0){return 1;}
	i <<= 1;
	while(i & (i-1)){
		i = i & (i-1);
	}
	return i;
}
