/************************************************************* 
	TIFF decoder

	The only reason I wrote this was to use the 
	LZW-decompressor with another client than GIF

	Tested for TIFF images saved by MS Paint (LZW with 
	Differencing Predictor) and another particular test 
	image: strike.tif (LZW).

	The decoder is based on TIFF Revision 6.0 (TIFF6.PDF)

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

// GCC -c -Wno-multichar tiff.c

#include <stdio.h> //printf
#include <malloc.h>

#include "GETC.h" 
#include "bmp.h"		// for convert
#include "lzw.h"		// only TIFF with LZW compression
#include "tiff.h"


#define Type_BYTE 1
#define Type_SHORT 3
#define Type_LONG 4

static void Read_Directory_Entry(struct TIFFINFO *t)
{
	t->DE.Tag = t->GETW();
	t->DE.Type= t->GETW();
	t->DE.Count= t->GETD();

	switch (t->DE.Type)
	{
		case Type_BYTE:
			t->DE_Reader= GETC;
			break;
		case Type_SHORT:
			t->DE_Reader= t->GETW;
			break;
		case Type_LONG:
			t->DE_Reader = t->GETD;
			break;
		default:
			t->DE_Reader = t->GETD;	// UNKNOWN TYPE!
	}	

	t->DE.Value = ( t->DE.Count > 1 )? t->GETD() : t->DE_Reader();	// a little unclear what is the size of the pointer? Same as Type or always 4-byte?
}



// Conversion works only for strike.tif and tif saved from MS Paint (8-bit RGB and RGBA TIFF) 

extern void TIFFConvert(struct TIFFINFO *t, enum BITPERPIXELENUM bppenum, unsigned char *dst, int dstPitch)
{
	unsigned char *BmpPixel, *src;
	int i, row, dstBPP=BM_WRITER[bppenum].BPP;
	void (*WriteRGBA)(void* src, void* dst) = BM_WRITER[bppenum].WriteRGBA;    // BM-pixel writer for TIFF RGBA-order

	if ( 2 == t->DifferencingPredictor ) // MS Paint saves .tif with LZW and Predictor: filter before convert
	{
		int ci, pred[4];
		src= t->tiff_mem;
		for (row=0; row<t->ImageHeight; row++) 
		{
			for (ci=0; ci < t->NumberOfComponents; ci++) pred[ci]= 0;
			for (i=0; i < t->ImageWidth; i++) {
				for (ci=0; ci < t->NumberOfComponents; ci++) pred[ci]= src[ci]+= pred[ci];	
				src+=t->NumberOfComponents;
			}
		}
	}

	src= t->tiff_mem;
	for (row=0; row<t->ImageHeight; row++) 
	{
		BmpPixel= dst;
		for (i=0; i < t->ImageWidth; i++) 
		{
			WriteRGBA(src, BmpPixel);
			src+=t->NumberOfComponents;
			BmpPixel+=dstBPP;
		}
		dst += dstPitch;
	}
}


extern int TIFFDecode(struct TIFFINFO *t)
{
	unsigned char *dst;
	int DataOffs;
	int StripOffset= t->StripOffset;		// position of the table of offsets to compressed data 
	int strip;

	t->tiff_mem= malloc(t->ImageWidth * t->ImageHeight * t->NumberOfComponents);		// 8-bit components
	if ( 0 == t->tiff_mem ) return 0;

	lzw_init_tiff(&t->U, (void*)GETC, 0);
	printf("Decoding...\n");
	dst= t->tiff_mem;

	for (strip=0; strip < t->NumberOfStrips; strip++) 
	{
		POS(StripOffset);
		DataOffs= t->StripOffset_Reader();
		StripOffset= TELL();
		POS(DataOffs);
		printf("%08x: strip data %d\n", DataOffs, strip+1);

		dst = lzw_decompress(&t->U, dst);

		if ( 0 == dst ) {
			printf("LZW ERROR\n");
			return 0;
		}
	}
	return 1;
}



extern int TIFFInfo(struct TIFFINFO *t)
{
	// Header 
	if (GETWli() == 'MM') t->GETW= GETWbi, t->GETD= GETDbi;	// set up endian-ness readers
	else t->GETW= GETWli, t->GETD= GETDli;
	if ( t->GETW() != 42 ) return 0;
	printf("Header: %s-endian\n", (t->GETW==GETWbi)?"big":"little");

	// some init
	t->DifferencingPredictor=0;
	t->tiff_mem=0;

	// <-- multiple linked ImageFileDirectory follows
	for (;;)
	{
		int NumberofDirectoryEntries;

		int IFDoffs = t->GETD();
		if ( 0 == IFDoffs ) break;	// Done. Offset of next IFD is NULL
		POS( IFDoffs );

		// <-- ImageFileDirectory follows
		NumberofDirectoryEntries= t->GETW();	
		printf("IFD %d entries\n", NumberofDirectoryEntries);
		IFDoffs+=2;

		while (NumberofDirectoryEntries--) 		// process N x (12-byte IFD Entry) Each entry's Value/Offset contains either the value itself (if fits) or a file_offset to the value(s)
		{
			Read_Directory_Entry(t);
			printf("IFD-Entry: %d\n", t->DE.Tag);

			switch (t->DE.Tag) {
				case 254:
					printf("  NewSubfileType: %d\n", t->DE.Value);
					break;
				case 256:
					t->ImageWidth= t->DE.Value;
					printf("  ImageWidth: %d\n", t->ImageWidth);
					break;
				case 257:
					t->ImageHeight= t->DE.Value;
					printf("  ImageHeight: %d\n", t->ImageHeight);
					break;
				case 258:		
					// BitsPerSample: possibly multiple 
					t->NumberOfComponents= t->DE.Count;
					printf("  %d x BitsPerSample: ", t->NumberOfComponents);
					if ( t->NumberOfComponents > 1 ) 
					{
						int n= t->NumberOfComponents;
						POS(t->DE.Value);
						while (n--) printf("%d ", t->DE_Reader());
						printf("\n");
					}
					else printf("%d\n", t->DE.Value);	// N/T
					break;
				case 259:
					printf("  Compression: %d", t->DE.Value);
					if ( 5 == t->DE.Value ) printf(" (LZW)\n"); 
					else 
					{
						printf(" (?)\n");
						return 0;	// decodes LZW only
					}
					break;
				case 262:
					t->PhotometricInterpretation = t->DE.Value;
					printf("  PhotometricInterpretation: %d (%s)\n", t->PhotometricInterpretation, (2==t->PhotometricInterpretation)?"RGB":"?");
					break;
				case 273:		
					// StripOffsets: address of compresed data (1 strip) or pointer to an array of StripOffsets
					t->NumberOfStrips= t->DE.Count;
					t->StripOffset_Reader= t->DE_Reader;
					t->StripOffset= t->DE.Value;
					printf("  Number of Strips: %d\n", t->NumberOfStrips);
					if ( 1 == t->NumberOfStrips ) t->StripOffset= TELL()-4;	// trick: points to value itself
					break;					
				case 277:
					printf("  SamplesPerPixel: %d\n", t->DE.Value);
					break;
				case 278:
					printf("  RowsPerStrip: %d\n", t->DE.Value);
					break;
				case 279:
					// StripByteCounts "For each strip, the number of bytes in the strip after compression." DONTCARE
					printf("    Number of StripByteCounts: %d\n", t->DE.Count);
					break;
				case 284:		
					// PlanarConfiguration: recommended 1=Chunky (RGBRGBRGB…)
					t->PlanarConfiguration = t->DE.Value;	
					printf("  PlanarConfiguration: %d (%s)\n", t->PlanarConfiguration, (t->PlanarConfiguration==1)?"Chunky/Packed-Pixel":"Planar");
					break;
				case 282:
					printf("  XResolution\n");
					break;
				case 283:
					printf("  YResolution\n");
					break;
				case 286:
					printf("  XPosition\n");
					break;
				case 287:
					printf("  YPosition\n");
					break;
				case 296:
					printf("  ResolutionUnit\n");
					break;
				case 338:
					printf("  ExtraSamples: %d\n", t->DE.Value);
					break;
				case 317:
					printf("  Differencing Predictor for LZW: ");
					t->DifferencingPredictor= t->DE.Value;
					printf("%s\n", t->DifferencingPredictor?"ON":"OFF");
					break;
			}
			POS( IFDoffs+=12 );
		}
	}
	return 1;
}
