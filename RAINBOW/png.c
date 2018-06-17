/***********************************************************  

  Tested against PngSuite-2017jul19 by Willem van Schaik

  Note that correct decoding is the aim here, not proper 
  display of these images. Only the 4 critical chunks 
  are implemented. 

  - Basic formats: OK
  - Odd sizes: OK
  - Background colors: OK (but bKGD chunk is not implemented)
  - Transparency with Alpha: OK 
  - Gamma: OK (but not implemented)
  - Filters: OK
  - Additional palettes: OK (but not implemented)
  - Ancillary chunks: OK (but none of them implemented)
  - Chunk ordering: OK
  - Zlib compression level: OK
  - Corrupt files:
      - ENUMPNGERR_SIGNATURE: OK
	  - ENUMPNGERR_CRLF: OK
      - ENUMPNGERR_INVALIDCOLOURTYPE: OK
	  - ENUMPNGERR_INVALIDBITDEPTHS: OK
	  - ENUMPNGERR_IDATMISSING: OK
	  - Incorrect CHUNK checksum: not checked, decoded


*   Copyright (c) 2017 A. Tarpai 
*   
*   Permission is hereby granted, free of charge, to any person obtaining a copy
*   of this software and associated documentation files (the "Software"), to deal
*   in the Software without restriction, including without limitation the rights
*   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*   copies of the Software, and to permit persons to whom the Software is
*   furnished to do so, subject to the following conditions:
*   
*   The above copyright notice and this permission notice shall be included in all
*   copies or substantial portions of the Software.
*   
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*   SOFTWARE.
*/

// GCC -c -Wno-multichar png.c

#include <stdio.h>		// debug only
#include <malloc.h>	

#include "GETC.h" 
#include "inflate.h"
#include "bmp.h" 
#include "png.h"


static void ChunkHead(struct PNGINFO *o)
{
	o->Chunk_Length= GETDbi();
	o->Chunk_Type= GETDbi();
	printf("%08X: CHUNK '%c%c%c%c' (%d)\n", TELL()-8, ((char*)&o->Chunk_Type)[3], ((char*)&o->Chunk_Type)[2], ((char*)&o->Chunk_Type)[1], ((char*)&o->Chunk_Type)[0], o->Chunk_Length);
}

static void NextChunk(struct PNGINFO *o)
{
	SEEK(4);		// skip CHUNK CRC
	ChunkHead(o);		
}

static int Byte_in(struct PNGINFO *o)
{
	while (0==o->Chunk_Length)  //zero-length IDAT?
	{
		NextChunk(o);
		if ( o->Chunk_Type != 'IDAT' ) return -1;//TODO: bail out 
	}
	o->Chunk_Length--;
	return GETC();
}

static int ScanLineInBytes(int w, int BitDepth, int CHANNEL)
{
	return (( ( w * BitDepth ) + 7 ) / 8) * CHANNEL + 1;
}

static struct Adam { int x0, y0, stx, sty, log2stx, log2sty, pad[2]; } 
ADAM7[] = {
	{ 0,0,8,8, 3,3 },
	{ 4,0,8,8, 3,3 },
	{ 0,4,4,8, 2,3 },
	{ 2,0,4,4, 2,2 },
	{ 0,2,2,4, 1,2 },
	{ 1,0,2,2, 1,1 },
	{ 0,1,1,2, 0,1 }};

static int Adam7sizInflateBuff(struct Adam7Parameters *pas, int w, int h, int BitDepth, int CHANNEL)
{
	int i, sum;
	for (sum=i=0; i<7; i++) 
	{
		pas[i].SubImageWidth= (w - ADAM7[i].x0 + ADAM7[i].stx - 1 ) >> ADAM7[i].log2stx;
		pas[i].SubImageHeight= (h - ADAM7[i].y0 + ADAM7[i].sty - 1 ) >> ADAM7[i].log2sty;
		pas[i].SubImageScanLineByteSize= pas[i].SubImageWidth? ScanLineInBytes(pas[i].SubImageWidth, BitDepth, CHANNEL) : 0;
		sum+= pas[i].SubImageSize= pas[i].SubImageScanLineByteSize * pas[i].SubImageHeight;
	}
	return sum;
}


