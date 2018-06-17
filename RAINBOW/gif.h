enum ENUMGIF {
	ENUMGIF_OK = 1,
	ENUMGIF_ERR_HEADER = -999, 			// not GIF
	ENUMGIF_ERR_MALLOC, 				// failed
	ENUMGIF_ERR_UNKNOWN_BLOCK_ID,
	ENUMGIF_ERR_UNKNOWN_EXTENSION_ID,
	ENUMGIF_ERR_LZW,
	ENUMGIF_ERR_LZW_BLOCK_TERMINATOR, 
	ENUMGIF_ERR_INVALIDCODESIZE, 
};


struct GIFINFO {

	int Frame;          // counter 
	enum ENUMGIF ERROR;	// different error codes 

	// LogicalScreenDescriptor: REQUIRED. exactly one. must appear immediately after the Header.
	int ScreenWidth;      /* Width of Display Screen in Pixels */
	int ScreenHeight;     /* Height of Display Screen in Pixels */
	int ScreenBackgroundColor;  /* Background Color Index */
	int ScreenPacked;           /* Screen and Color Map Information */
		//<Packed Fields>  =     Global Color Table Flag       1 Bit
		//                       Color Resolution              3 Bits
		//                       Sort Flag                     1 Bit
		//                       Size of Global Color Table    3 Bits

	// ImageDescriptor: REQUIRED
	int ImageLeft;         /* X position of image on the display */
	int ImageTop;          /* Y position of image on the display */
	int ImageWidth;        /* Width of the image in pixels */
	int ImageHeight;       /* Height of the image in pixels */
	int ImagePacked;       /* Image and Color Table Data Information */
		//<Packed Fields>  =     Local Color Table Flag        1 Bit
		//                       Interlace Flag                1 Bit
		//                       Sort Flag                     1 Bit
		//                       Reserved                      2 Bits
		//                       Size of Local Color Table     3 Bits
		
	//GraphicsControlExtension: OPTIONAL
	int has_GraphicsControlExtension;
	int GraphicDelayTime;    /* Hundredths of seconds to wait	*/
	int GraphicColorIndex;   /* Transparent Color Index */
	int GraphicPacked;       /* Method of graphics disposal to use */
		//<Packed Fields>  =     Reserved                      3 Bits
		//                       Disposal Method               3 Bits
		//                       User Input Flag               1 Bit
		//                       Transparent Color Flag        1 Bit 	

	//ApplicationExtension: OPTIONAL
	int has_ApplicationExtension;
	char APP[12];

	struct LZW *lzw;				// my lzw instance, big (32K, we malloc)
	unsigned char *ImageData;		// <-- LZW decompresses here
	unsigned char BlkCnt;
	
	int GlobalTrueColorTable[256];
	int LocalTrueColorTable[256];
	int *TrueColorTable;			// actual

	// Conversion: parameters from BM_WRITER 
	int bmBPP;
	void (*WritePixel)(void *color, void *p);   // converting RGB-palette value to target pixel
	void (*CopyPixel)(void *color, void *p);	// then fast copy from palette during conversion

	// memory address to write scanlines
	unsigned char *DSTaddr;
	int DSTpitch;

	void *gif_mem;
};

extern enum ENUMGIF GIFInfo(struct GIFINFO *g);
extern enum ENUMGIF GIFSetup(struct GIFINFO *g, enum BITPERPIXELENUM bppenum, unsigned char *dst, int dstPitch);
extern enum ENUMGIF GIFNextImage(struct GIFINFO *g);
extern enum ENUMGIF GIFDecodeImage(struct GIFINFO *g);
extern void GIFConvertImage(struct GIFINFO *g);
