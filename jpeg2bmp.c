/***********************************************

  Front-end for the JPEG DECODER

  1. JPEGDecode() --> into coefficient buffer 
  2. De-quantization
  3. IDCT
  4. Conversion: YUV-to-RGB only

  It has it's own fast integer scaled IDCT 
  together with de-quantization. See idct.c

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
#include <malloc.h>

#include "bmp.h"
#include "jpeg81.h"
#include "idct.h"

extern void WriteBmp(struct BMP *bmp, char *path);

static struct JPEGD J;		// a decoder object 


extern void jpeg2bmp(char *path)
{
	enum JPEGENUM err;
	struct JPEGD *j= &J;
	printf("JPEG decoding: \n");
	
	err= JPEGDecode(j);

	if ( err > 0 ) 
	{
		if ( 3 == j->Nf ) 
		{
			printf("Scaled de-quantization and IDCT.. ");
			{
				int c, i, n;

				// Pre-scale quant-tables
				int SQ[4][64];
				for (c=0; c<4 && j->QT[c][0]; c++) 
				{
					int *q= j->QT[c], *sq= SQ[c];
					for (i=0; i<64; i++) sq[i]= q[i] * SCALEM[zigzag[i]];
				}

				// DEQUANT + IDCT
				for (c=0; c<j->Nf; c++) 
				{
					struct COMP *C= j->Components+c;
					int *q= j->QT[C->Qi], *sq= SQ[C->Qi];

					for (n=0; n < C->du_size; n++)				
					{
						/*
						// <--- scaled idct 							
						int k, t[64];
						TCOEF *coef= du[x];
						t[0]= coef[0] * q[0] + 1024;							// dequant DC and level-shift (8-bit)
						for (k=1; k<64; k++) t[zigzag[k]] = coef[k] * q[k];		// dequant AC (+zigzag)
						idct_s(t, coef);
						*/
						
						// <--- scaled idct with dequant
						idct_sq( C->du[ (n / C->du_w) * C->du_width +  n % C->du_w ], sq );		
					}
				}
			}
			printf("done\n");
		}
		else printf("Not RGB\n");

		// Primitive yuv-rgb converter for all sub-sampling types, 24-bit BMP only
		if ( 3 == j->Nf ) 
		{
			enum BITPERPIXELENUM FORMAT= BITPERPIXEL24;
			int bmpSize= BmpSize(FORMAT, j->X, j->Y);
			struct BMP *bmp= malloc(bmpSize);
			printf("Malloc for BMP... ");
			if (bmp) 
			{
			    printf("OK\n");
				printf("YUV-to-RGB conversion.. ");
				BmpPrep(bmp, FORMAT, j->X, j->Y);
				{
					PBYTE BMPLINE= (PBYTE)bmp + bmp->bmfh.bfOffBits;

					int h0= j->Hmax / j->Components[0].Hi;
					int v0= j->Vmax / j->Components[0].Vi;
					int h1= j->Hmax / j->Components[1].Hi;
					int v1= j->Vmax / j->Components[1].Vi;
					int h2= j->Hmax / j->Components[2].Hi;
					int v2= j->Vmax / j->Components[2].Vi;

					int x, y;
					for (y=0; y < j->Y; y++) 
					{
						PBYTE BMPPIX= BMPLINE;

						TCOEF *C0= j->Components[0].du[ j->Components[0].du_width*((y/v0)/8) ] + 8*((y/v0)&7);
						TCOEF *C1= j->Components[1].du[ j->Components[1].du_width*((y/v1)/8) ] + 8*((y/v1)&7);
						TCOEF *C2= j->Components[2].du[ j->Components[2].du_width*((y/v2)/8) ] + 8*((y/v2)&7);

						for (x=0; x < j->X; x++) 
						{
							TCOEF c0= C0[ (x/h0/8)*64+((x/h0)&7) ];
							TCOEF c1= C1[ (x/h1/8)*64+((x/h1)&7) ];
							TCOEF c2= C2[ (x/h2/8)*64+((x/h2)&7) ];

							// ITU BT.601 full-range YUV-to-RGB integer approximation 
							{
								int y= (c0<<5)+16;
								int u=  c1-128;
								int v=  c2-128;

								*BMPPIX++= CLIP[(y + 57 * u)>>5];			// B
								*BMPPIX++= CLIP[(y - 11 * u - 23 * v)>>5];	// G
								*BMPPIX++= CLIP[(y + 45 * v)>>5];			// R
							}
						}
						BMPLINE += bmp->biPitch;
					}
				}
				printf("done\n");

				WriteBmp(bmp, path);
				free(bmp);
			} 
			else printf("FAILED!\n");
		}
	}
	else printf("ERROR CODE: %d\n", err);	

	if (j->jpeg_mem) free(j->jpeg_mem);
}
