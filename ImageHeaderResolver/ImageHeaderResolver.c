#if defined _WIN32 || defined _WIN64
#define seek_file(file, offset, pos) _fseeki64(file, offset, pos)
#define tell_file(file) _ftelli64(file)
#define _CRT_SECURE_NO_WARNINGS
#else
#define seek_file(file, offset, pos) fseeko(file, offset, pos)
#define tell_file(file) ftello(file)
#define _FILE_OFFSET_BITS 64
#endif

#include "ImageHeaderResolver.h"
#include "TiffFunctionTemplate.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef enum Endian
{
	IHR_ENDIAN_UNKNOWN,
	IHR_ENDIAN_LITTLE,
	IHR_ENDIAN_BIG
} Endian;

typedef enum Format
{
	IHR_FMT_JPEG,
	IHR_FMT_BMP,
	IHR_FMT_TIFF,
	IHR_FMT_PNG,
	IHR_FMT_TGA,
	IHR_FMT_UNDEF,
	NUM_OF_SUPPORTED_FORMATS = IHR_FMT_UNDEF
} Format;

static inline Endian check_endian(void)
{
	int checker = 1;
	if (*((char *)&checker) == 1)
		return IHR_ENDIAN_LITTLE;
	else
		return IHR_ENDIAN_BIG;
}

static inline void change_endian_16_bit(void *addr)
{
	uint16_t u16 = *(uint16_t *)addr;
	*(uint16_t *)addr = (u16 << 8 & 0xff00) | (u16 >> 8 & 0xff);
}

static inline void change_endian_32_bit(void *addr)
{
	uint32_t u32 = *(uint32_t *)addr;
	*(uint32_t *)addr = 
		(u32 << 24 & 0xff000000) |
	 	(u32 << 8  & 0xff0000) |
		(u32 >> 8  & 0xff00) |
		(u32 >> 24 & 0xff);
}

static inline void change_endian_64_bit(void *addr)
{
	uint64_t u64 = *(uint64_t *)addr;
	*(uint64_t *)addr = 
		(u64 << 56 & 0xff00000000000000) |
	 	(u64 << 48 & 0xff000000000000) |
		(u64 << 40 & 0xff0000000000) |
		(u64 << 32 & 0xff00000000) |
		(u64 >> 32 & 0xff000000) |
		(u64 >> 40 & 0xff0000) |
		(u64 >> 48 & 0xff00) |
		(u64 >> 56 & 0xff);
}

static inline void initialize_image_info(Image_Info *info)
{
	memset(info, 0, sizeof(Image_Info));
}

static inline FILE *load_image_file(
	size_t *file_size_ptr,
	const char *image_path)
{
	FILE *file = fopen(image_path, "rb");

	if (file == NULL)
		perror(image_path);
	else
	{
		seek_file(file, 0, SEEK_END);

		*file_size_ptr = (size_t)tell_file(file);

		seek_file(file, 0, SEEK_SET);
	}

	return file;
}

static inline void terminate(FILE *file)
{
	if (fclose(file) != 0)
		perror("problem occurs when closing image file");
}

static Format resolve_image_format(FILE *file)
{
	seek_file(file, 0, SEEK_SET);

	uint8_t buffer[20];
	if(fread(buffer, 1, 20, file) != 20)
		return IHR_FMT_UNDEF;

	seek_file(file, 0, SEEK_SET);

	if (buffer[0] == 0xff && buffer[1] == 0xd8)
		return IHR_FMT_JPEG;
	else if (buffer[0] == 0x42 && buffer[1] == 0x4d)
		return IHR_FMT_BMP;
	else if ((*((uint16_t*)buffer) == 18761/*II little endian*/ || *((uint16_t*)buffer) == 19789)/*MM big endian*/ &&
	(*((uint16_t*)(buffer + 2)) == 42 || *((uint16_t*)(buffer + 2)) == 10752/*42 normal tif*/ ||
	 *((uint16_t*)(buffer + 2)) == 43 || *((uint16_t*)(buffer + 2)) == 11008)/*43 big tif*/)
		return IHR_FMT_TIFF;
	else if (*((uint64_t *)buffer) == 0x89504e470d0a1a0a || *((uint64_t *)buffer) == 0x0a1a0a0d474e5089)
		return IHR_FMT_PNG;
	else if ((buffer[1] == 0 || buffer[1] == 1) /*color map type*/&& 
	(buffer[2] == 1 || buffer[2] == 2 || buffer[2] == 3 ||
	 buffer[2] == 9 || buffer[2] == 10 || buffer[2] == 11||
	 buffer[2] == 32 || buffer[2] == 33) /*image type*/&& 
	(buffer[16] == 8 || buffer[16] == 15 || buffer[16] == 16 || buffer[16] == 24 || buffer[16] == 32)/*bits per pixel*/)
		return IHR_FMT_TGA;
	else
		return IHR_FMT_UNDEF;
}

