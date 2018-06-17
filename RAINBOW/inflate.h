enum INFLENUM {
	INFLENUM_OK=1,
	INFLENUMERR_INVALIDBTYPE = -99,
	INFLENUMERR_GT18,
	INFLENUMERR_INPUT,		// TODO: error reading input
	//INFLATEERR_NONZLIBDATA,// inflate has nothing to do with ZLIB anymore
};

struct JDHT {		// JPEG-style Huffman Definition Table 
	int *b;			// pointer to cumulative # of symbols [15]
	int *s;			// pointer to symbols
	//int min;		// opt for first code?
};

struct INFLATE {

	unsigned char *dst;			// to write uncompressed data
	int (*NextByte)(void *o);	// a CallBack to get the next byte from source  
	void *o;					// dont't-care extra callback parameter - see callbacks
	enum INFLENUM ERROR;
	
	// reader 
	int src;		// 1-byte buffer
	int sbit;		// shift

	// Huffman-tables
	//int n1[7], s1[19];		// jpeg-style DHT for len-of-len, 3-bit codes, max. 19 symbols, 0..14 values
	int b2[16], s2[288];		// lenght codes from 0..15, maxbitlenght in deflate is 15
	int b3[16], s3[32];			// distance codes, jpeg-style DHT, 
	int len[288+32];			// To make the HT-s
	struct JDHT J2, J3;
};

extern enum INFLENUM inflate(struct INFLATE *inf, unsigned char *dst, int (*get_byte)(void *o), void *o);
