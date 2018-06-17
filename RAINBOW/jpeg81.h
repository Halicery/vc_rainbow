//	(C) 2012 Attila Tarpai (tarpai76 at gmail dot com)

enum JPEGENUM {
	JPEGENUM_OK=1, 
	JPEGENUMERR_MISSING_SOI = -999,	// file doesnt start with SOI 
	JPEGENUMERR_UNKNOWN_SOF,		// differential frame?
	JPEGENUMERR_COMP4,				// more than 4 components in file
	JPEGENUMERR_MALLOC,				// malloc failed
	JPEGENUMERR_NODHT,				// no DHT found
	JPEGENUMERR_NODQT,				// no DQT found
	JPEGENUMERR_QTREDEF,			// not implemented (*)
	JPEGENUMERR_MARKERC8,			// JPG-1 extension?
	JPEGENUMERR_MARKERDNL,			// DNL marker found (not supported)
	JPEGENUMERR_ZEROY,				// Y in SOFn is zero (DNL?)
	JPEGENUMERR_COMPNOTFOUND,		// Scan component selector (Csj) not found among Component identifiers (Ci)
};

typedef short TCOEF;	// 16-bit coefficients
typedef TCOEF DU[64];	// The DATA UNIT
typedef unsigned short TSAMP;	// Lossless 'coefficients' are unsigned 

struct CABACSTATE  {	// borrowed from the AVC decoder
	int StateIdx;
	int valMPS;
};

struct COMP {		// Image Component Info and variables 

	// from SOF
	int Ci;			// Component identifier
	int Hi;			// Horizontal sampling factor
	int Vi;			// Vertical sampling factor
	int Qi;			// Quantization table destination selector

	// Computed parameters
	int du_w, du_h;		// width/height in data units for single scans and for idct/conversion
	int du_size;        // = du_w * du_h
	
	int du_width;	  // total width in DU (storage and interleaved scans)
	int du_total;     // -> for malloc 

	// Component coefficient buffer
	union {
		DU *du;			// DCT: pointer to DU
		TSAMP *samp;	// LL
	};

	// In scans
	union {					// either/or
		int DC;				// jpeg's differential encoded DC (per scan-component)
		int EOBRUN;			// Only in AC-single scan (Huffman)
	};

	// Huffman
		int *ACB;			// AC-'base-values'
		unsigned char *ACS;	// AC-symbols
		int *DCB;				
		unsigned char *DCS;	

	// Arithmetic
		struct CABACSTATE *DCST;	// DC-statistical area
		struct CABACSTATE *ACST;	// with -1 offset (we use 'k' itself for addressing)
		struct CABACSTATE *LLST;
		int U, L, Kx;
		int DIFF;	// diff value for the previous DC

	// Lossless 
		short *diffAbove;	// stored DIFF for LOSSLESS (an MCU line - 1?)
		int diffLeft[4+1];	// LineNo added (to fast test for zero)
};


struct JPEGD {		// The JPEG DECODER OBJECT

	struct COMP Components[4];

	// SOFn: Frame Header
	int Nf;
	int P;
	int Y, X;	// "number of samples per line in the component with the maximum number of horizontal samples" and vertical
	int SOF;		// save marker to make decisions
	int QT[4][64];	// Q-tables from stream

	void *jpeg_mem;				// <-- free me

	int Hmax, Vmax;	// for conversion
	int mcu_width;
	int mcu_height;
	int mcu_total;	// covers the whole image
	
	int HTB[2][4][16];					// Huffman 'base' values 
	unsigned char HTS[2][4][256];		// Huffman 'symbol' values


	int Ri;		// Restart interval
	
	void (*Reset_decoder)(struct JPEGD *j);	// huffman/arithmetic/lossless

	// Actual Scan
	int Ns;
	struct COMP *ScanComponents[4];		// --> pointer to COMP in order of scan component definition 
	int Ss, Se, Al, Ah;
	int Al2; // =1<<Al

	int ScanByte;
	int ScanBit;
	int (*Byte_in)(struct JPEGD *j);		// huffman/arithmetic
	void (*DecodeDataUnit)(struct JPEGD *j, struct COMP *sc, TCOEF *coef); // DCT huffman/arithmetic


	// LOSSLESS
		int LineNo;	// Ri clears this
		void (*decode_lossless)(struct JPEGD *j, struct COMP *C, int x, int y, TSAMP *coef);	// huffman/arithmetic

	// Arith
		// DAC, ArithmeticConditioning
		int U[4], L[4], Kx[4];

		unsigned short C;	//CodeRegister;					// C  <-- from bit stream
		unsigned short A;	//ProbabilityIntervalRegister;	// A  <-- 

		// AVC-style (JPEG 'statictical area')
		struct CABACSTATE DCST[4][49];
		struct CABACSTATE ACST[4][245];

};

extern int JPEGDecode(struct JPEGD *j);

/*
Tqi: Quantization table destination selector � Specifies one of four possible quantization table destinations
from which the quantization table to use for dequantization of DCT coefficients of component Ci is retrieved. If
the decoding process uses the dequantization procedure, this table shall have been installed in this destination
by the time the decoder is ready to decode the scan(s) containing component Ci. The destination shall not be respecified,
or its contents changed, until all scans containing Ci have been completed.
*/