static bool resolve_jpeg(
	Image_Info *info,
	FILE *file,
	const Endian sys_endian)
{
	//jpeg file header is always stored as big endian.
	bool is_same_endian = sys_endian == IHR_ENDIAN_BIG;

	//Jump the leading SOI(aka Start Of Image) symbol.
	seek_file(file, 2, SEEK_SET);

	int byte = 0;

	while (feof(file) == false)
	{
		byte = fgetc(file);

		if(byte == EOF)
			break;

		//jpeg symbol mark indicator.
		if(byte != 0xff)
			continue;

		//Skip the possible padding bytes.
		do
		{
			byte = fgetc(file);
		}while(byte == 0xff);
		
		//Jump invalid symbol marks.
		if(byte == 0x00)
			continue;

		//The SOS(aka Start Of Scan) or EOI(aka End Of Image) symbol mark.
		if(byte == 0xda || byte == 0xd9)
			break;

		/*
		SOF(aka Start Of Frame) segment.

		|  Mark  | Name  | Description                              |
		| 0xFFC0 | SOF0  | most used                                |
		| 0xFFC1 | SOF1  | almost never used                        |
		| 0XFFC2 | SOF2  | rarely used                              |
		| 0xFFC3 | SOF3  | almost never used                        |
		| 0xFFC4 | DHT   | not a SOF mark(aka Define Huffman Table) |
		| 0XFFC5 | SOF5  | almost never used                        |
		| 0XFFC6 | SOF6  | almost never used                        |
		| 0XFFC7 | SOF7  | almost never used                        |
		| 0xFFC8 | SOF8  | reserved, almost never used              |
		| 0XFFC9 | SOF9  | almost never used                        |
		| 0XFFCA | SOF10 | almost never used                        |
		| 0xFFCB | SOF11 | almost never used                        |
		| 0xFFCC | SOF12 | reserved, almost never used              |
		| 0xFFCD | SOF13 | almost never used                        |
		| 0xFFCE | SOF14 | almost never used                        |
		| 0xFFCF | SOF15 | almost never used                        |

		A jpeg image file can contain only one top-level SOF segment,
		which stores the basic information of the main image.
		Additional SOF segments may be nested within APP segments to
		define thumbnails.
		So theoretically, we just need to resolve the first top-level
		SOF(0-15, except 4).
		*/
		if((byte & 0xf0) != 0xc0 || byte == 0xc4)
		{
			uint16_t length = 0;

			if(fread(&length, sizeof(uint16_t), 1, file) != 1)
				break;

			if(is_same_endian == false)
				change_endian_16_bit(&length);

			if(seek_file(file, length - 2, SEEK_CUR) != 0)
				break;

			continue;
		}

		uint8_t sof[8];

		if(fread(sof, 1, 8, file) != 8)
			break;

		info->_color_depth = *(uint8_t *)(sof + 2);
		info->_height = *(uint16_t *)(sof + 3);
		info->_width = *(uint16_t *)(sof + 5);
		info->_channels=*(uint8_t *)(sof + 7);

		if(is_same_endian == false)
		{
			change_endian_16_bit(&info->_width);
			change_endian_16_bit(&info->_height);
		}

		//Stop resolving when the first top-level SOF segment is read.
		break;
	}

	info->_color_depth = info->_color_depth * info->_channels;

	return true;
}

static bool resolve_bmp(
	Image_Info *info,
	FILE *file,
	const Endian sys_endian)
{
	//bmp file header is always stored as little endian.
	bool is_same_endian = sys_endian == IHR_ENDIAN_LITTLE;

	//The bmp file header is organized as fixed data and offset.
	seek_file(file, 2, SEEK_SET);

	uint32_t file_size;
	if (fread(&file_size, sizeof(uint32_t), 1, file) != 1)
		return false;

	if(is_same_endian == false)
		change_endian_32_bit(&file_size);

	//If the real file size is smaller the resolved file size, abort resolving.
	if (info->_file_size < file_size)
		return false;

	seek_file(file, 18, SEEK_SET);

	uint8_t header_data[12];
	if (fread(header_data, 1, 12, file) != 12)
		return false;

	info->_width = *(uint32_t *)(header_data);
	info->_height = *(uint32_t *)(header_data + 4);
	info->_color_depth = *(uint16_t *)(header_data + 10);

	if (is_same_endian == false)
	{
		change_endian_32_bit(&info->_width);
		change_endian_32_bit(&info->_height);
		change_endian_16_bit(&info->_color_depth);
	}

	if (info->_color_depth <= 8)
		info->_channels = 1;
	else if (info->_color_depth < 32)
		info->_channels = 3;
	else
		info->_channels = 4;

	return true;
}

