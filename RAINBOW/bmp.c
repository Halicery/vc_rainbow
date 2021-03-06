/*******************************************************************************

  All BM- and BMP-related part put together 
  in one file for simplicity. 

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

#include "bmp.h"

/*************************************** 
               BMP-FILE 
***************************************/

extern int BmpSize(enum BITPERPIXELENUM bppenum, int Width, int Height)
{
	return PAD2DWORD( Width * BM_WRITER[bppenum].BPP ) * Height + sizeof(struct BMP);
}

extern void BmpPrep(struct BMP *bmp, enum BITPERPIXELENUM bppenum, int Width, int Height)
{
	struct BMW *bmw= BM_WRITER + bppenum;
	bmp->biPitch= PAD2DWORD( Width * bmw->BPP );

	//BITMAPINFOHEADER
	{
		int h= (Height<0)? -Height : Height;			// never up-side-down DIBs
		bmp->bmih.biSize= sizeof(BITMAPINFOHEADER);		// SHOULD BE 40 (= Windows DIB header BITMAPINFOHEADER)
		bmp->bmih.biWidth=Width;
		bmp->bmih.biHeight=-h;
		bmp->bmih.biPlanes=1;
		bmp->bmih.biBitCount= bmw->biBitCount;

		// RGB565 has biCompression=BI_BITFIELDS (3) and 3 DWORD mask values
		// others biCompression=BI_RGB (0)
		if (bmp->bmih.biCompression = bmw->biCompression)
		{
			bmp->bmiColors[0]= 0xF800;
			bmp->bmiColors[1]= 0x07E0;
			bmp->bmiColors[2]= 0x001F;
		}

		bmp->bmih.biSizeImage= bmp->biPitch * Height;
		//bmih->biXPelsPerMeter=bmih->biYPelsPerMeter= 0;
		bmp->bmih.biClrUsed=0;	// needed for proper BMP for f.ex. MS Paint 
		//bmih->biClrImportant=0;
	}

	//BITMAPFILEHEADER
	{
		bmp->bmfh.bfType= 0x4D42;//='BM'
		bmp->bmfh.bfReserved=0;
		bmp->bmfh.bfSize= bmp->bmih.biSizeImage + sizeof(struct BMP);
		bmp->bmfh.bfOffBits= sizeof(struct BMP);
	}

	// 2-byte padding anyway: put a bpp-string here:
	*(short*)bmp->true_bpp_string = *(short*)bmw->true_bpp_string;
	bmp->true_bpp= bmw->true_bpp;
	// For fun
	{
		static char T[BMPTEXTLENGTH] = "-bit BMP built by Rainbow (halicery.com)";
		int i;
		for( i=0; i<BMPTEXTLENGTH; i++) bmp->mytext[i] = T[i];
	}
}


/***************************************************************************** 
  BM-PIXEL MEMORY WRITERS
  From 8-bit color samples of various format 
  to BM-pixels in 4 DIRECT COLOR format: 

                                                    
      B  G  R  A  .. .. .. .. .. .. .. .. -->     32-bit RGB8888 
                                 
      B  G  R  .. .. .. .. .. .. .. .. .. -->     24-bit RGB888
                                 
      bg gr .. .. .. .. .. .. .. .. .. .. -->     16-bit RGB565
                                
      bg gr .. .. .. .. .. .. .. .. .. .. -->     15-bit RGB555

*****************************************************************************/

