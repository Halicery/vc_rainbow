/********************************************************************************
    LZW decompressor for GIF and TIFF

      GIF: 
        -  little-endian packing
        -  code increase before eg. 512, 1024..
        -  input data block-ed
        -  StartCodeSize: 2..8
        -  DEFERRED CLEAR CODE after 4095

      TIFF: 
        -  big-endian packing
        -  code increase before eg. 511, 1023..
        -  input data is continous
        -  StartCodeSize: 8
        -  CLR after 4095


    ---------->   LZW DECOMPRESS  ------>   .. .. .. .. .. .. ..
    NextByte()                               destination buffer


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

#include "lzw.h" 

static int LASTCODEGIF[]=  { 0,0,0,8,16,32,64,128,256,512,1024,2048,4096 };
static int LASTCODETIFF[]= { 0,0,0,7,15,31,63,127,255,511,1023,2047,4095 };		// by CodeSize (also used as Bitmasks for code-readers)


static int Byte_in(struct LZW *u)
{
	return u->Buff = u->NextByte(u->o);	// Input callback. TODO: error checking	
}

static int ReadCodeli(struct LZW *u)	// GIF: little-endian byte-packing
{
	int code, s;
	code = u->BuffBit? u->Buff : Byte_in(u);	
	s = u->CodeSize + u->BuffBit;
	if ( s > 8 ) 
	{
		((char*)&code)[1] = Byte_in(u);
		if ( s > 16 ) ((char*)&code)[2] = Byte_in(u);
	}
	code = ( code >> u->BuffBit ) & u->CodeMask;
	u->BuffBit = s & 7;
	return code;
}

static int ReadCodebi(struct LZW *u)	// TIFF: big-endian byte-packing
{
	int code;
	if ( 0 == u->BuffBit) code = Byte_in(u), u->BuffBit=8;
	else code = u->Buff;
	while ( u->CodeSize > u->BuffBit) code = (code << 8 ) | Byte_in(u), u->BuffBit+=8;
	u->BuffBit = u->BuffBit - u->CodeSize;
	return ( code >> u->BuffBit ) & u->CodeMask;
}

static void setCodeSize(struct LZW *u, int CodeSize)
{
	u->CodeSize= CodeSize;
	u->LastCode= u->LASTCODE[CodeSize];
	u->CodeMask= LASTCODETIFF[CodeSize];
}

static unsigned char *emit(struct DICT *w, unsigned char *dst) 
{
	unsigned char *p= dst + w->len + 1;
	do dst[w->len]= w->c; while (w=w->w);
	return p;
}

extern unsigned char *lzw_decompress(struct LZW *u, unsigned char *dst)
{
	int code, idx;
	struct DICT *w;		// <-- Dictionary Entry of previous string: emit
	struct DICT *e;		// <-- Dictionary Entry of new string

	setCodeSize(u, u->FirstCodeSize);
	u->BuffBit = 0;

	for(;;)
	{
		code = u->ReadCode(u);	// new or previous dict entry
		if (u->CLR==code) goto clr;
		if (u->EOI==code) return dst;

		e = u->D + idx;		// Always add new entry, also handles 'code not defined yet' 
		e->x = w->x + 1;	// Length+1 and FirstCharPropagation in c0
		e->w = w;			// w here is previous string  
		w = u->D + code;	// this is 'k' --> next iteration's 'w'
		e->c = w->c0;		// append FirstChar of 'k' to new entry
		dst = emit(w, dst);	// emit string backwards

		if (++idx == u->LastCode) 
		{
			if ( u->CodeSize < 12 ) setCodeSize(u, u->CodeSize+1);
			else for (;;) 
			{ 
				code = u->ReadCode(u);			// reached top of Dictionary
				if (u->CLR==code) goto clr;		// should be for TIFF
				if (u->EOI==code) return dst;		
				dst = emit(u->D+code, dst);		// DEFERRED CLEAR CODE: emitting the code strings without adding to dictionary
			}
		}
		continue;

		clr: setCodeSize(u, u->FirstCodeSize);
		code = u->ReadCode(u);
		if (u->CLR==code) goto clr;
		if (u->EOI==code) return dst;
		idx = u->EOI + 1;		// dictionary index init
		w = u->D + code;		// store as previous 
		*dst++ = code; 			// first-read DOES NOT GO into dictionary, base code
	}
}

static void DictionaryInit(struct DICT *D, int dstart, int dend)
{
	int i;
	unsigned int x= 0x01010000 * dstart;
	for (i=dstart; i<dend; i++) 
	{
		D[i].w=0;
		D[i].x = x;
		x+= 0x01010000;
	}
}

extern unsigned char *lzw_decompress_gif(struct LZW *u, unsigned char *dst, int StartCodeSize)
{
	int CLR= 1 << StartCodeSize;
	if (CLR > u->CLR) DictionaryInit(u->D, u->CLR, CLR); // opt for subsequent calls (GIF anim)
	u->CLR= CLR;
	u->EOI= u->CLR + 1;
	u->FirstCodeSize= StartCodeSize + 1;
	return lzw_decompress(u, dst);
}

extern void lzw_init_gif(struct LZW *u, int (*cbNextByte)(void *obj), void *obj)
{
	u->CLR=0;
	u->LASTCODE= LASTCODEGIF;
	u->ReadCode= ReadCodeli;
	u->NextByte= cbNextByte, u->o=obj;
}

extern void lzw_init_tiff(struct LZW *u, int (*cbNextByte)(void *obj), void *obj)
{
	DictionaryInit(u->D, 0, 256);
	u->CLR= 256;
	u->EOI= 257;
	u->FirstCodeSize= 9;
	u->LASTCODE= LASTCODETIFF;
	u->ReadCode= ReadCodebi;
	u->NextByte= cbNextByte, u->o=obj;
}