//Use macro to create tif resolving functions.
create_tiff_function_instance(normal_tif, 32, 16, 12)

create_tiff_function_instance(big_tif, 64, 64, 20)

static bool resolve_tif(
	Image_Info *info,
	FILE *file, 
	const Endian sys_endian)
{
	seek_file(file, 0, SEEK_SET);

	uint8_t file_header[4];
	if(fread(file_header, 1, 4, file)!=4)
		return false;

	//The resolved order of tif file.
	Endian file_endian = file_header[0] == 0x49 && file_header[1] == 0x49 ? IHR_ENDIAN_LITTLE : IHR_ENDIAN_BIG;
	bool is_same_endian = sys_endian == file_endian;

    bool is_big_tif = file_header[2] == 0x2b || file_header[3] == 0x2b;

	if(is_big_tif == true)
	{
		//Skip the 0x00000008
		if(seek_file(file, 4, SEEK_CUR) != 0)
			return false;

		uint64_t first_ifd_pos = 0;

		if(fread(&first_ifd_pos, sizeof(uint64_t), 1, file) != 1)
			return false;

		if(is_same_endian == false)
			change_endian_64_bit(&first_ifd_pos);

		//Locate to first image directory.
		if(seek_file(file, (int64_t)first_ifd_pos, SEEK_SET) != 0)
			return false;

		return resolve_big_tif(info, file, is_same_endian);
	}
	else
	{
		uint32_t first_ifd_pos = 0;

		if(fread(&first_ifd_pos, sizeof(uint32_t), 1, file) != 1)
			return false;

		if(is_same_endian == false)
			change_endian_32_bit(&first_ifd_pos);

		//Locate to first image directory.
		if(seek_file(file, (int64_t)first_ifd_pos, SEEK_SET) != 0)
            return false;

        return resolve_normal_tif(info, file, is_same_endian);
    }
}

static bool resolve_png(
	Image_Info *info,
	FILE *file,
	const Endian sys_endian)
{
	//png file header is always stored as big endian.
	bool is_same_endian = sys_endian == IHR_ENDIAN_BIG;

	//Skip the fixed leading bytes.
	seek_file(file, 8, SEEK_SET);

	//The IHDR(aka Image Header) section data buffer.
	uint8_t IHDR[25];

	if(fread(IHDR, 1, 25, file) != 25)
		return false;

	//The first 4 byte of IHDR must be number 13.
	uint32_t data_number = *(uint32_t *)IHDR;
	if(is_same_endian == false)
		change_endian_32_bit(&data_number);
	if(data_number != 13)
		return false;

	//The following 4 bytes must be 0x49484452(aka "IHDR").
	uint8_t section_name[4] = {'I', 'H', 'D', 'R'};
	if(memcmp(IHDR + 4, section_name, 4) != 0)
		return false;

	//Image width is stored in the next 4 byte.
	info->_width = *(uint32_t *)(IHDR + 8);

	//Image height is stored after image width another 4 bytes.
	info->_height = *(uint32_t *)(IHDR + 12);

	//Color bit depth takes 1 byte after image height.
	info->_color_depth = *(IHDR + 16);

	//At last the color type in 1 byte.
	switch(*(IHDR + 17))
	{
		case 0x00:
			info->_channels = 1;
			break;
		case 0x02:
		case 0x03:
			info->_channels = 3;
			break;
		case 0x04:
			info->_channels = 2;
			break;
		case 0x06:
			info->_channels = 4;
			break;
		default:
			break;
	}

	if(is_same_endian == false)
	{
		change_endian_32_bit(&info->_width);
		change_endian_32_bit(&info->_height);
	}

	info->_color_depth = info->_color_depth * info->_channels;

	return true;
}

