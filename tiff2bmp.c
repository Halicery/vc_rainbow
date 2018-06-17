/*****************************************************************************

  A simple client for the tiff decoder. The same concept as for png: 

	1.	Info() returns information about the image 
	2.	Decode() 
	3.	Convert()

*********************************************************************/


#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
#include <malloc.h>

#include "bmp.h" 
#include "lzw.h"
#include "tiff.h"

extern void WriteBmp(struct BMP *bmp, char *path);

static struct TIFFINFO T;


extern void tiff2bmp(char *path)
{
	enum BITPERPIXELENUM FORMAT= BITPERPIXEL32;

	if ( TIFFInfo(&T) ) 
	{
		int bmpSize= BmpSize(FORMAT, T.ImageWidth, T.ImageHeight);
		struct BMP *bmp= malloc(bmpSize);

		if ( bmp )
		{
			BmpPrep(bmp, FORMAT, T.ImageWidth, T.ImageHeight);

			TIFFDecode(&T);
			TIFFConvert(&T, FORMAT, (PBYTE)bmp + bmp->bmfh.bfOffBits, bmp->biPitch);

			WriteBmp(bmp, path);
			free(bmp);	
		}
	}

	if (T.tiff_mem) free(T.tiff_mem);

}
