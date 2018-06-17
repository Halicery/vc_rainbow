struct DIRECTORY_ENTRY {
	unsigned short Tag;
	unsigned short Type;
	unsigned int Count;
	unsigned int Value;
};

struct TIFFINFO {
		
	// public
		
	int ImageWidth;		// image dimensions in pixels 
	int ImageHeight;
	void *tiff_mem;		// private, client free()-s at the end

	// private 

	struct DIRECTORY_ENTRY DE;		// for reading directory entries 
	int (*DE_Reader)();				// W or D reader

	struct LZW U;					// my LZW object
	int DifferencingPredictor;		// LZW with predictor (MS Paint .tif)

	int NumberOfStrips, StripOffset, (*StripOffset_Reader)();
	int NumberOfComponents;
	int PlanarConfiguration;
	int PhotometricInterpretation;

	int (*GETW)();		// bi/li based on TIFF-endian-type
	int (*GETD)();	
};

extern int TIFFInfo(struct TIFFINFO *t);
extern int TIFFDecode(struct TIFFINFO *t);
extern void TIFFConvert(struct TIFFINFO *t, enum BITPERPIXELENUM bppenum, unsigned char *dst, int dstPitch);