static WORD // for speed
RGB5[256]=   { 0,0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,10,10,10,10,10,10,10,10,11,11,11,11,11,11,11,11,12,12,12,12,12,12,12,12,13,13,13,13,13,13,13,13,13,14,14,14,14,14,14,14,14,15,15,15,15,15,15,15,15,16,16,16,16,16,16,16,16,17,17,17,17,17,17,17,17,18,18,18,18,18,18,18,18,18,19,19,19,19,19,19,19,19,20,20,20,20,20,20,20,20,21,21,21,21,21,21,21,21,22,22,22,22,22,22,22,22,22,23,23,23,23,23,23,23,23,24,24,24,24,24,24,24,24,25,25,25,25,25,25,25,25,26,26,26,26,26,26,26,26,27,27,27,27,27,27,27,27,27,28,28,28,28,28,28,28,28,29,29,29,29,29,29,29,29,30,30,30,30,30,30,30,30,31,31,31,31,31,},
RGB5_5[256]= { 0<<5,0<<5,0<<5,0<<5,0<<5,1<<5,1<<5,1<<5,1<<5,1<<5,1<<5,1<<5,1<<5,2<<5,2<<5,2<<5,2<<5,2<<5,2<<5,2<<5,2<<5,3<<5,3<<5,3<<5,3<<5,3<<5,3<<5,3<<5,3<<5,4<<5,4<<5,4<<5,4<<5,4<<5,4<<5,4<<5,4<<5,4<<5,5<<5,5<<5,5<<5,5<<5,5<<5,5<<5,5<<5,5<<5,6<<5,6<<5,6<<5,6<<5,6<<5,6<<5,6<<5,6<<5,7<<5,7<<5,7<<5,7<<5,7<<5,7<<5,7<<5,7<<5,8<<5,8<<5,8<<5,8<<5,8<<5,8<<5,8<<5,8<<5,9<<5,9<<5,9<<5,9<<5,9<<5,9<<5,9<<5,9<<5,9<<5,10<<5,10<<5,10<<5,10<<5,10<<5,10<<5,10<<5,10<<5,11<<5,11<<5,11<<5,11<<5,11<<5,11<<5,11<<5,11<<5,12<<5,12<<5,12<<5,12<<5,12<<5,12<<5,12<<5,12<<5,13<<5,13<<5,13<<5,13<<5,13<<5,13<<5,13<<5,13<<5,13<<5,14<<5,14<<5,14<<5,14<<5,14<<5,14<<5,14<<5,14<<5,15<<5,15<<5,15<<5,15<<5,15<<5,15<<5,15<<5,15<<5,16<<5,16<<5,16<<5,16<<5,16<<5,16<<5,16<<5,16<<5,17<<5,17<<5,17<<5,17<<5,17<<5,17<<5,17<<5,17<<5,18<<5,18<<5,18<<5,18<<5,18<<5,18<<5,18<<5,18<<5,18<<5,19<<5,19<<5,19<<5,19<<5,19<<5,19<<5,19<<5,19<<5,20<<5,20<<5,20<<5,20<<5,20<<5,20<<5,20<<5,20<<5,21<<5,21<<5,21<<5,21<<5,21<<5,21<<5,21<<5,21<<5,22<<5,22<<5,22<<5,22<<5,22<<5,22<<5,22<<5,22<<5,22<<5,23<<5,23<<5,23<<5,23<<5,23<<5,23<<5,23<<5,23<<5,24<<5,24<<5,24<<5,24<<5,24<<5,24<<5,24<<5,24<<5,25<<5,25<<5,25<<5,25<<5,25<<5,25<<5,25<<5,25<<5,26<<5,26<<5,26<<5,26<<5,26<<5,26<<5,26<<5,26<<5,27<<5,27<<5,27<<5,27<<5,27<<5,27<<5,27<<5,27<<5,27<<5,28<<5,28<<5,28<<5,28<<5,28<<5,28<<5,28<<5,28<<5,29<<5,29<<5,29<<5,29<<5,29<<5,29<<5,29<<5,29<<5,30<<5,30<<5,30<<5,30<<5,30<<5,30<<5,30<<5,30<<5,31<<5,31<<5,31<<5,31<<5,31<<5,},
RGB5_10[256]={ 0<<10,0<<10,0<<10,0<<10,0<<10,1<<10,1<<10,1<<10,1<<10,1<<10,1<<10,1<<10,1<<10,2<<10,2<<10,2<<10,2<<10,2<<10,2<<10,2<<10,2<<10,3<<10,3<<10,3<<10,3<<10,3<<10,3<<10,3<<10,3<<10,4<<10,4<<10,4<<10,4<<10,4<<10,4<<10,4<<10,4<<10,4<<10,5<<10,5<<10,5<<10,5<<10,5<<10,5<<10,5<<10,5<<10,6<<10,6<<10,6<<10,6<<10,6<<10,6<<10,6<<10,6<<10,7<<10,7<<10,7<<10,7<<10,7<<10,7<<10,7<<10,7<<10,8<<10,8<<10,8<<10,8<<10,8<<10,8<<10,8<<10,8<<10,9<<10,9<<10,9<<10,9<<10,9<<10,9<<10,9<<10,9<<10,9<<10,10<<10,10<<10,10<<10,10<<10,10<<10,10<<10,10<<10,10<<10,11<<10,11<<10,11<<10,11<<10,11<<10,11<<10,11<<10,11<<10,12<<10,12<<10,12<<10,12<<10,12<<10,12<<10,12<<10,12<<10,13<<10,13<<10,13<<10,13<<10,13<<10,13<<10,13<<10,13<<10,13<<10,14<<10,14<<10,14<<10,14<<10,14<<10,14<<10,14<<10,14<<10,15<<10,15<<10,15<<10,15<<10,15<<10,15<<10,15<<10,15<<10,16<<10,16<<10,16<<10,16<<10,16<<10,16<<10,16<<10,16<<10,17<<10,17<<10,17<<10,17<<10,17<<10,17<<10,17<<10,17<<10,18<<10,18<<10,18<<10,18<<10,18<<10,18<<10,18<<10,18<<10,18<<10,19<<10,19<<10,19<<10,19<<10,19<<10,19<<10,19<<10,19<<10,20<<10,20<<10,20<<10,20<<10,20<<10,20<<10,20<<10,20<<10,21<<10,21<<10,21<<10,21<<10,21<<10,21<<10,21<<10,21<<10,22<<10,22<<10,22<<10,22<<10,22<<10,22<<10,22<<10,22<<10,22<<10,23<<10,23<<10,23<<10,23<<10,23<<10,23<<10,23<<10,23<<10,24<<10,24<<10,24<<10,24<<10,24<<10,24<<10,24<<10,24<<10,25<<10,25<<10,25<<10,25<<10,25<<10,25<<10,25<<10,25<<10,26<<10,26<<10,26<<10,26<<10,26<<10,26<<10,26<<10,26<<10,27<<10,27<<10,27<<10,27<<10,27<<10,27<<10,27<<10,27<<10,27<<10,28<<10,28<<10,28<<10,28<<10,28<<10,28<<10,28<<10,28<<10,29<<10,29<<10,29<<10,29<<10,29<<10,29<<10,29<<10,29<<10,30<<10,30<<10,30<<10,30<<10,30<<10,30<<10,30<<10,30<<10,31<<10,31<<10,31<<10,31<<10,31<<10,},
RGB5_11[256]={ 0<<11,0<<11,0<<11,0<<11,0<<11,1<<11,1<<11,1<<11,1<<11,1<<11,1<<11,1<<11,1<<11,2<<11,2<<11,2<<11,2<<11,2<<11,2<<11,2<<11,2<<11,3<<11,3<<11,3<<11,3<<11,3<<11,3<<11,3<<11,3<<11,4<<11,4<<11,4<<11,4<<11,4<<11,4<<11,4<<11,4<<11,4<<11,5<<11,5<<11,5<<11,5<<11,5<<11,5<<11,5<<11,5<<11,6<<11,6<<11,6<<11,6<<11,6<<11,6<<11,6<<11,6<<11,7<<11,7<<11,7<<11,7<<11,7<<11,7<<11,7<<11,7<<11,8<<11,8<<11,8<<11,8<<11,8<<11,8<<11,8<<11,8<<11,9<<11,9<<11,9<<11,9<<11,9<<11,9<<11,9<<11,9<<11,9<<11,10<<11,10<<11,10<<11,10<<11,10<<11,10<<11,10<<11,10<<11,11<<11,11<<11,11<<11,11<<11,11<<11,11<<11,11<<11,11<<11,12<<11,12<<11,12<<11,12<<11,12<<11,12<<11,12<<11,12<<11,13<<11,13<<11,13<<11,13<<11,13<<11,13<<11,13<<11,13<<11,13<<11,14<<11,14<<11,14<<11,14<<11,14<<11,14<<11,14<<11,14<<11,15<<11,15<<11,15<<11,15<<11,15<<11,15<<11,15<<11,15<<11,16<<11,16<<11,16<<11,16<<11,16<<11,16<<11,16<<11,16<<11,17<<11,17<<11,17<<11,17<<11,17<<11,17<<11,17<<11,17<<11,18<<11,18<<11,18<<11,18<<11,18<<11,18<<11,18<<11,18<<11,18<<11,19<<11,19<<11,19<<11,19<<11,19<<11,19<<11,19<<11,19<<11,20<<11,20<<11,20<<11,20<<11,20<<11,20<<11,20<<11,20<<11,21<<11,21<<11,21<<11,21<<11,21<<11,21<<11,21<<11,21<<11,22<<11,22<<11,22<<11,22<<11,22<<11,22<<11,22<<11,22<<11,22<<11,23<<11,23<<11,23<<11,23<<11,23<<11,23<<11,23<<11,23<<11,24<<11,24<<11,24<<11,24<<11,24<<11,24<<11,24<<11,24<<11,25<<11,25<<11,25<<11,25<<11,25<<11,25<<11,25<<11,25<<11,26<<11,26<<11,26<<11,26<<11,26<<11,26<<11,26<<11,26<<11,27<<11,27<<11,27<<11,27<<11,27<<11,27<<11,27<<11,27<<11,27<<11,28<<11,28<<11,28<<11,28<<11,28<<11,28<<11,28<<11,28<<11,29<<11,29<<11,29<<11,29<<11,29<<11,29<<11,29<<11,29<<11,30<<11,30<<11,30<<11,30<<11,30<<11,30<<11,30<<11,30<<11,31<<11,31<<11,31<<11,31<<11,31<<11,},
RGB6_5[256]= { 0<<5,0<<5,0<<5,1<<5,1<<5,1<<5,1<<5,2<<5,2<<5,2<<5,2<<5,3<<5,3<<5,3<<5,3<<5,4<<5,4<<5,4<<5,4<<5,5<<5,5<<5,5<<5,5<<5,6<<5,6<<5,6<<5,6<<5,7<<5,7<<5,7<<5,7<<5,8<<5,8<<5,8<<5,8<<5,9<<5,9<<5,9<<5,9<<5,10<<5,10<<5,10<<5,10<<5,11<<5,11<<5,11<<5,11<<5,12<<5,12<<5,12<<5,12<<5,13<<5,13<<5,13<<5,13<<5,14<<5,14<<5,14<<5,14<<5,15<<5,15<<5,15<<5,15<<5,16<<5,16<<5,16<<5,16<<5,17<<5,17<<5,17<<5,17<<5,18<<5,18<<5,18<<5,18<<5,19<<5,19<<5,19<<5,19<<5,20<<5,20<<5,20<<5,20<<5,21<<5,21<<5,21<<5,21<<5,21<<5,22<<5,22<<5,22<<5,22<<5,23<<5,23<<5,23<<5,23<<5,24<<5,24<<5,24<<5,24<<5,25<<5,25<<5,25<<5,25<<5,26<<5,26<<5,26<<5,26<<5,27<<5,27<<5,27<<5,27<<5,28<<5,28<<5,28<<5,28<<5,29<<5,29<<5,29<<5,29<<5,30<<5,30<<5,30<<5,30<<5,31<<5,31<<5,31<<5,31<<5,32<<5,32<<5,32<<5,32<<5,33<<5,33<<5,33<<5,33<<5,34<<5,34<<5,34<<5,34<<5,35<<5,35<<5,35<<5,35<<5,36<<5,36<<5,36<<5,36<<5,37<<5,37<<5,37<<5,37<<5,38<<5,38<<5,38<<5,38<<5,39<<5,39<<5,39<<5,39<<5,40<<5,40<<5,40<<5,40<<5,41<<5,41<<5,41<<5,41<<5,42<<5,42<<5,42<<5,42<<5,42<<5,43<<5,43<<5,43<<5,43<<5,44<<5,44<<5,44<<5,44<<5,45<<5,45<<5,45<<5,45<<5,46<<5,46<<5,46<<5,46<<5,47<<5,47<<5,47<<5,47<<5,48<<5,48<<5,48<<5,48<<5,49<<5,49<<5,49<<5,49<<5,50<<5,50<<5,50<<5,50<<5,51<<5,51<<5,51<<5,51<<5,52<<5,52<<5,52<<5,52<<5,53<<5,53<<5,53<<5,53<<5,54<<5,54<<5,54<<5,54<<5,55<<5,55<<5,55<<5,55<<5,56<<5,56<<5,56<<5,56<<5,57<<5,57<<5,57<<5,57<<5,58<<5,58<<5,58<<5,58<<5,59<<5,59<<5,59<<5,59<<5,60<<5,60<<5,60<<5,60<<5,61<<5,61<<5,61<<5,61<<5,62<<5,62<<5,62<<5,62<<5,63<<5,63<<5,63<<5,},
GREY555[256]={ 0,0,0,0,0,1057,1057,1057,1057,1057,1057,1057,1057,2114,2114,2114,2114,2114,2114,2114,2114,3171,3171,3171,3171,3171,3171,3171,3171,4228,4228,4228,4228,4228,4228,4228,4228,4228,5285,5285,5285,5285,5285,5285,5285,5285,6342,6342,6342,6342,6342,6342,6342,6342,7399,7399,7399,7399,7399,7399,7399,7399,8456,8456,8456,8456,8456,8456,8456,8456,9513,9513,9513,9513,9513,9513,9513,9513,9513,10570,10570,10570,10570,10570,10570,10570,10570,11627,11627,11627,11627,11627,11627,11627,11627,12684,12684,12684,12684,12684,12684,12684,12684,13741,13741,13741,13741,13741,13741,13741,13741,13741,14798,14798,14798,14798,14798,14798,14798,14798,15855,15855,15855,15855,15855,15855,15855,15855,16912,16912,16912,16912,16912,16912,16912,16912,17969,17969,17969,17969,17969,17969,17969,17969,19026,19026,19026,19026,19026,19026,19026,19026,19026,20083,20083,20083,20083,20083,20083,20083,20083,21140,21140,21140,21140,21140,21140,21140,21140,22197,22197,22197,22197,22197,22197,22197,22197,23254,23254,23254,23254,23254,23254,23254,23254,23254,24311,24311,24311,24311,24311,24311,24311,24311,25368,25368,25368,25368,25368,25368,25368,25368,26425,26425,26425,26425,26425,26425,26425,26425,27482,27482,27482,27482,27482,27482,27482,27482,28539,28539,28539,28539,28539,28539,28539,28539,28539,29596,29596,29596,29596,29596,29596,29596,29596,30653,30653,30653,30653,30653,30653,30653,30653,31710,31710,31710,31710,31710,31710,31710,31710,32767,32767,32767,32767,32767},
GREY565[256]={ 0,0,0,32,32,2081,2081,2113,2113,2113,2113,2145,2145,4194,4194,4226,4226,4226,4226,4258,4258,6307,6307,6339,6339,6339,6339,6371,6371,8420,8420,8452,8452,8452,8452,8484,8484,8484,10533,10565,10565,10565,10565,10597,10597,10597,12646,12678,12678,12678,12678,12710,12710,12710,14759,14791,14791,14791,14791,14823,14823,14823,16872,16904,16904,16904,16904,16936,16936,16936,18985,19017,19017,19017,19017,19049,19049,19049,19049,21130,21130,21130,21130,21162,21162,21162,21162,23211,23243,23243,23243,23243,23275,23275,23275,25324,25356,25356,25356,25356,25388,25388,25388,27437,27469,27469,27469,27469,27501,27501,27501,27501,29582,29582,29582,29582,29614,29614,29614,29614,31695,31695,31695,31695,31727,31727,31727,31727,33808,33808,33808,33808,33840,33840,33840,33840,35921,35921,35921,35921,35953,35953,35953,35953,38034,38034,38034,38034,38066,38066,38066,38066,38098,40147,40147,40147,40179,40179,40179,40179,40211,42260,42260,42260,42292,42292,42292,42292,42324,44373,44373,44373,44373,44405,44405,44405,44405,46486,46486,46486,46486,46518,46518,46518,46518,46550,48599,48599,48599,48631,48631,48631,48631,48663,50712,50712,50712,50744,50744,50744,50744,50776,52825,52825,52825,52857,52857,52857,52857,52889,54938,54938,54938,54970,54970,54970,54970,55002,57051,57051,57051,57083,57083,57083,57083,57115,57115,59164,59164,59196,59196,59196,59196,59228,59228,61277,61277,61309,61309,61309,61309,61341,61341,63390,63390,63422,63422,63422,63422,63454,63454,65503,65503,65535,65535,65535};