// PNG PIXEL converters: 
//    16-bit bitdepths converts HI byte only
//    Alpha-blending: test, over white background: "output = alpha * foreground + (1-alpha) * background"

static void ConvertRGB(struct OP *op)// R G B --> BM
{
		op->WritePixel(op->src, op->dst);
		op->src+= op->PNGBPP;
}

static void ConvertRGB16(struct OP *op)// R R G G B B  -->  BM
{
		BYTE pix[4];	
		pix[0]= op->src[0];
		pix[1]= op->src[2];
		pix[2]= op->src[4];
		op->WritePixel(pix, op->dst);
		op->src+= 6;
}

static void ConvertRGBA(struct OP *op)// R G B A  -->  BM 
{
		BYTE pix[4];
		pix[0]= ( op->src[0] * op->src[3] >> 8 ) + ~op->src[3];		
		pix[1]= ( op->src[1] * op->src[3] >> 8 ) + ~op->src[3];
		pix[2]= ( op->src[2] * op->src[3] >> 8 ) + ~op->src[3];
		op->WritePixel(pix, op->dst);
		op->src+= 4;
}

static void ConvertRGBA16(struct OP *op)// R R G G B B A A  -->  BM
{
		BYTE pix[4];	// HI bytes only 			
		pix[0]= ( op->src[0] * op->src[6] >> 8 ) + ~op->src[6];
		pix[1]= ( op->src[2] * op->src[6] >> 8 ) + ~op->src[6];
		pix[2]= ( op->src[4] * op->src[6] >> 8 ) + ~op->src[6];
		op->WritePixel(pix, op->dst);
		op->src+= 8;
}

static void ConvertGREYA(struct OP *op)// Y A  --> BM
{
		BYTE pix[4];
		pix[0]= ( op->src[0] * op->src[1] >> 8 ) + ~op->src[1];
		op->WritePixel(pix, op->dst);
		op->src+= 2;
}

static void ConvertGREYA16(struct OP *op)// Y Y A A  --> BM
{
		BYTE pix[4];
		pix[0]= ( op->src[0] * op->src[2] >> 8 ) + ~op->src[2];
		op->WritePixel(pix, op->dst);
		op->src+= 4;
}

static void ConvertPAL(struct OP *op)// 8-bit PAL-index --> BM (all Greyscale here too)
{
	op->WritePixel(&op->TRUECOLORPALETTE[*op->src], op->dst);
	op->src+= op->PNGBPP;
}

static void ConvertPACKED(struct OP *op)// 1,2,4-bit PAL-index --> BM
{
		int bitpos= op->cntPixel << op->log2BitDepth;
		int shift= op->FirstShift - (bitpos & 7);
		op->WritePixel(&op->TRUECOLORPALETTE[( op->src[bitpos >> 3] >> shift ) & op->mask], op->dst);
}



static struct PNGTYPE {	// IHDR_ColourType/IHDR_BitDepth dependent params 
	void (*Convert)(struct OP *op);
	void (*Convert16)(struct OP *op);
	int PNGBPP;
	int CHANNEL;
	int Mask;
	char *text;//debug
}
IMAGETYPE[] = {
	{   ConvertPAL,      ConvertPAL,   1, 1,  1|2|4|8|16,  "Grayscale"             },   // 0 : make fake PALETTE then copy pixels
	{            0,                                                                },   // 1 : NA
	{   ConvertRGB,    ConvertRGB16,   3, 3,        8|16,  "Truecolour"            },   // 2 : 
	{   ConvertPAL,               0,   1, 1,     1|2|4|8,  "Indexed-colour"        },   // 3 : from PALETTE copy pixels
	{ ConvertGREYA,  ConvertGREYA16,   2, 2,        8|16,  "Grayscale with Alpha"  },   // 4 : 
	{            0,                                                                },   // 5 : NA
	{  ConvertRGBA,   ConvertRGBA16,   4, 4,        8|16,  "Truecolour with Alpha" }};  // 6 : 


