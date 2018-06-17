// Possible DIRECT-COLOR output from these decoders: 15, 16, 24, 32-bpp BM-format
enum BITPERPIXELENUM
{
	BITPERPIXEL16,
	BITPERPIXEL15,
	BITPERPIXEL24,
	BITPERPIXEL32,
};

// same as WinDef.h
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef BYTE *PBYTE;
typedef WORD *PWORD;
typedef DWORD *PDWORD;


struct BMW
{
	void (*WritePixel)(void* src, void* dst);	// writes target BM-pixel from 'normal' BGRA order
	void (*WriteRGBA)(void* src, void* dst);    // writes BM-pixel from OpenGL/PNG/GIF format (RGBA-order)
	void (*WriteGREY)(void* src, void* dst);    // writes BM-pixel from one-channel gray (PNG, JPEG, TIFF?)
	void (*CopyPixel)(void *color, void *p);	// pixel copy: used in PALETTE mode
	int BPP;
	int true_bpp;
	DWORD biCompression;
	WORD biBitCount;
	char true_bpp_string[2];
};

extern struct BMW BM_WRITER[];

// useful macros
#define PAD2DWORD(x) ( ((x)+3) & ~3 )
#define   INDWORD(x) ( ((x)+3) >> 2 )

/************* BMP-FILE DECLARATIONS ***********************************

	Packing structs:
	gcc:    __attribute__((packed))
	VC++:   pragma pack(push,1)
***********************************************************************/

#define BMPTEXTLENGTH (5*8)	// for some (c) text 

#ifdef _WIN32
#define __attribute__(x)	// clear for VC++
#pragma pack(push,1)
#endif

	typedef struct __attribute__((packed)) tagBITMAPFILEHEADER { 
	  WORD    bfType;
	  DWORD   bfSize;
	  DWORD   bfReserved;
	  DWORD   bfOffBits;
	} BITMAPFILEHEADER, *PBITMAPFILEHEADER;

	typedef struct __attribute__((packed)) tagBITMAPINFOHEADER{
	  DWORD  biSize;
	  long   biWidth;
	  long   biHeight;
	  WORD   biPlanes;
	  WORD   biBitCount;
	  DWORD  biCompression;
	  DWORD  biSizeImage;
	  long   biXPelsPerMeter; 
	  long   biYPelsPerMeter; 
	  long   biClrUsed;
	  long   biClrImportant;
	} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

	struct __attribute__((packed)) BMP {
		BITMAPFILEHEADER bmfh;		// 14 bytes
		BITMAPINFOHEADER bmih;		// 40 bytes
		DWORD bmiColors[3];			// should follow for RGB565 (color masks)
		// + any extra (and still compatible with BMP)
		char true_bpp_string[2];		// have to pad with 2 anyway.
		char mytext[BMPTEXTLENGTH];		// for some text + 2 bytes padding
		int true_bpp;					// debug
		DWORD biPitch;					// byte-pitch of raster lines 
	};

#ifdef _WIN32
#pragma pack(pop)
#endif

extern int  BmpSize(enum BITPERPIXELENUM bppenum, int Width, int Height);						// Size of the BMP-file in bytes						
extern void BmpPrep(struct BMP *bmp, enum BITPERPIXELENUM bppenum, int Width, int Height);		// Prepares the BMP-HEADER 
