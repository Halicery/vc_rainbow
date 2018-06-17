/**************************
	Dictionary entry

	31              0
	+---+---+-------+
	| c | c0|  len  |
	+---+---+-------+
	|     prev      |
	+---------------+

**************************/

struct DICT {
	union { // union for for fast copy and init 
		struct
		{
			unsigned short len;
			unsigned char c0, c;	// actually, these two can be simply emitted as a short as the last 2 char of the string(!)
		};
		unsigned int x;
	};
	struct DICT *w;		// previous entry
};

struct LZW {						
	
	int (*NextByte)(void *o);	// a CallBack to get the next byte from source  
	void *o;					// dont't-care extra callback parameter - see callbacks

	struct DICT D[4096];	// 32K(!)

	int CLR;
	int EOI;
	int FirstCodeSize;

	// For code-readers: 
	int CodeSize;
	int LastCode;
	int CodeMask;
	int Buff;
	int BuffBit;

	// GIF/TIFF 
	int *LASTCODE;
	int (*ReadCode)(struct LZW *u);
};


// for GIF
extern void lzw_init_gif(struct LZW *u, int (*cbNextByte)(void *obj), void *obj);
extern unsigned char *lzw_decompress_gif(struct LZW *u, unsigned char *dst, int MinCodeSize); 

// for TIFF
extern void lzw_init_tiff(struct LZW *u, int (*cbNextByte)(void *obj), void *obj);
extern unsigned char *lzw_decompress(struct LZW *u, unsigned char *dst); 
