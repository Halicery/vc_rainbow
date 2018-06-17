/*****************************************************************************
	This VC++ 2005 project was used for developing and testing RAINBOW, 
	the re-entrant JPEG/PNG/GIF/TIFF-decoder made originally for a homebrew 
	Operating System.  

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

extern void jpeg2bmp(char *arg);
extern void tiff2bmp(char *arg);
extern void png2bmp(char *arg);
extern void gif2bmp(char *arg);
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

static void onefile(char *path, void (*_2bmp)(char*))
{
	if ( OPEN(path) )
	{
		_2bmp(path);
		CLOSE();
	}
}

static void onedir(char *dirpath, char *pattern, void (*_2bmp)(char*))		// pattern example "*.png"
{
		struct _finddata_t fileInfo;
		intptr_t fPtr;

		_makepath(_Path, "", dirpath, pattern, "");

		if ((fPtr = _findfirst(_Path,  &fileInfo )) == -1L) return;
		else do
		{
			_makepath(_Path, "", dirpath, fileInfo.name, "");
			onefile(_Path, _2bmp);
		}
		while (_findnext(fPtr, &fileInfo) == 0);

		_findclose(fPtr);
}

int main(int argc, char* argv[])
{
	char *filename;


	// GIF TEST
	filename= "images/FRACT002.GIF";		// interlaced GIF
	filename= "images/Light Gray Canvas.gif";		// interlaced GIF
	filename= "images/everest.gif";		// DEFERRED CLEAR CODE
	onefile(filename, gif2bmp);
	filename= "images/lierelva-web.gif";	// big
	onefile(filename, gif2bmp);
	filename= "images/gif_intr.gif";		// interlaced GIF
	onefile(filename, gif2bmp);
	onedir("images/gif", "*.gif", gif2bmp);



	// JPEG TEST
	//unzip2folder("ITU83.zip");	// unzip all and decode all
	//onedir("ITU83", "*.jpg", jpeg2bmp);
	filename= "images/20061020539_1.jpg"; // 2x2 1x2 1x1 sub-sampling
	onefile(filename, jpeg2bmp);
	filename= "images/feka.jpg";	// big progressive
	onefile(filename, jpeg2bmp);
	filename= "images/firerainbow.jpg";	// progressive JPEG
	onefile(filename, jpeg2bmp);
	filename= "images/6502_blueprint full.jpg"; // even bigger
	filename= "images/liliom.jpg";
	onefile(filename, jpeg2bmp);
	filename= "images/bedroom_arithmetic.jpg";	// arithmetic coded 
	onefile(filename, jpeg2bmp);


	// ZIP TEST
	filename= "idct.zip";
	filename= "NetFx20SP2_x86.zip";
	filename= "images/test.zip";
	filename= "images/hevc.zip";
	filename= "images/6502_blueprint full.zip";
	//unzip2folder(filename);	
	unzip2folder("images/PngSuite-2017jul19.zip");	// test unzip and decode all PNG 

	// PNG TEST
	onedir("images/PngSuite-2017jul19", "*.png", png2bmp);
	filename= "images/PngSuite-2017jul19/s04i3p01.png";
	filename= "images/eu.png";
	onefile(filename, png2bmp);
	filename= "images/bedroom_arithmetic_tiff_without_predictor.png";
	filename= "images/6502_blueprint full.png";
	filename= "images/light_bulb.png";
	onefile(filename, png2bmp);



	// TIFF TEST
	onefile("images/bedroom_arithmetic.tif", tiff2bmp);
	onefile("images/strike.tif", tiff2bmp);
	onefile("images/STSCI-H-p1729a-f-3000x2400.tif", tiff2bmp);	// Mars Adobe Diff Pred LZW
	onefile("images/L_01_012_027.tif", tiff2bmp);// compression 7: "Technote2 overrides old-style JPEG compression, and defines 7 = JPEG ('new-style' JPEG)
	onefile("images/lierelva-web.tif", tiff2bmp);
	onefile("images/firerainbow.tif", tiff2bmp);
	onefile("images/nagy.tif", tiff2bmp);
	onefile("images/pici.tif", tiff2bmp);

}

