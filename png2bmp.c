/*****************************************************************************

  A simple client for the PNG decoder:

	1.	PNGInfo()
	2.	PNGDecode() 
	3.	PNGConvert()

*********************************************************************/


#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
#include <malloc.h>	

#include "bmp.h" 
#include "inflate.h"
#include "png.h"

extern void WriteBmp(struct BMP *bmp, char *path);

static struct PNGINFO P;	// a PNG-decoder object


extern void png2bmp(char *path)
{
	struct PNGINFO *p= &P;
	struct BMP *bmp= 0;
	enum BITPERPIXELENUM FORMAT= BITPERPIXEL32;

	if ( PNGInfo(p) > 0 ) 
	{
		int bmpSize= BmpSize(FORMAT, p->ImageWidth, p->ImageHeight);

		if ( bmp = malloc(bmpSize) )
		{
			BmpPrep(bmp, FORMAT, p->ImageWidth, p->ImageHeight);

			if ( PNGDecode(p) > 0 ) 
			{
				PNGConvert(p, FORMAT, (PBYTE)bmp + bmp->bmfh.bfOffBits, bmp->biPitch);
				WriteBmp(bmp, path);
			}
		}
		else printf("Malloc for BMP fails\n");
	}

	if (p->ERROR<0) printf("PNG Error code: %d\n", p->ERROR);
	if (p->png_mem) free(p->png_mem);
	if (bmp) free(bmp);
}
