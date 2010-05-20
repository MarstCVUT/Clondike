static inline int my_atoi(const char *buf, const int len, unsigned long *num){
	int i;
	*num = 0;

	for( i = 0; i < len; i++ ){
		if( buf[i] <= '9' && buf[i] >= '0' ){
			*num *= 10;
			*num += buf[i] - '0';
		}
		else return -1;
	}
	return 0;
}