static bool resolve_tga(
	Image_Info *info,
	FILE *file,
	const Endian sys_endian)
{
	bool is_same_endian = sys_endian == IHR_ENDIAN_LITTLE;

	uint8_t header[20];

	if(fread(header, 1, 20, file) != 20)
		return false;

	uint8_t color_depth = header[16];

	uint8_t alpha_bits = header[17] & 0x0f;

	bool valid_data = true;

	switch(color_depth)
	{
		case 8://grey scale
			if(alpha_bits != 0/*ordinary format*/ &&
			   alpha_bits != 8/*special format, just an alpha mask*/)
				valid_data = false;
			else 
				info->_channels = 1;
		break;
		case 15://5-5-5-1 BGRA
			if(alpha_bits != 1)
				valid_data = false;
			else
			 	info->_channels = 4;
		break;
		case 16://5-5-5-1 BGRA
			if(alpha_bits != 1)
				valid_data = false;
			else
				info->_channels = 4;
		break;
		case 24://8-8-8 BGR
			if(alpha_bits != 0)
				valid_data = false;
			else
			 	info->_channels = 3;
		break;
		case 32://8-8-8-8 BGRA
			if(alpha_bits != 0 && alpha_bits != 8)
				valid_data = false;
			else
			 	info->_channels = 4;
		break;
		default:
		break;
	}

	if(valid_data == false)
		return false;

	info->_color_depth = color_depth;

	uint16_t width = *(uint16_t *)(header + 12);
	if(is_same_endian == false)
		change_endian_16_bit(&width);

	info->_width = width;

	uint16_t height = *(uint16_t *)(header + 14);
	if(is_same_endian == false)
		change_endian_16_bit(&height);

	info->_height = height;

	return true;
}

/**
* The definition of the enum Format makes sure that the value of NUM_OF_SUPPORTED_FORMATS
* represents the number of defined formats, and the value of each format label is just the
* corresponding index in the _resolve_func_array.
*/
static bool(*_resolve_func_array[NUM_OF_SUPPORTED_FORMATS])(Image_Info *, FILE *, const Endian) =
{
	&resolve_jpeg,
	&resolve_bmp,
	&resolve_tif,
	&resolve_png,
	&resolve_tga
};

static inline void convert_format_string(const Format format, char *string)
{
	switch(format)
	{
		case IHR_FMT_JPEG	:
			strcpy(string, "jpeg");
		break;
		case IHR_FMT_BMP	:
			strcpy(string, "bmp");
		break;
		case IHR_FMT_TIFF	:
			strcpy(string, "tiff");
		break;
		case IHR_FMT_PNG	:
			strcpy(string, "png");
		break;
		case IHR_FMT_TGA	:
			strcpy(string, "tga");
		break;
		default:
		break;
	}
}

static inline bool resolve_image_file(
	FILE *file,
	Image_Info *image_info)
{
	Format image_format = IHR_FMT_UNDEF;
	if ((image_format = resolve_image_format(file)) == IHR_FMT_UNDEF)
		return false;

	Endian sys_endian = check_endian();

	bool success = _resolve_func_array[image_format](image_info, file, sys_endian);

	if(success == false)
	{
		release_image_info(image_info);

		initialize_image_info(image_info);

		return false;
	}
	else
	{
		//Set up the shared data.
		uint64_t file_size = image_info->_file_size;

		uint32_t page_number = 0;

		char format_string[8];

		convert_format_string(image_format, format_string);

		Image_Info *walker = image_info;
		do
		{
			++page_number;

			walker->_file_size = file_size;

			strcpy(walker->_format, format_string);

			walker = walker->_next;
		} while(walker != NULL);

		walker = image_info;
		do
		{
			walker->_page_number = page_number;

			walker = walker->_next;
		} while(walker != NULL);
	}

	if(is_image_info_valid(image_info) == false)
	{
		release_image_info(image_info);

		initialize_image_info(image_info);

		return false;
	}

	return true;
}

bool get_image_info(
	const char *img_path,
	Image_Info *image_info)
{
	if(image_info == NULL)
		return false;

	initialize_image_info(image_info);

	FILE *file = load_image_file(&image_info->_file_size, img_path);

	if (file == NULL)
		return false;

	if(image_info->_file_size == 0ULL)
	{
		terminate(file);
		
		return false;
	}
	
	//We do not want to close the file in every return point of the entry
	//function, so we put the resolving operations into another function.
	bool success = resolve_image_file(file, image_info);

	terminate(file);

	return success;
}

bool is_image_info_valid(const Image_Info *info)
{
	return info->_width > 0 && info->_height > 0 &&
		info->_channels > 0 && info->_color_depth > 0 &&
		info->_file_size > 0ULL && strlen(info->_format) != 0ULL;
}

void release_image_info(Image_Info *info)
{
	if (info->_next == NULL)
		return;

	Image_Info *walker = info->_next;
	while (walker != NULL)
	{
		Image_Info *next = walker->_next;

		free(walker);

		walker = next;
	}
}