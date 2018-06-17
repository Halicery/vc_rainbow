/***************************************************************************

  Client for the GIF-decoder: saves multiple BMP-files from the GIF-STREAM

		HEADER
		|
		IMAGE          ---->    _001.bmp
		|
		IMAGE          ---->    _002.bmp
		|
		...
	
*********************************************************************/

#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
#include <STDLIB.H>
#include <string.H>

#include "bmp.h"
#include "gif.h"

extern void WriteBmp(struct BMP *bmp, char *path);

static char _Path[_MAX_PATH], _Drive[_MAX_DRIVE], _Dir[_MAX_DIR], _Fname[_MAX_FNAME], _Ext[_MAX_EXT];
static struct GIFINFO G;


extern void gif2bmp(char *path)
{
	struct BMP *bmp= 0;
	struct GIFINFO *g=&G;
	enum BITPERPIXELENUM FORMAT= BITPERPIXEL16;

	if ( GIFInfo(g) > 0 )
	{
		int bmpSize= BmpSize(FORMAT, g->ScreenWidth, g->ScreenHeight);

		if ( bmp = malloc(bmpSize) )
		{
			BmpPrep(bmp, FORMAT, g->ScreenWidth, g->ScreenHeight);

			if ( GIFSetup(g, FORMAT, (PBYTE)bmp+bmp->bmfh.bfOffBits, bmp->biPitch) > 0)
			{
				_splitpath(path, _Drive, _Dir, _Fname, _Ext);
				_makepath(_Path, _Drive, _Dir, _Fname, "");
				
				while (GIFNextImage(g)) 
				{ 		
					// TODO: Disposal of image (u.gif)

					GIFDecodeImage(g);
					GIFConvertImage(g);
					
					sprintf(_Ext, "_%03d", g->Frame);
					WriteBmp(bmp, strcat(strcpy(_Fname, _Path), _Ext) );
				}
			}
		}
		else printf("Malloc for BMP fails\n");
	}

	if (g->ERROR<0) printf("GIF Error code: %d\n", g->ERROR);
	if (g->gif_mem) free(g->gif_mem);
	if (bmp) free(bmp);
}



