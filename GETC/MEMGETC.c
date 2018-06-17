/*********************************************************

  This is for testing the memory input: 
  implementation of GETC.h for in-memory input. 
  Useful for small image files linked with the kernel
  (then OPEN() returns only an address). 

***********************************************************/

#define _CRT_SECURE_NO_DEPRECATE 1
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <STDLIB.H>


	static unsigned char *F;
	static int FPointer;


	// GETC and multibyte readers 

	extern int GETC()		
	{
		return F[FPointer++];
	}


	// multibyte helpers: host-endian independent - if evaluated in right order (ie. don't optimize)

	extern int GETWbi()		// 16-bit big-endian
	{
		return ( GETC()<<8 ) | GETC();
	}

	extern int GETDbi()		// 32-bit big-endian input
	{
		return ( GETC()<<24 ) | ( GETC()<<16 ) | ( GETC()<<8 ) | GETC();
	}

	extern int GETWli()		// 16-bit little-endian
	{
		
		return GETC() | ( GETC()<<8 );
	}

	extern int GETDli()		// 32-bit little-endian
	{
		return GETC() | ( GETC()<<8 ) | ( GETC()<<16 ) | ( GETC()<<24 );
	}


	// seek/tell

	extern void SEEK(int d)		
	{
		FPointer+=d;
	}

	extern void POS(int d)		
	{
		FPointer=d;
	}

	extern int TELL()
	{
		return FPointer;
	}


	// OPEN/CLOSE

	extern void *OPEN(char *f)
	{
		int fh; 
		F=0;
		FPointer=0;
		printf("Opening %s\n", f);
		_set_fmode(_O_BINARY);		
		if( (fh = _open( f, _O_RDONLY, 0 )) != -1 )
		{
			F = malloc(_filelength(fh));
			if ( 0 == F ) return 0;
			_read(fh, F, _filelength(fh));
			_close( fh );
			return F;
		}
		else printf("Error opening %s\n", f);
		return F;
	}

	extern void CLOSE()
	{
		if (F) free(F);
	}
