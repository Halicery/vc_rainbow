/***********************************************************************************

	A simple usage of the unzip object: 

		while ( next_local_file_header() )
		{
			unzip();
		}

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

#include "GETC.h" 
#include "inflate.h"
#include "unzip.h"


extern int next_local_file_header(struct ZIP *z, char *FileNameToCopy)
{
	if ( GETDli() != 0x04034b50 ) return 0;

	z->lfh.version_needed_to_extract= GETWli();
	z->lfh.general_purpose_bit_flag= GETWli();
	z->lfh.compression_method= GETWli();
	SEEK(4);	// last mod
	z->lfh.crc32= GETDli();
	z->lfh.compressed_size= GETDli();
	z->lfh.uncompressed_size= GETDli();
	z->lfh.file_name_length= GETWli();
	z->lfh.extra_field_length= GETWli();

	{
		int i;
		for ( i=0; i < z->lfh.file_name_length; i++) FileNameToCopy[i]= GETC();
		FileNameToCopy[i]= 0;	// to C-string

		SEEK(z->lfh.extra_field_length);
	}

	return 1;
}


extern enum ENUMZIP unzip(struct ZIP *z, unsigned char *dst)	// unzip to destination
{
	if ( 8 == z->lfh.compression_method )	// 8: Deflate
	{
		if ( inflate(&z->Inflate, dst, (void*)GETC,0) < 0 ) 
			return ENUMZIPERROR_INFLATEERROR;
	}
	else if ( 0 == z->lfh.compression_method )	// uncompressed 
	{
		unsigned int i, n;
		n= z->lfh.uncompressed_size >> 2;
		for (i=0; i < n; i++) ((int*)dst)[i]= GETDli();	
		for (i = z->lfh.uncompressed_size & ~3; i < z->lfh.uncompressed_size; i++) dst[i]= GETC();						
	}
	else return ENUMZIPERROR_UNKNOWNCOMPRESSION;

	return ENUMZIP_OK;
}

