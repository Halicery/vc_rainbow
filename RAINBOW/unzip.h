enum ENUMZIP 
{
	ENUMZIP_OK = 100,
	ENUMZIPERROR_UNKNOWNCOMPRESSION = -999,
	ENUMZIPERROR_INFLATEERROR,
};

struct LFH {	// local_file_header 
	unsigned int local_file_header_signature;	// = 0x04034b50
	unsigned short version_needed_to_extract;
	unsigned short general_purpose_bit_flag;
	unsigned short compression_method;
	unsigned short last_mod_file_time;
	unsigned short last_mod_file_date;
	unsigned int crc32;
	unsigned int compressed_size;
	unsigned int uncompressed_size;
	unsigned short file_name_length;
	unsigned short extra_field_length;
	// n bytes of file name
	// m bytes of extra 
	// This is immediately followed by the compressed data
};

struct ZIP {
	struct LFH lfh;
	struct INFLATE Inflate;		// my INFLATE object
};

extern int next_local_file_header(struct ZIP *z, char *FileNameToCopy);	// reads to next header 
extern enum ENUMZIP unzip(struct ZIP *z, unsigned char *dst);			// unzip to destination buffer 
