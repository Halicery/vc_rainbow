/*****************************************************************************
	This VC++ project was used for developing and testing RAINBOW, 
	the re-entrant JPEG/PNG/GIF/TIFF-decoder made originally for a 
	home-brew Operating System.  

	It's quite verbose with a lots of comments and limited error-checking. 

	RAINBOW decodes and converts images to memory-bitmaps, in Direct Color
	15/16/24/32 bit-per-pixel BM-format that matches the video card's frame 
	buffer. The PCI SVGA cards I used (S3, Cirrus, ATi, SiS) were programmed 
	to one of these Direct Color Modes using a Linear Frame Buffer. 

	VC_RAINBOW implements different clients for RAINBOW and writes bitmaps 
	as BMP-files on disk. Main is only responsible for opening/closing files 
	and call the appropriate BMP-converter client for RAINBOW. 

	Project dependecies:

	  GETC            GETC   RAINBOW  
	   |                |       |
	   |                +--+----+
	  RAINBOW              |
						VC_RAINBOW


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

#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
#include <io.h>
#include <STDLIB.H>

#include "GETC.h" // input file OPEN/CLOSE
#include "bmp.h"  // output BMP file

extern int jpeg2bmp(char *arg);
extern int tiff2bmp(char *arg);
extern int png2bmp(char *arg);
extern int gif2bmp(char *arg);
extern int unzip2folder(char *arg);


static char _Path[_MAX_PATH], _Drive[_MAX_DRIVE], _Dir[_MAX_DIR], _Fname[_MAX_FNAME], _Ext[_MAX_EXT];	// _splitpath/_makepath <stdlib.h>

extern void WriteBmp(struct BMP *bmp, char *path)
{
	_splitpath(path, _Drive, _Dir, _Fname, _Ext);
	_makepath(_Path, _Drive, _Dir, _Fname, "bmp");

	printf("Writing %d-bpp BMP: %s\n", bmp->true_bpp, _Path);
		{
			FILE *f= fopen(_Path, "wb");
			fwrite(bmp, bmp->bmfh.bfSize, 1, f);
			fclose(f);
		}
}



static void onefile(char *path, int (*f)(char *))
{
	if(  OPEN(path) )
	{
		f(path);
		CLOSE();
	}
}

static void onedir(char *dirpath, char *pattern, int (*f)(char *arg))		// pattern example "*.png"
{
		struct _finddata_t fileInfo;
		intptr_t fPtr;

		_makepath(_Path, "", dirpath, pattern, "");

		if ((fPtr = _findfirst(_Path,  &fileInfo )) == -1L) return;
		else do
		{
			_makepath(_Path, "", dirpath, fileInfo.name, "");
			onefile(_Path, f);
		}
		while (_findnext(fPtr, &fileInfo) == 0);

		_findclose(fPtr);
}



int main(int argc, char* argv[])
{
	// JPEG TEST
	onefile("images/bedroom_arithmetic.jpg", jpeg2bmp); // arithmetic coding 4:4:4
	onefile("images/liliom.jpg", jpeg2bmp); // baseline 4:2:0 Restart Markers
	onefile("images/firerainbow.jpg", jpeg2bmp); // progressive JPEG

	// GIF TEST
	onefile("images/everest.gif", gif2bmp); // DEFERRED CLEAR CODE
	onefile("images/FRACT002.GIF", gif2bmp); // DEFERRED CLEAR CODE
	onedir("images/gif", "*.gif", gif2bmp); // GIF animations

	// TIFF TEST
	onefile("images/strike.tif", tiff2bmp); // Adobe, transparent (not implemented)
	onefile("images/nagy.tif", tiff2bmp); // saved by MS Paint

	// ZIP TEST
	unzip2folder("images/PngSuite-2017jul19.zip");  // unzip and decode all PNG 

	// PNG TEST
	onedir("images/PngSuite-2017jul19", "*.png", png2bmp);

}

