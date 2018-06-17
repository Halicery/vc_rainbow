/*************************************************************

	For testing unzip and inflate

*************************************************************/

#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
#include <STDLIB.H>
#include <direct.h>	// _mkdir()
#include <malloc.h>

#include "GETC.h" // OPEN/CLOSE
#include "inflate.h"
#include "unzip.h"


static struct ZIP Z;

static char _Path[_MAX_PATH], _Drive[_MAX_DRIVE], _Dir[_MAX_DIR], _Fname[_MAX_FNAME], _Ext[_MAX_EXT];



static void WriteFile(struct ZIP *z, void *data)
{
	_makepath(_Path, "", _Dir, _Fname, "");

	printf("Writing %s\n", _Path);
	{
		FILE *f= fopen(_Path, "wb");
		fwrite(data, z->lfh.uncompressed_size, 1, f);
		fclose(f);
	}
}

extern int unzip2folder(char *arg)
{
	struct ZIP *z= &Z;

	if ( OPEN(arg) ) 
	{		
		_splitpath(arg, _Drive, _Dir, _Fname, _Ext);
		
		// under dir:
		_makepath(_Path, _Drive, _Dir, _Fname, "");
		_mkdir(_Path);
		_makepath(_Dir, "", "", _Path, "");

		while (next_local_file_header(z, _Fname))
		{
			if (z->lfh.uncompressed_size)
			{
				char *data = malloc(z->lfh.uncompressed_size);
				if (!data) return 0;

				printf("Decompressing %s\n", _Fname);

				if ( unzip(z, data) < 0 ) 
				{
					free(data);
					return 0;
				}

				WriteFile(z, data);	
				free(data);		
			}
			else {	// folder
				
				_makepath(_Path, "", _Dir, _Fname, "");
				_mkdir(_Path);
			}
		}
		CLOSE();
	}

	return 1;
}



