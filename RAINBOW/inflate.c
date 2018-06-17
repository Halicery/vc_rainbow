/*
    DEFLATE decompressor for PNG and ZIP

	- decompresses all Deflate data 
	- gzip Window not implemented 

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

#include "inflate.h"	

static int
zz[]={16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15},
len_extrabase[]={3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258},
len_extrabits[]={0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0},
dist_extrabase[]={1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577},
dist_extrabits[]={0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};

static int // STATIC HUFFMAN: JPEG-LIKE TABLE DEFINITIONS
B2[]={0,0,0,0,0,0,24,24+152,24+152+112},
B3[]={0,0,0,0,32},
S2[]={256,257,258,259,260,261,262,263,264,265,266,267,268,269,270,271,272,273,274,275,276,277,278,279,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,280,281,282,283,284,285,286,287,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255},
S3[]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};


static int Get1bit(struct INFLATE *inf)	// lsb first
{
	if ( ++inf->sbit & 8 ) {
		inf->sbit= 0;
		inf->src= inf->NextByte(inf->o); //TODO:check for error and bail out
		return inf->src & 1;
	}
	return (inf->src >> inf->sbit) & 1;
}

static int Getnbit(struct INFLATE *inf, int n) // lsb first, n can be zero
{
	int b, val=0;
	for (b=0; b<n; b++) val|= Get1bit(inf) << b;
	return val;
}

static int ReadHuffman(struct INFLATE *inf, int *b)	// returns index into sym-table
{
	int idx= Get1bit(inf);
	while ( idx >= *b ) idx= 2*idx + Get1bit(inf) - *b++;
	return idx;
}	

static void ini_dht(struct JDHT *j, int *s, int *b)
{
	j->s= s;
	j->b= b;
}

static void dht(struct JDHT *j, int *len, int size, int max, int *s, int *b) // Deflate-style Huffmann Table Definition - to - JPEG-style Huffmann Table Definition
{
	int i;
	for (i=0; i<max; i++) s[i]=0;			
	for (i=0; i<size; i++) s[len[i]]++;						// use 's' to count codes of equal lenght (NB: counts zero-lengths too for faster code and N[max] is DONTCARE)
	for (b[0]=0, i=1; i<max; i++) b[i] = b[i-1] + s[i];		// compute 'base (=running total)
	for (i=0; i<size; i++) if (len[i]) s[b[len[i]-1]++]= i;	// fill with symbols, b[i] <-- b[i+1] 
	ini_dht(j, s, b);
}

static int GetExBits(struct INFLATE *inf, int *extrabase, int *extrabits, int c)
{
	return extrabase[c] + Getnbit(inf, extrabits[c]); 
}

static unsigned char *copyn(unsigned char *dst, unsigned char *src, int n)
{
	int i= 0;
	do dst[i]=src[i]; while (++i<n);
	return dst + n;
}

static void inflt(struct INFLATE *inf)
{
	int c, l, d;	
	for (;;) {
		c= inf->J2.s[ReadHuffman(inf, inf->J2.b)];
		if (c > 256) {
			l= GetExBits(inf, len_extrabase, len_extrabits, c-257);
			d= GetExBits(inf, dist_extrabase, dist_extrabits, inf->J3.s[ReadHuffman(inf, inf->J3.b)]);
			inf->dst= copyn(inf->dst, inf->dst-d, l);
		}
		else if (c<256) *inf->dst++=c;
		else return;	// EOD (256)
	}
}

static int repn(int *dst, int val, int n)
{
	int i= 0;
	do dst[i]= val; while (++i<n);
	return n;
}

extern enum INFLENUM inflate(struct INFLATE *inf, unsigned char *dst, int (*get_byte)(void *o), void *o)
{
	int i, bfinal, hlit, hdist, hclen;

	inf->dst= dst;
	inf->NextByte= get_byte;
	inf->o= o;
	inf->sbit= 7;

	do 
	{
		bfinal= Get1bit(inf);

		switch (Getnbit(inf,2)) 
		{

			case 0:		// Uncompressed
				inf->sbit= 7;					// align bitstream
				i= inf->NextByte(inf->o);		// LO
				i+= inf->NextByte(inf->o)<<8;	// HI
				inf->NextByte(inf->o);			// discard NLEN
				inf->NextByte(inf->o);
				while (i--) *inf->dst++ = inf->NextByte(inf->o);
				break;

			case 1:		// Static Huffman
				ini_dht(&inf->J2, S2, B2);
				ini_dht(&inf->J3, S3, B3);
				inflt(inf);
				break;

			case 2:		// Dynamic Huffman 
				hlit=  Getnbit(inf,5) + 257;
				hdist= Getnbit(inf,5) + 1;
				hclen= Getnbit(inf,4) + 4;
				for (i=0; i<hclen; i++) inf->len[zz[i]]=Getnbit(inf,3);
				for (; i<19; i++) inf->len[zz[i]]=0;
				dht(&inf->J2, inf->len, 19, 7, inf->s2, inf->b2);
				hclen=hlit+hdist;
				for (i=0; i<hclen; ) 
				{
					int c= inf->J2.s[ReadHuffman(inf, inf->J2.b)];
					if (c>15) {
						if (c==16) i+= repn(inf->len+i, inf->len[i-1], Getnbit(inf,2) + 3);	// rep previous
						else if (c==17) i+= repn(inf->len+i, 0, Getnbit(inf,3) + 3);		// rep 0 for 3 - 10 times. 
						else if (c==18) i+= repn(inf->len+i, 0, Getnbit(inf,7) + 11);		// rep 0 for 11 - 138 times 								
						else return inf->ERROR=INFLENUMERR_GT18;
					}
					else inf->len[i++]=c;
				}
				dht(&inf->J2, inf->len,      hlit,  15, inf->s2, inf->b2);
				dht(&inf->J3, inf->len+hlit, hdist, 15, inf->s3, inf->b3);
				inflt(inf);
				break;

			case 3: 
				return inf->ERROR=INFLENUMERR_INVALIDBTYPE;
		}

	} while (!bfinal);	

	return inf->ERROR=INFLENUM_OK;
}