// From 8-bit samples of BGR-order

	static void Write16(PBYTE src, PWORD dst)		// B G R ---> bg gr
	{
		dst[0]= RGB5_11[src[2]] | RGB6_5[src[1]] | RGB5[src[0]];
	}

	static void Write15(PBYTE src, PWORD dst)		// B G R ---> bg gr
	{
		dst[0]= RGB5_10[src[2]] | RGB5_5[src[1]] | RGB5[src[0]];
	}

	static void Write24(PBYTE src, PBYTE dst) // B G R ---> B G R (as Write32? but 1. scanlines are DWORD-padded, ok, but unaligned DWORD writes? 2. Transparent GIF: PAL only so it's ok)
	{
		dst[0]= src[0];
		dst[1]= src[1];
		dst[2]= src[2];
	}


// From OpenGL/RGBA-style byte order (PNG, TIFF and GIF-palette-conversion) 

	static void WriteRGBA16(PBYTE src, PWORD dst)		// R G B ---> bg gr 
	{
		dst[0]= RGB5_11[src[0]] | RGB6_5[src[1]] | RGB5[src[2]];
	}

	static void WriteRGBA15(PBYTE src, PWORD dst)		// R G B ---> bg gr
	{
		dst[0]= RGB5_10[src[0]] | RGB5_5[src[1]] | RGB5[src[2]];
	}

	static void WriteRGBA24(PBYTE src, PBYTE dst)	// R G B (A) ---> B G R (A) 
	{
		dst[0]= src[2];
		dst[1]= src[1];
		dst[2]= src[0];
		//dst[3]= src[3]; // skip Alpha, no BMP/FB support
	}