/******  Filtering and Conversion *****************************

  Because of sub-images and interlace, Filtering and Conversion 
  is done together using a double-buffer. 2 DWORD-aligned lines 
  padded left with 8 zeroes to avoid left neighbour check. 
  Conversion always from PIXLINE --> to BM-pixels. 
             
     +--------+-------+-------+-------+-------+-------+-------+                                                          
     |00000000| R G B | R G B | R G B | R G B | R G B | R G B |    PRIORPIXLINE
     |00000000| R G B | R G B | R G B | R G B | R G B | R G B |         PIXLINE 
     +--------+-------+-------+-------+-------+-------+-------+                                                          
*/

static int Paeth(int a, int b, int c)
{
    int pa, pb, pc;
    if ( (pa=b-c) < 0 ) pa= -pa;
    if ( (pb=a-c) < 0 ) pb= - pb;
    if ( (pc=a+b-2*c) < 0 ) pc= -pc;
    if (pa <= pb && pa <= pc) return a;
    else if (pb <= pc) return b;
    else return c;
}

static void filter4(struct OP *op)		// PAETH
{
	int i;
	PBYTE a, b, c;
	a= op->pixline - op->PNGBPP;
	b= op->priorpixline;
	c= b - op->PNGBPP;
	for (i=0; i<op->ImageWidth8; i++) op->pixline[i]= Paeth(a[i], b[i], c[i]) + op->InflateBuff[i];
}

static void filter3(struct OP *op)		// AVG
{
	int i;
	PBYTE a, b;
	a= op->pixline - op->PNGBPP;
	b= op->priorpixline;
	for (i=0; i<op->ImageWidth8; i++) op->pixline[i]= ((a[i] + b[i]) >> 1) + op->InflateBuff[i];
}

static void filter2(struct OP *op)		// UP
{
	int i;
	PBYTE b;
	b= op->priorpixline;
	for (i=0; i<op->ImageWidth8; i++) op->pixline[i]= b[i] + op->InflateBuff[i];
}

static void filter1(struct OP *op)		// SUB
{
	int i;
	PBYTE a;
	a= op->pixline - op->PNGBPP;
	for (i=0; i<op->ImageWidth8; i++) op->pixline[i]= a[i] + op->InflateBuff[i];
}


static void filter0(struct OP *op)		// NONE
{
	int i;
	for (i=0; i<op->ImageWidth8; i++) op->pixline[i]= op->InflateBuff[i];
}

static void (*Filter[])(struct OP *op) = { filter0, filter1, filter2, filter3, filter4 };



static enum ENUMPNG ConvertSubImage(struct OP *op, int Width, int Height, int ScanLineByteSize, PBYTE dst, int dstLinePitch, int dstPixelPitch)
{
	int i;
	void *tmp;
	int FilterByte = *op->InflateBuff++;

	op->ImageWidth8= ScanLineByteSize-1;

	if (FilterByte > 1) for (i=0; i<op->ImageWidth8; i++) op->priorpixline[i]= 0;
	
	for(;;) {

			if (FilterByte>4) return ENUMPNGERR_INVALIDFILTERBYTE; // TODO: handle it

			Filter[FilterByte](op);
			op->InflateBuff += op->ImageWidth8;

			op->src= op->pixline;
			op->dst= dst;
			for (op->cntPixel=0; op->cntPixel < Width; op->cntPixel++) {
				op->Convert(op);
				op->dst+= dstPixelPitch;
			}

			if ( 0 == --Height) return ENUMPNG_OK;

			tmp= op->pixline;
			op->pixline= op->priorpixline;
			op->priorpixline= tmp;									
			FilterByte= *op->InflateBuff++;
			dst += dstLinePitch;
	}
}

