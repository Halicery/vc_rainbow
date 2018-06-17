enum ENUMPNG {						// >0 for OK and <0 for ERROR
	ENUMPNG_OK = 1,
	ENUMPNGERR_SIGNATURE = -999,
	ENUMPNGERR_CRLF,
	ENUMPNGERR_INVALIDFILTERBYTE,
	ENUMPNGERR_INVALIDCOLOURTYPE,
	ENUMPNGERR_INVALIDBITDEPTHS,
	ENUMPNGERR_INFLATEERROR,		// Check png->Inflate->ERROR for more details
	ENUMPNGERR_NOTDEFLATE,			// GZIP Compression method should be 8 for PNG: "deflate" 
	ENUMPNGERR_INFLATEDELTA,		// Check png->Inflate->ERROR for more details
	ENUMPNGERR_IDATMISSING,			// IEND reached but no IDAT
	ENUMPNGERR_IENDEXPECTED,		// IEND not follows IDAT
	ENUMPNGERR_MALLOC,				// failed
};

struct Adam7Parameters {
	int SubImageWidth;
	int SubImageHeight;
	int SubImageScanLineByteSize;	// Computed for all sub-images (FilterByte + PNG pixel data)
	int SubImageSize;				// # of bytes for subimage in inflated data (used to calculate buffer size and to skip passes when zero)
};

struct OP {		// to throw only one parameter back-and-forth during filtering/conversion

	// For Converters
	PBYTE InflateBuff;		// Filtered (uncompressed) PNG Image data is here. THIS POINTER CHANGES DURING CONVERSION!
	PBYTE src, dst;
	void (*Convert)(struct OP *op);
	void (*WritePixel)(void *color, void *addr);	// --> to BM_WRITER
	int PNGBPP;

	DWORD TRUECOLORPALETTE[256];	// <-- target-converted palette-colors
	int FirstShift;					// For PACKED pixel types (1,2,4-bpp)
	int mask;
	int log2BitDepth;
	int cntPixel;

	// For the byte filters
	int ImageWidth8;				// PNG Image width in BYTE 
	PBYTE pixline, priorpixline;	// filter-line double-buffer pointers 
};

struct PNGINFO {

	// public
		
		int ImageWidth;			// image dimensions in pixels (read from IHDR)
		int ImageHeight;
		void *png_mem;			// free me after
		enum ENUMPNG ERROR;		// different error codes 
		struct INFLATE Inflate;	// my Inflate object

	// private

		struct OP OP;	// working parameters: passed along during conversion 

		struct PNGTYPE *pngType;

		int Chunk_Length;		// actual Chunk
		int Chunk_Type;			// GCC needs -Wno-multichar to suppress warning on cases like 'IDAT'

		//int IHDR_Width;		// IHDR  
		//int IHDR_Height;
		int IHDR_BitDepth;
		int IHDR_ColourType;
		int IHDR_CompressionMethod;
		int IHDR_FilterMethod;
		int IHDR_InterlaceMethod;
		int PLTE_Length;

		struct Adam7Parameters ADAM7[7];	// Interlace. Parameters of the 7 sub-images 
		int ScanLineByteSize;				// Non-interlace. For the byte-filters (+1 WITH filter byte)
};

extern enum ENUMPNG PNGInfo(struct PNGINFO *o);
extern enum ENUMPNG PNGDecode(struct PNGINFO *o);
extern void PNGConvert(struct PNGINFO *o, enum BITPERPIXELENUM bppenum, PBYTE dst, int dstPitch);
