/*
    GIF-STREAM decoder          
	
	Usage:          
                                                         
		GIFInfo()
		GIFSetup()
		while ( GIFNextImage() )
		{
			GIFDecodeImage();
			GIFConvertImage();
		}

*
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

#include <stdio.h>
#include <malloc.h>

#include "GETC.h" 
#include "bmp.h" 
#include "lzw.h"	//	lzw.c now handles "DEFERRED CLEAR CODE IN LZW COMPRESSION", see GIF98a.txt
#include "gif.h"


/********************  Callback from lzw ************************************************************************/

static int NextByte(struct GIFINFO *g)
{
	if (0==g->BlkCnt) 
	{
		g->BlkCnt= GETC();
	    if (0==g->BlkCnt) 
			printf("LZW STREAM ERROR: BLOCK TERMINATOR?\n"); // TODO: bail out
	}
	g->BlkCnt--;
	return GETC();
}


/**************************************** Convert to BM ***********************************************************/

static unsigned char *ImagePass(struct GIFINFO *g, unsigned char *src, int Line, int LineInc)
{
		int BmpLineInc, x;
		unsigned char *BmpPixel, *BmpLine;
		
		BmpLine= g->DSTaddr + g->DSTpitch * (Line + g->ImageTop) + g->ImageLeft * g->bmBPP;
		BmpLineInc= LineInc * g->DSTpitch;

		while (Line < g->ImageHeight ) 
		{
			BmpPixel= BmpLine;

			for (x=0; x < g->ImageWidth; x++)	
			{				
				// v2: we skip transparent pixels
				int ColorIdx= *src++;
				if (g->GraphicColorIndex != ColorIdx) g->CopyPixel( g->TrueColorTable + ColorIdx, BmpPixel); //copy from converted palette
				BmpPixel += g->bmBPP;
			}

			Line += LineInc;
			BmpLine += BmpLineInc;
		}
		return src; // for interlace to continue
}

extern void GIFConvertImage(struct GIFINFO *g)
{
	if (g->ImagePacked & 64)	// 4-pass interlaced gif image
	{
		unsigned char *src;
		src= ImagePass(g, g->ImageData, 0, 8);
		src= ImagePass(g, src, 4, 8);
		src= ImagePass(g, src, 2, 4);
		src= ImagePass(g, src, 1, 2);
	}
	else ImagePass(g, g->ImageData, 0, 1);
}

extern enum ENUMGIF GIFDecodeImage(struct GIFINFO *g)
{
	// <-- points to Image Data (block-ed LZW-stream)
	int MinCodeSize= GETC();
	if ( MinCodeSize<2 || MinCodeSize>8 ) return g->ERROR=ENUMGIF_ERR_INVALIDCODESIZE;	

	g->BlkCnt= GETC();

	// <-- points to first byte of LZW data
	if ( 0 == lzw_decompress_gif(g->lzw, g->ImageData, MinCodeSize) ) return g->ERROR=ENUMGIF_ERR_LZW;//TODO	

	// <-- Here we should point to Block Terminator (0x00)
	if (GETC()) return g->ERROR=ENUMGIF_ERR_LZW_BLOCK_TERMINATOR;

	return g->ERROR=ENUMGIF_OK;
}

static void SkipBlocks()
{
	int BlockSize;
	while (BlockSize=GETC()) SEEK(BlockSize);
}

static void write_comment()
{
	int i, BlockSize;
	printf("  COMMENT: ");
	while (BlockSize=GETC()) for(i=0;i<BlockSize;i++) printf("%c", GETC());
	printf("\n");
}

static void SetTrueColorTable(struct GIFINFO *g, int *TrueColorTable, int Packed)
{
	if (Packed & 0x80)		// If Color Table is present
	{
		int NumberOfColors= (1<<((Packed&7)+1));
		BYTE rgb[4];
		int i;
		for (i=0; i < NumberOfColors; i++) 
		{
			rgb[0]=GETC();//R
			rgb[1]=GETC();//G
			rgb[2]=GETC();//B
			g->WritePixel( rgb, TrueColorTable + i ); // write as target format --> later copy
		}
		g->TrueColorTable= TrueColorTable;
	}
	else {
		g->TrueColorTable= g->GlobalTrueColorTable;	// Fall-back 
	}
}