static void isPacked(struct PNGINFO *o)	// ColourType 0 or 3 
{
	if (o->IHDR_BitDepth<8) {
		o->OP.Convert= ConvertPACKED;
		o->OP.mask= (1<<o->IHDR_BitDepth) - 1;
		o->OP.log2BitDepth= o->IHDR_BitDepth>>1;
		o->OP.FirstShift= 8-o->IHDR_BitDepth;
	}
}

extern void PNGConvert(struct PNGINFO *o, enum BITPERPIXELENUM bppenum, PBYTE dst, int dstPitch)
{
	int i;
	struct BMW *bmw= BM_WRITER + bppenum;

	if ( o->IHDR_BitDepth == 16) 
	{
		o->OP.Convert= o->pngType->Convert16;	
		o->OP.PNGBPP= o->pngType->PNGBPP*2;
	}
	else 
	{
		o->OP.Convert= o->pngType->Convert;
		o->OP.PNGBPP= o->pngType->PNGBPP;
	}

	if (0==o->IHDR_ColourType) 	// Greyscale without Alpha 1,2,4,8,16 bit: build fake pal 
	{
		int n, step, Y;				
		if ( o->IHDR_BitDepth < 8 ) n= 1<<o->IHDR_BitDepth, step= 255/(n-1);
		else step=1, n=256;
		for (Y=i=0; i<n; i++, Y+=step) bmw->WriteGREY(&Y, o->OP.TRUECOLORPALETTE+i);
		o->OP.WritePixel= bmw->CopyPixel;
		isPacked(o);
	}
	else if (3==o->IHDR_ColourType) // 'true' palette in PLTE
	{
		for (i=0; i < o->PLTE_Length; i++)	{
			DWORD col =  o->OP.TRUECOLORPALETTE[i];
			bmw->WriteRGBA(&col, o->OP.TRUECOLORPALETTE+i);
		}
		o->OP.WritePixel= bmw->CopyPixel;
		isPacked(o);
	}
	else if (4==o->IHDR_ColourType) o->OP.WritePixel= bmw->WriteGREY;	// Greyscale with Alpha 8/16-bit
	else o->OP.WritePixel= bmw->WriteRGBA;								// Truecolor and Truecolor with Alpha 

	if (o->IHDR_InterlaceMethod) 
	{
		struct Adam7Parameters *pas= o->ADAM7;
		for (i=0; i<7; i++) 
			if ( pas[i].SubImageSize ) 
				ConvertSubImage(&o->OP, pas[i].SubImageWidth, pas[i].SubImageHeight, pas[i].SubImageScanLineByteSize, 
				dst + dstPitch * ADAM7[i].y0 + ADAM7[i].x0 * bmw->BPP, dstPitch * ADAM7[i].sty, bmw->BPP * ADAM7[i].stx);
	}
	else ConvertSubImage(&o->OP, o->ImageWidth, o->ImageHeight, o->ScanLineByteSize, dst, dstPitch, bmw->BPP);
}