// From greyscale 1-byte for PNG, JPEG (TIFF?)

	static void WriteGREY16(PBYTE src, PWORD dst)	// Y ---> yy yy
	{
		dst[0]= GREY565[src[0]];
	}

	static void WriteGREY15(PBYTE src, PWORD dst)	// Y ---> yy yy
	{
		dst[0]= GREY555[src[0]];
	}

	static void WriteGREY24(PBYTE src, PBYTE dst)	// Y ---> Y Y Y (A)
	{
		dst[0]= src[0];       // B
		dst[1]= src[0];       // G
		dst[2]= src[0];       // R
		//dst[3]= src[3]; // A (skip, no BMP/FB support)
	}


// From pre-converted PALETTE

	static void Copy2(PWORD src, PWORD dst)
	{
		dst[0]= src[0];
	}

	static void Copy3(PBYTE src, PBYTE dst)	// bc of GIF and transparency cannot do 32bit
	{
		dst[0]= src[0];
		dst[1]= src[1];
		dst[2]= src[2];
	}

	static void Copy4(PDWORD src, PDWORD dst)
	{
		dst[0]= src[0];
	}



// Initialize 4 structs per BITPERPIXELENUM:
// (void*) for GCC warnings

struct BMW BM_WRITER[4] =
{
	{ (void*)Write16, (void*)WriteRGBA16, (void*)WriteGREY16, (void*)Copy2, 2, 16, 3, 16, "16" }, // BITPERPIXEL16: biCompression=BI_BITFIELDS(3) and 3 color masks
	{ (void*)Write15, (void*)WriteRGBA15, (void*)WriteGREY15, (void*)Copy2, 2, 15, 0, 16, "15" }, // BITPERPIXEL15, BMP biBitCount=16(!)
	{ (void*)Write24, (void*)WriteRGBA24, (void*)WriteGREY24, (void*)Copy3, 3, 24, 0, 24, "24" }, // BITPERPIXEL24
	{ (void*)Write24, (void*)WriteRGBA24, (void*)WriteGREY24, (void*)Copy4, 4, 32, 0, 32, "32" }, // BITPERPIXEL32
};