extern enum ENUMGIF GIFNextImage(struct GIFINFO *g)		// finds next frame in gif stream.. decompresses it and returns g. End-of-gif returns NULL. 
{
	int ID;

	g->has_GraphicsControlExtension=0;
	g->has_ApplicationExtension= 0;

	g->GraphicColorIndex= 256;	// set transparency to none: default
	g->GraphicPacked= 0; // in case the block is missing, set Disposal to none

	while ( (ID=GETC()) != 0x2C ) {						// until Image Descriptor.... or END OF GIF (!)
			switch (ID) {
				case 0x21:		// Extension Introducer?
					switch (GETC()) {	// Label
						case 0xF9:		//GIFGRAPHICCONTROL						// Important! Transparency, Disposal, Delaytime is here! 
							GETC();	// SubBlockLength (always 04h)
							g->GraphicPacked= GETC();
							g->GraphicDelayTime= GETWli();
							if (0 == (g->GraphicPacked & 1)) GETC();	// skip
							else g->GraphicColorIndex= GETC();
							GETC();		// Block Terminator (should be 0)
							g->has_GraphicsControlExtension = 1;
							break;

						case 0xFF:		// GIFAPPLICATION. Usually NETSCAPE2.0
							GETC();		// BlockSize (always 0Bh)
							{
								int i;
								for(i=0;i<11;i++) g->APP[i]= GETC();
								g->APP[i]=0;
							}
							g->has_ApplicationExtension = 1; 
							SkipBlocks();
							break;

						case 0xFE:		// COMMENT. This block is OPTIONAL; any number of them may appear in the Data Stream. // The Comment Extension may be ignored by the decoder
							write_comment();
							break;

						case 0x01:		// GIFPLAINTEXT. Not implemented.
							SkipBlocks();
							break;

						default: 
							printf("UNKNOWN EXTENSION ID\n");
							return g->ERROR=ENUMGIF_ERR_UNKNOWN_EXTENSION_ID;
					}
					break;
				case 0x3B:	// END OF GIF
					printf("\n------------- END OF GIF --------------------\n\n");
					return 0;
				case 0x00:	// Some stuffing??? u.gif
					break;
				default: 
					printf("UNKNOWN BLOCK ID\n");
					return g->ERROR=ENUMGIF_ERR_UNKNOWN_BLOCK_ID;
			}
	}

	// <-- here we point to Image Descriptor
	g->ImageLeft= GETWli();
	g->ImageTop= GETWli();
	g->ImageWidth= GETWli();
	g->ImageHeight= GETWli();
	g->ImagePacked= GETC();

	SetTrueColorTable(g, g->LocalTrueColorTable, g->ImagePacked);
	g->Frame++;

	if (1)
	{
		printf("\nImage #%d\n", g->Frame);
		printf("  Image %d x %d at %d;%d\n", g->ImageWidth, g->ImageHeight, g->ImageLeft, g->ImageTop);
		printf("  Interlaced: %s\n", (g->ImagePacked & 64)? "YES":"NO");
		printf("  Local Color Table: %s", (g->ImagePacked & 128)? "YES":"NO");
		if (g->ImagePacked & 128) printf(" (%d entries)", (1<<((g->ImagePacked&7)+1)));
		printf("\n"); 

		if (g->has_ApplicationExtension) printf("  Application Extension: %s\n", g->APP);

		if (g->has_GraphicsControlExtension) 
		{
			static char *DISP[4]= { "0: No disposal specified", "1: Do not dispose", "2: Restore to background color", "3: Restore to previous" };
			printf("  Graphic Control Extension\n");
			printf("    Image Disposal method %s\n", DISP[(g->GraphicPacked >> 2 ) & 3]);
			if ( ((g->GraphicPacked >> 2) & 3) > 1)
				g=g;
			printf("    Transparent: ");
			if (g->GraphicPacked & 1) printf("YES (Color Index: %d)\n", g->GraphicColorIndex);
			else printf("NO\n");
		}
	}

	return g->ERROR=ENUMGIF_OK;
}


// An extra entry point: to setup parameters once becuase of subsequent calls to next/convert

extern enum ENUMGIF GIFSetup(struct GIFINFO *g, enum BITPERPIXELENUM bppenum, unsigned char *dst, int dstPitch)
{
	// BM-parameters we need
	struct BMW *bmw= BM_WRITER + bppenum;
	g->bmBPP= bmw->BPP;
	g->CopyPixel= bmw->CopyPixel;
	g->WritePixel= bmw->WriteRGBA;

	// <-- finish reading and converting Screen Color Palette if any
	SetTrueColorTable(g, g->GlobalTrueColorTable, g->ScreenPacked);

	g->gif_mem= malloc( sizeof(struct LZW) + g->ScreenWidth*g->ScreenHeight);
	if ( 0 == g->gif_mem) return g->ERROR=ENUMGIF_ERR_MALLOC;
	g->lzw= g->gif_mem;
	g->ImageData= (PBYTE)g->lzw + sizeof(struct LZW);

	g->DSTaddr= dst;
	g->DSTpitch= dstPitch;

	lzw_init_gif(g->lzw, (void*)NextByte, g); // for GCC warning
	g->Frame=0;

	// PREPARE CANVAS? If Global Color Table is present.. and first image is smaller than screen AND/OR transparent: fill CANVAS with global Bg-Color?
	return g->ERROR=ENUMGIF_OK;
}


extern enum ENUMGIF GIFInfo(struct GIFINFO *g)
{
	BYTE Version[2];

	// Init
	g->gif_mem= 0;

	// HEADER
	if (GETDli() != 0x38464947) return g->ERROR=ENUMGIF_ERR_HEADER;
	Version[0]= GETC();
	Version[1]= GETC();

	// Logical Screen Descriptor: REQUIRED
	g->ScreenWidth= GETWli();
	g->ScreenHeight= GETWli();
	g->ScreenPacked= GETC();
	g->ScreenBackgroundColor= GETC();
	GETC();	// Pixel Aspect Ratio

	if(1)
	{
		printf("\nHeader:\n");
		printf("  Version: GIF8%c%c\n", Version[0], Version[1]);
		printf("  Logical Screen: %d x %d\n", g->ScreenWidth, g->ScreenHeight);

		printf("  Global Color Table: ");
		if (g->ScreenPacked & 0x80) {
			printf("YES (%d entries)\n", 1 << ( (g->ScreenPacked & 7) + 1) );
			printf("  Background Color Index: %d\n", g->ScreenBackgroundColor );
		}
		else printf("NO\n");

		printf("  Color Resolution: %d-bit\n",  ((g->ScreenPacked >> 4)&7)+1);
	}

	return g->ERROR=ENUMGIF_OK;
}