extern enum ENUMPNG PNGDecode(struct PNGINFO *o)
{
	int FilterLineByteSize;
	int sizInflateBuff;
	o->ScanLineByteSize= ScanLineInBytes(o->ImageWidth, o->IHDR_BitDepth, o->pngType->CHANNEL); //with +1 FilterByte
	FilterLineByteSize= PAD2DWORD(o->ScanLineByteSize+(8-1)); // 8-byte left-padding minus FilterByte
	sizInflateBuff= o->IHDR_InterlaceMethod? Adam7sizInflateBuff(o->ADAM7, o->ImageWidth, o->ImageHeight, o->IHDR_BitDepth, o->pngType->CHANNEL) : o->ScanLineByteSize * o->ImageHeight;

	o->png_mem= malloc( 2*FilterLineByteSize + sizInflateBuff );
	if ( 0 == o->png_mem ) return o->ERROR=ENUMPNGERR_MALLOC;

	o->OP.InflateBuff= (PBYTE)o->png_mem + 2*FilterLineByteSize;
	o->OP.pixline= (PBYTE)o->png_mem + 8;
	o->OP.priorpixline= o->OP.pixline + FilterLineByteSize;
	((PDWORD)o->OP.pixline)[-1]= ((PDWORD)o->OP.pixline)[-2]= 0;
	((PDWORD)o->OP.priorpixline)[-1]= ((PDWORD)o->OP.priorpixline)[-2]= 0;

	// <-- READ and INFLATE the GZIP data from (possibly multiple) IDAT CHUNKS:
	// Byte_in(): 'Proper way' to read bytes from from IDAT. Handles skipping IDAT headers. An inflate call-back too. 
	{
		int cmf, flg;
		cmf= Byte_in(o);
		flg= Byte_in(o);

		if ( (cmf & 15) != 8 ) return o->ERROR=ENUMPNGERR_NOTDEFLATE;
		//LZ77WindowSize= 1 << ( ( cmf >> 4 ) + 8 );	// Full buffer, NOT implemented. Usually 32K for small PNG too.  

		if ( 0 > inflate(&o->Inflate, o->OP.InflateBuff, (void*)Byte_in,o) ) return o->ERROR=ENUMPNGERR_INFLATEERROR;
		
		Byte_in(o);//Discard ADLER32
		Byte_in(o);
		Byte_in(o);
		Byte_in(o);
	}

	NextChunk(o);
	if (o->Chunk_Type == 'IEND') return ENUMPNG_OK;
	return o->ERROR=ENUMPNGERR_IENDEXPECTED; 
}


extern enum ENUMPNG PNGInfo(struct PNGINFO *o)
{
	o->ERROR= ENUMPNG_OK;	
	o->png_mem= 0;

	if (GETDbi() != '\x89PNG') return o->ERROR=ENUMPNGERR_SIGNATURE; 
	if (GETDbi() != 0x0d0a1a0a) return o->ERROR=ENUMPNGERR_CRLF;
	printf("%08X: PNG-SIGNATURE\n", TELL()-8);
	ChunkHead(o);

	for(;;) 
	{
		switch (o->Chunk_Type)		// ONLY THE 4 CRITICAL CHUNKS ARE IMPLEMENTED
		{
			case 'IHDR': 

				o->ImageWidth= GETDbi();
				o->ImageHeight= GETDbi();
				o->IHDR_BitDepth= GETC();
				o->IHDR_ColourType= GETC();
				o->IHDR_CompressionMethod= GETC();
				o->IHDR_FilterMethod= GETC();
				o->IHDR_InterlaceMethod= GETC();

				if ( o->IHDR_ColourType > 6 || 0 == IMAGETYPE[o->IHDR_ColourType].Convert ) return o->ERROR=ENUMPNGERR_INVALIDCOLOURTYPE;
				o->pngType= IMAGETYPE + o->IHDR_ColourType;
				if ( 0 == (o->pngType->Mask & o->IHDR_BitDepth) ) return o->ERROR=ENUMPNGERR_INVALIDBITDEPTHS;
				printf("  %d x %d %s (%d-bpp)\n", o->ImageWidth, o->ImageHeight, IMAGETYPE[o->IHDR_ColourType].text, o->IHDR_BitDepth);
				break;

			case 'IDAT':
				return ENUMPNG_OK;	// IDAT has been reached. OK, ready for decode (client may cancel the process, too big image?)
 
			case 'IEND':
				return o->ERROR=ENUMPNGERR_IDATMISSING;

			case 'PLTE':	
				if (3==o->IHDR_ColourType)  // 'true' palette?
				{
					int i;
					o->PLTE_Length = o->Chunk_Length / 3;  // # of entries determined by Length (always 8-bit values)
					for (i=0; i < o->PLTE_Length; i++)	o->OP.TRUECOLORPALETTE[i] = GETC() | (GETC()<<8) | (GETC()<<16); // R-G-B order
					break;
				}
				//else: we don't care about palette suggestions in this true-color decoder: SEEK()

			default:
				SEEK(o->Chunk_Length);
		}
		NextChunk(o);
	}
}
