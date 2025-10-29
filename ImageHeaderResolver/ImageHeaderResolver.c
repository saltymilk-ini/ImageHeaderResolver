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

//Directory entry data type(only define data types our program concerns).
#define IHR_DE_TYPE_BYTE 							1	//unsigned 8-bit integer
#define IHR_DE_TYPE_SHORT 							3	//unsigned 16-bit integer
#define IHR_DE_TYPE_LONG 							4	//unsigned 32-bit integer
#define IHR_DE_TYPE_SBYTE 							6	//signed 8-bit integer
#define IHR_DE_TYPE_SSHORT 							8	//signed 16-bit integer
#define IHR_DE_TYPE_SLONG 							9	//signed 32-bit integer
#define IHR_DE_TYPE_LONG8 							16	//unsigned 64-bit integer
#define IHR_DE_TYPE_SLONG8 							17	//signed 64-bit integer

//Tiff tag reference(only baseline tags).
#define	IHR_TIF_TAG_NO_REFERENCE 					0x0000
#define	IHR_TIF_TAG_NEW_SUBFILE_TYPE 				0x00fe
#define	IHR_TIF_TAG_SUBFILE_TYPE 					0x00ff
#define	IHR_TIF_TAG_IMAGE_WIDTH 					0x0100
#define	IHR_TIF_TAG_IMAGE_HEIGHT 					0x0101
#define	IHR_TIF_TAG_BITS_PER_SAMPLE 				0x0102
#define	IHR_TIF_TAG_COMPRESSION 					0x0103
#define	IHR_TIF_TAG_PHOTO_METRIC_INTERPRETATION 	0x0106
#define	IHR_TIF_TAG_THRESHOLDING 					0x0107
#define	IHR_TIF_TAG_CELL_WIDTH 						0x0108
#define	IHR_TIF_TAG_CELL_HEIGHT 					0x0109
#define	IHR_TIF_TAG_FILL_ORDER 						0x010a
#define	IHR_TIF_TAG_IMAGE_DESCRIPTION 				0x010e
#define	IHR_TIF_TAG_SCANNER_MAKER 					0x010f
#define	IHR_TIF_TAG_SCANNER_MODEL 					0x0110
#define	IHR_TIF_TAG_STRIP_OFFSET 					0x0111
#define	IHR_TIF_TAG_ORIENTATION 					0x0112
#define	IHR_TIF_TAG_SAMPLES_PER_PIXEL 				0x0115
#define	IHR_TIF_TAG_ROWS_PER_STRIP 					0x0116
#define	IHR_TIF_TAG_STRIP_BYTE_COUNT 				0x0117
#define	IHR_TIF_TAG_MIN_SAMPLE_VALUE 				0x0118
#define	IHR_TIF_TAG_MAX_SAMPLE_VALUE 				0x0119
#define	IHR_TIF_TAG_XRESOLUTION 					0x011a
#define	IHR_TIF_TAG_YRESOLUTION 					0x011b
#define	IHR_TIF_TAG_PLANAR_CONFIGURATION 			0x011c
#define	IHR_TIF_TAG_FREE_OFFSET 					0x0120
#define	IHR_TIF_TAG_FREE_BYTE_COUNT 				0x0121
#define	IHR_TIF_TAG_GRAY_RESPONSE_UNIT 				0x0122
#define	IHR_TIF_TAG_GRAY_RESPONSE_CURVE 			0x0123
#define	IHR_TIF_TAG_RESOLUTION_UNIT 				0x0128
#define	IHR_TIF_TAG_SOFTWARE 						0x0131
#define	IHR_TIF_TAG_DATE_TIME						0x0132
#define	IHR_TIF_TAG_ARTIST 							0x013b
#define	IHR_TIF_TAG_HOST_COMPUTER 					0x013c
#define	IHR_TIF_TAG_COLOR_MAP 						0x0140
#define	IHR_TIF_TAG_EXTRA_SAMPLES 					0x0152
#define	IHR_TIF_TAG_COPYRIGHT 						0x8298

//New subfile type of image file directory that uses bit mask.
#define IHR_TIF_NST_DEFAULT 						0x00
#define IHR_TIF_NST_REDUCED_IMAGE 					0x01
#define IHR_TIF_NST_SINGLE_PAGE 					0x02
#define IHR_TIF_NST_TRANSPARENT_MASK 				0x04
#define IHR_TIF_NST_DEPTH_MAP 						0x08

//Subfile type of image directory that uses value(this field is deprecated).
#define IHR_TIF_ST_FILETYPE_IMAGE 					1
#define IHR_TIF_ST_FILETYPE_REDUCED_IMAGE 			2
#define IHR_TIF_ST_FILETYPE_SINGLE_PAGE 			3

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
	 buffer[2] == 9 || buffer[2] == 10 || buffer[2] == 11) /*image type*/&& 
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

		//The EOI(aka End Of Image) symbol mark.
		if(byte == 0xd9)
			break;

		//The symbol is not SOF0, we just read it's length and skip it.
		if(byte != 0xc0)
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

		//Read SOF0(aka Start Of Frame 0) symbol.
		//In most circumstances, the information of the first SOF0 symbol is
		//what we need.
		//But a jepg image file may contain several SOF0-SOF3 data, some of
		//then store the thumbnail information.
		//So, for more accuracy, we should go through all SOF0-SOF3 symbols
		//in a jepg image file, then select the biggest width, height, color
		//depth and channels as the final result.
		//For now, we just read the first SOF0, later optimization is needed.
		//Todo: read all SOF0-SOF3 data.
		uint8_t sof0[8];

		if(fread(sof0, 1, 8, file) != 8)
			break;

		info->_color_depth = *(uint8_t *)(sof0 + 2);
		info->_height = *(uint16_t *)(sof0 + 3);
		info->_width = *(uint16_t *)(sof0 + 5);
		info->_channels=*(uint8_t *)(sof0 + 7);

		if(is_same_endian == false)
		{
			change_endian_16_bit(&info->_width);
			change_endian_16_bit(&info->_height);
		}

		//Stop resolving when the first SOF0 is read.
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

static inline uint64_t convert_de_content_big_tif(
	uint16_t data_type,
	uint8_t *content,
	bool is_same_endian)
{
	uint64_t converted = 0;

	union
	{
		uint16_t 	ui_16;
		uint32_t 	ui_32;
		int16_t 	i_16;
		int32_t 	i_32;
		int64_t 	i_64;
	} data;

	switch (data_type) 
	{
		case IHR_DE_TYPE_BYTE:
			converted = *content;
		break;
		case IHR_DE_TYPE_SHORT:
			data.ui_16 = *(uint16_t *)(content);

			if(is_same_endian == false)
				change_endian_16_bit(&data.ui_16);

			converted = data.ui_16;
		break;
		case IHR_DE_TYPE_LONG:
			data.ui_32 = *(uint32_t *)(content);

			if(is_same_endian == false)
				change_endian_16_bit(&data.ui_32);

			converted = data.ui_32;
		break;
		case IHR_DE_TYPE_LONG8:
			converted = *(uint64_t *)(content);

			if(is_same_endian == false)
				change_endian_64_bit(&converted);
		break;
		case IHR_DE_TYPE_SBYTE:
			converted = (uint64_t)*(int8_t *)(content);
		break;
		case IHR_DE_TYPE_SSHORT:
			data.i_16 = *(int16_t *)(content);

			if(is_same_endian == false)
				change_endian_16_bit(&data.i_16);

			converted = (uint64_t)data.i_16;
		break;
		case IHR_DE_TYPE_SLONG:
			data.i_32 = *(int32_t *)(content);

			if(is_same_endian == false)
				change_endian_32_bit(&data.i_32);

			converted = (uint64_t)data.i_32;
		break;
		case IHR_DE_TYPE_SLONG8:
			data.i_64 = *(int64_t *)(content);

			if(is_same_endian == false)
				change_endian_64_bit(&data.i_64);

			converted = (uint64_t)data.i_64;
		break;
		default:
		break;
	}

	return converted;
}

static inline bool read_de_content_big_tif(
	uint64_t pos,
	FILE *file,
	uint8_t *content_ptr,
	uint64_t content_size)
{
	int64_t current = tell_file(file);

	seek_file(file, (int64_t)pos, SEEK_SET);

	bool success = fread(content_ptr, content_size, 1, file) == 1;

	seek_file(file, current, SEEK_SET);

	return success;
}

static inline uint64_t evaluate_de_content_size_big_tif(
	uint16_t data_type,
	uint64_t count)
{
	uint64_t size = count;

	switch(data_type)
	{
		case IHR_DE_TYPE_SHORT 	:
		case IHR_DE_TYPE_SSHORT	:
			size <<= 1;
		break;

		case IHR_DE_TYPE_LONG 	:
		case IHR_DE_TYPE_SLONG 	:
			size <<= 2;
		break;

		case IHR_DE_TYPE_LONG8 	: 
		case IHR_DE_TYPE_SLONG8 :
			size <<= 3;
		break;

		case IHR_DE_TYPE_BYTE 	:
		case IHR_DE_TYPE_SBYTE 	:
		default:
		break;
	}

	return size;
}

#define color_depth_accumulate_big_tif(type, content_buffer, count, color_depth, is_same_endian)\
	for (uint64_t i = 0; i != (count); ++i)\
	{\
		type value = ((type *)content_buffer)[i];\
		if(is_same_endian == false)\
		{\
			if(sizeof(type) == 2)\
				change_endian_16_bit(&value);\
			else if(sizeof(type) == 4)\
				change_endian_32_bit(&value);\
			else if(sizeof(type) == 8)\
				change_endian_64_bit(&value);\
		}\
		color_depth += (uint16_t)value;\
	}

static inline void resolve_bits_per_sample_big_tif(
	Image_Info *info,
	uint16_t data_type,
	uint64_t count,
	uint8_t *content_buffer,
	bool is_same_endian)
{
	bool valid_value = true;

	uint16_t color_depth = 0;

	switch (data_type) 
	{
		case IHR_DE_TYPE_BYTE:
			color_depth_accumulate_big_tif(uint8_t, content_buffer, count, color_depth, is_same_endian)
		break;
		case IHR_DE_TYPE_SHORT:
			color_depth_accumulate_big_tif(uint16_t, content_buffer, count, color_depth, is_same_endian)
		break;
		case IHR_DE_TYPE_LONG:
			color_depth_accumulate_big_tif(uint32_t, content_buffer, count, color_depth, is_same_endian)
		break;
		case IHR_DE_TYPE_LONG8:
			color_depth_accumulate_big_tif(uint64_t, content_buffer, count, color_depth, is_same_endian)
		break;
		case IHR_DE_TYPE_SBYTE:
			color_depth_accumulate_big_tif(int8_t, content_buffer, count, color_depth, is_same_endian)
		break;
		case IHR_DE_TYPE_SSHORT:
			color_depth_accumulate_big_tif(int16_t, content_buffer, count, color_depth, is_same_endian)
		break;
		case IHR_DE_TYPE_SLONG:
			color_depth_accumulate_big_tif(int32_t, content_buffer, count, color_depth, is_same_endian)
		break;
		case IHR_DE_TYPE_SLONG8:
			color_depth_accumulate_big_tif(int64_t, content_buffer, count, color_depth, is_same_endian)
		break;
		default:
		break;
	}

	info->_color_depth = color_depth;
}

static inline bool resolve_de_content_buffer_big_tif(
	Image_Info *info,
	uint16_t tag,
	uint16_t data_type,
	uint64_t count,
	uint8_t *content_ptr,
	bool is_same_endian,
	bool *is_single_page)
{
	bool valid_content = true;

	uint64_t de_value = 0;

	switch(tag)
	{
		case IHR_TIF_TAG_NEW_SUBFILE_TYPE :
			de_value = convert_de_content_big_tif(data_type, content_ptr, is_same_endian);

			*is_single_page = !(de_value & (IHR_TIF_NST_REDUCED_IMAGE | IHR_TIF_NST_TRANSPARENT_MASK | IHR_TIF_NST_DEPTH_MAP));
		break;
		case IHR_TIF_TAG_SUBFILE_TYPE :
			de_value = convert_de_content_big_tif(data_type, content_ptr, is_same_endian);

			*is_single_page = de_value != IHR_TIF_ST_FILETYPE_REDUCED_IMAGE;
		break;
		case IHR_TIF_TAG_IMAGE_WIDTH :
			de_value = convert_de_content_big_tif(data_type, content_ptr, is_same_endian);

			info->_width = (uint32_t)de_value;
		break;
		case IHR_TIF_TAG_IMAGE_HEIGHT :
			de_value = convert_de_content_big_tif(data_type, content_ptr, is_same_endian);

			info->_height = (uint32_t)de_value;
		break;
		case IHR_TIF_TAG_BITS_PER_SAMPLE :
			if(info->_channels != 0 && info->_channels != count)
			{
				//We encountered an inconsistency in tif file.
				printf("error in tif : the IHR_TIF_TAG_SAMPLES_PER_PIXEL value conflicts with the IHR_TIF_TAG_BITS_PER_SAMPLE value.");

				valid_content = false;
			}
			else
			{
				resolve_bits_per_sample_big_tif(info, data_type, count, content_ptr, is_same_endian);

				info->_channels = (uint16_t)count;
			}
		break;
		case IHR_TIF_TAG_SAMPLES_PER_PIXEL :
			de_value = convert_de_content_big_tif(data_type, content_ptr, is_same_endian);

			if(info->_channels != 0 && info->_channels != de_value)
			{
				//We encountered an inconsistency in tif file.
				printf("error in tif : the IHR_TIF_TAG_SAMPLES_PER_PIXEL value conflicts with the IHR_TIF_TAG_BITS_PER_SAMPLE value.");

				valid_content = false;
			}
			else
				info->_channels = (uint16_t)de_value;
		break;
		default:
		break;
	}

	return valid_content;
}

static bool resolve_big_tif(
	Image_Info *info,
	FILE *file,
	bool is_same_endian)
{
	//The data of a directory entry, 20 bytes.
	uint16_t *tag = NULL;		//offset:0
	uint16_t *data_type = NULL;	//offset:2
	uint64_t *count = NULL;		//offset:4
	uint8_t *content = NULL;	//offset:12

	uint64_t entry_list_buffer_prev_size = 0;
	uint8_t *entry_list_buffer = NULL;

	uint64_t content_buffer_prev_size = 0;
	uint8_t *content_buffer = NULL;

	uint64_t next_ifd_pos = 0;

	//Pointer of currently resolved image page.
	Image_Info *current_page_ptr = NULL;

	bool success = true;

	//Resolve a single image file directory.
	do
	{
		uint64_t entry_count = 0;

		if(fread(&entry_count, 8, 1, file) != 1)
		{
			success = false;

			break;
		}

		if (is_same_endian == false)
			change_endian_64_bit(&entry_count);

		//Every directory entry of big tif file is 20 byte.
		uint64_t current_buffer_size = 20 * entry_count;

		//Previous buffer is not allocated or is not big enough to store current entry lists.
		if(entry_list_buffer == NULL || current_buffer_size > entry_list_buffer_prev_size)
		{
			free(entry_list_buffer);

			entry_list_buffer = (uint8_t *)malloc(current_buffer_size);

			entry_list_buffer_prev_size = current_buffer_size;
		}
		
		//Memory allocation is failed.
		if(entry_list_buffer == NULL)
		{
			perror("");

			success = false;

			break;
		}

		if(fread(entry_list_buffer, current_buffer_size, 1, file) != 1)
		{
			success = false;

			break;
		}

		//Current page resolved.
		Image_Info current_page;
		initialize_image_info(&current_page);

		uint8_t *buffer_cpy = entry_list_buffer;

		bool is_single_page = true;

		bool entry_resolve_success = true;

		for(uint64_t i = 0; i != entry_count; ++i, buffer_cpy += 20)
		{
			//Bind a directory entry.
			tag = (uint16_t *)buffer_cpy;
			data_type = (uint16_t *)(buffer_cpy + 2);
			count = (uint64_t *)(buffer_cpy + 4);
			content = (uint8_t *)(buffer_cpy + 12);

			if(is_same_endian == false)
				change_endian_16_bit(tag);

			//Our program only concerns about tags from IHR_TIF_TAG_NEW_SUBFILE_TYPE to IHR_TIF_TAG_BITS_PER_SAMPLE and IHR_TIF_TAG_SAMPLES_PER_PIXEL.
			if(*tag < IHR_TIF_TAG_NEW_SUBFILE_TYPE || *tag > IHR_TIF_TAG_BITS_PER_SAMPLE && *tag != IHR_TIF_TAG_SAMPLES_PER_PIXEL)
				continue;

			if(is_same_endian == false)
			{
				change_endian_16_bit(data_type);
				change_endian_64_bit(count);
			}

			uint64_t content_size = evaluate_de_content_size_big_tif(*data_type, *count);

			uint8_t *content_ptr = NULL;

			if(content_size <= 8)//The value can be stored in content.
				content_ptr = content;
			else//The value is stored otherwhere, content is just an offset.
			{
				//We need to allocate memory for content value.
				if(content_buffer == NULL || content_buffer_prev_size < content_size)
				{
					free(content_buffer);

					content_buffer = (uint8_t *)malloc(content_size);

					if(content_buffer == NULL)
					{
						perror("");//Run out of memory.

						entry_resolve_success = false;

						break;
					}

					content_buffer_prev_size = content_size;
				}

				content_ptr = content_buffer;

				uint64_t content_real_pos = convert_de_content_big_tif(*data_type, content, is_same_endian);

				//Read content from file stream.
				if(read_de_content_big_tif(content_real_pos, file, content_ptr, content_size) == false)
				{
					entry_resolve_success = false;

					break;
				}
			}

			if(resolve_de_content_buffer_big_tif(&current_page, *tag, *data_type, *count,
				content_ptr, is_same_endian, &is_single_page) == false)
			{
				entry_resolve_success = false;

				break;
			}
		}

		if(entry_resolve_success == false)
		{
			success = false;

			break;
		}

		//An ifd is resolved, check result.
		if(is_single_page == true && (
		   	current_page._width == 0 ||
		    current_page._height == 0 ||
			current_page._color_depth == 0 ||
			current_page._channels == 0))
		{
			//Should be a valid image page, but the result is invalid, abort resolving.
			success = false;

			break;
		}

		if(current_page_ptr == NULL)
		{
			//The first page.
			info->_width = current_page._width;
			info->_height = current_page._height;
			info->_color_depth = current_page._color_depth;
			info->_channels = current_page._channels;

			current_page_ptr = info;
		}
		else
		{
			//The following pages.
			Image_Info *image_free_memory_ptr = (Image_Info *)malloc(sizeof(Image_Info));

			if(image_free_memory_ptr == NULL)
			{
				perror("can not allocate memory for image page.");

				success = false;

				break;
			}

			*image_free_memory_ptr = current_page;

			current_page_ptr->_next = image_free_memory_ptr;

			current_page_ptr = current_page_ptr->_next;
		}

		if(fread(&next_ifd_pos, sizeof(uint64_t), 1, file) != 1)
		{
			success = false;

			break;
		}

		if(is_same_endian == false)
			change_endian_64_bit(&next_ifd_pos);

		if(seek_file(file, (int64_t)next_ifd_pos, SEEK_SET) != 0)
		{
			success = false;

			break;
		}
	//If the next ifd position is 0 or an invalid value(locate out of file),
	//stop traversing the ifd list.
	} while(next_ifd_pos != 0 && next_ifd_pos < info->_file_size);

	//Deallocate the buffers last used.
	if(entry_list_buffer != NULL)
		free(entry_list_buffer);

	if(content_buffer != NULL)
		free(content_buffer);

	return success;
}

static inline uint32_t convert_de_content_normal_tif(
	uint16_t data_type,
	uint8_t *content,
	bool is_same_endian)
{
	uint32_t converted = 0;

	union 
	{
		uint16_t 	ui_16;
		int16_t		i_16;
		int32_t		i_32;
	} data;

	switch (data_type) 
	{
		case IHR_DE_TYPE_BYTE:
			converted = *content;
		break;
		case IHR_DE_TYPE_SHORT:
			data.ui_16 = *(uint16_t *)(content);

			if(is_same_endian == false)
				change_endian_16_bit(&data.ui_16);

			converted = data.ui_16;
		break;
		case IHR_DE_TYPE_LONG:
			converted = *(uint32_t *)(content);

			if(is_same_endian == false)
				change_endian_16_bit(&converted);
		break;
		case IHR_DE_TYPE_SBYTE:
			converted = (uint32_t)*(int8_t *)(content);
		break;
		case IHR_DE_TYPE_SSHORT:
			data.i_16 = *(int16_t *)(content);

			if(is_same_endian == false)
				change_endian_16_bit(&data.i_16);

			converted = (uint32_t)data.i_16;
		break;
		case IHR_DE_TYPE_SLONG:
			data.i_32 = *(int32_t *)(content);

			if(is_same_endian == false)
				change_endian_32_bit(&data.i_32);

			converted = (uint32_t)data.i_32;
		break;
		default:
		break;
	}

	return converted;
}

static inline bool read_de_content_normal_tif(
	uint32_t pos,
	FILE *file,
	uint8_t *content_ptr,
	uint32_t content_size)
{
	int64_t current = tell_file(file);

	seek_file(file, (int64_t)pos, SEEK_SET);

	bool success = fread(content_ptr, content_size, 1, file) == 1;

	seek_file(file, current, SEEK_SET);

	return success;
}

static inline uint32_t evaluate_de_content_size_normal_tif(
	uint16_t data_type,
	uint32_t count)
{
	uint32_t size = count;

	switch(data_type)
	{
		case IHR_DE_TYPE_SHORT 	:
		case IHR_DE_TYPE_SSHORT	:
			size <<= 1;
		break;

		case IHR_DE_TYPE_LONG 	:
		case IHR_DE_TYPE_SLONG 	:
			size <<= 2;
		break;

		case IHR_DE_TYPE_LONG8 	: 
		case IHR_DE_TYPE_SLONG8 :
			size <<= 3;
		break;

		case IHR_DE_TYPE_BYTE 	:
		case IHR_DE_TYPE_SBYTE 	:
		default:
		break;
	}

	return size;
}

#define color_depth_accumulate_normal_tif(type, content_buffer, count, color_depth, is_same_endian)\
	for (uint32_t i = 0; i != (count); ++i)\
	{\
		type value = ((type *)content_buffer)[i];\
		if(is_same_endian == false)\
		{\
			if(sizeof(type) == 2)\
				change_endian_16_bit(&value);\
			else if(sizeof(type) == 4)\
				change_endian_32_bit(&value);\
			else if(sizeof(type) == 8)\
				change_endian_64_bit(&value);\
		}\
		color_depth += (uint16_t)value;\
	}
		

static inline void resolve_bits_per_sample_normal_tif(
	Image_Info *info,
	uint16_t data_type,
	uint32_t count,
	uint8_t *content_buffer,
	bool is_same_endian)
{
	uint16_t color_depth = 0;

	switch (data_type) 
	{
		case IHR_DE_TYPE_BYTE:
			color_depth_accumulate_normal_tif(uint8_t, content_buffer, count, color_depth, is_same_endian)
		break;
		case IHR_DE_TYPE_SHORT:
			color_depth_accumulate_normal_tif(uint16_t, content_buffer, count, color_depth, is_same_endian)
		break;
		case IHR_DE_TYPE_LONG:
			color_depth_accumulate_normal_tif(uint32_t, content_buffer, count, color_depth, is_same_endian)
		break;
		case IHR_DE_TYPE_SBYTE:
			color_depth_accumulate_normal_tif(int8_t, content_buffer, count, color_depth, is_same_endian)
		break;
		case IHR_DE_TYPE_SSHORT:
			color_depth_accumulate_normal_tif(int16_t, content_buffer, count, color_depth, is_same_endian)
		break;
		case IHR_DE_TYPE_SLONG:
			color_depth_accumulate_normal_tif(int32_t, content_buffer, count, color_depth, is_same_endian)
		break;
		default:
		break;
	}

	info->_color_depth = color_depth;
}

static inline bool resolve_de_content_buffer_normal_tif(
	Image_Info *info,
	uint16_t tag,
	uint16_t data_type,
	uint32_t count,
	uint8_t *content_ptr,
	bool is_same_endian,
	bool *is_single_page)
{
	bool valid_content = true;

	uint32_t de_value;

	switch(tag)
	{
		case IHR_TIF_TAG_NEW_SUBFILE_TYPE :
			de_value = convert_de_content_normal_tif(data_type, content_ptr, is_same_endian);

			*is_single_page = !(de_value & (IHR_TIF_NST_REDUCED_IMAGE | IHR_TIF_NST_TRANSPARENT_MASK | IHR_TIF_NST_DEPTH_MAP));
		break;
		case IHR_TIF_TAG_SUBFILE_TYPE :
			de_value = convert_de_content_normal_tif(data_type, content_ptr, is_same_endian);

			*is_single_page = de_value != IHR_TIF_ST_FILETYPE_REDUCED_IMAGE;
		break;
		case IHR_TIF_TAG_IMAGE_WIDTH :
			de_value = convert_de_content_normal_tif(data_type, content_ptr, is_same_endian);

			info->_width = de_value;
		break;
		case IHR_TIF_TAG_IMAGE_HEIGHT :
			de_value = convert_de_content_normal_tif(data_type, content_ptr, is_same_endian);

			info->_height = de_value;
		break;
		case IHR_TIF_TAG_BITS_PER_SAMPLE :
			if(info->_channels != 0 && info->_channels != count)
			{
				//We encountered an inconsistency in tif file.
				printf("error in tif : the IHR_TIF_TAG_SAMPLES_PER_PIXEL value conflicts with the IHR_TIF_TAG_BITS_PER_SAMPLE value.");

				valid_content = false;
			}
			else
			{
				resolve_bits_per_sample_normal_tif(info, data_type, count, content_ptr, is_same_endian);

				info->_channels = (uint16_t)count;
			}
		break;
		case IHR_TIF_TAG_SAMPLES_PER_PIXEL :
			de_value = convert_de_content_normal_tif(data_type, content_ptr, is_same_endian);

			if(info->_channels != 0 && info->_channels != de_value)
			{
				//We encountered an inconsistency in tif file.
				printf("error in tif : the IHR_TIF_TAG_SAMPLES_PER_PIXEL value conflicts with the IHR_TIF_TAG_BITS_PER_SAMPLE value.");

				valid_content = false;
			}
			else
				info->_channels = (uint16_t)de_value;
		break;
		default:
		break;
	}

	return valid_content;
}

static bool resolve_normal_tif(
	Image_Info *info,
	FILE *file,
	bool is_same_endian)
{
	//The data of a directory entry, 12 bytes.
	uint16_t *tag = NULL;		//offset:0
	uint16_t *data_type = NULL;	//offset:2
	uint32_t *count = NULL;		//offset:4
	uint8_t *content = NULL;	//offset:8

	uint32_t entry_list_buffer_prev_size = 0;
	uint8_t *entry_list_buffer = NULL;

	uint32_t content_buffer_prev_size = 0;
	uint8_t *content_buffer = NULL;

	uint32_t next_ifd_pos = 0;

	//Pointer of currently resolved image page.
	Image_Info *current_page_ptr = NULL;

	bool success = true;

	//Resolve a single image file directory.
	do
	{
		uint16_t entry_count = 0;

		if(fread(&entry_count, sizeof(uint16_t), 1, file) != 1)
		{
			success = false;

			break;
		}

		if (is_same_endian == false)
			change_endian_32_bit(&entry_count);

		//Every directory entry of normal tif file is 12 byte.
		uint32_t current_buffer_size = 12 * entry_count;

		//Previous buffer is not allocated or is not big enough to store current entry lists.
		if(entry_list_buffer == NULL || current_buffer_size > entry_list_buffer_prev_size)
		{
			free(entry_list_buffer);

			entry_list_buffer = (uint8_t *)malloc(current_buffer_size);

			entry_list_buffer_prev_size = current_buffer_size;
		}
		
		//Memory allocation is failed.
		if(entry_list_buffer == NULL)
		{
			perror("");

			success = false;

			break;
		}

		if(fread(entry_list_buffer, current_buffer_size, 1, file) != 1)
		{
			success = false;

			break;
		}

		//Current page resolved.
		Image_Info current_page;
		initialize_image_info(&current_page);

		uint8_t *buffer_cpy = entry_list_buffer;

		bool is_single_page = true;

		bool entry_resolve_success = true;

		for(uint16_t i = 0; i != entry_count; ++i, buffer_cpy += 12)
		{
			//Bind a directory entry.
			tag = (uint16_t *)buffer_cpy;
			data_type = (uint16_t *)(buffer_cpy + 2);
			count = (uint32_t *)(buffer_cpy + 4);
			content = buffer_cpy + 8;

			if(is_same_endian == false)
				change_endian_16_bit(tag);

			//Our program only concerns about tags from IHR_TIF_TAG_NEW_SUBFILE_TYPE to IHR_TIF_TAG_BITS_PER_SAMPLE and IHR_TIF_TAG_SAMPLES_PER_PIXEL.
			if(*tag < IHR_TIF_TAG_NEW_SUBFILE_TYPE || *tag > IHR_TIF_TAG_BITS_PER_SAMPLE && *tag != IHR_TIF_TAG_SAMPLES_PER_PIXEL)
				continue;

			if(is_same_endian == false)
			{
				change_endian_16_bit(data_type);
				change_endian_32_bit(count);
			}

			uint32_t content_size = evaluate_de_content_size_normal_tif(*data_type, *count);

			uint8_t *content_ptr = NULL;

			if(content_size <= 4)//The value can be stored in content.
				content_ptr = content;
			else//The value is stored otherwhere, content is just an offset.
			{
				//We need to allocate memory for content value.
				if(content_buffer == NULL || content_buffer_prev_size < content_size)
				{
					free(content_buffer);

					content_buffer = (uint8_t *)malloc(content_size);

					if(content_buffer==NULL)
					{
						perror("");//Run out of memory.

						entry_resolve_success = false;

						break;
					}

					content_buffer_prev_size = content_size;
				}

				content_ptr = content_buffer;

				uint32_t content_real_pos = convert_de_content_normal_tif(*data_type, content, is_same_endian);

				//Read content from file stream.
				if(read_de_content_normal_tif(content_real_pos, file, content_ptr, content_size) == false)
				{
					entry_resolve_success = false;

					break;
				}
			}

			if(resolve_de_content_buffer_normal_tif(&current_page, *tag, *data_type, *count,
				content_ptr, is_same_endian, &is_single_page) == false)
			{
				entry_resolve_success = false;

				break;
			}
		}

		if(entry_resolve_success == false)
		{
			success = false;

			break;
		}

		//An ifd is resolved, check result.
		if(is_single_page == true && (
		   	current_page._width == 0 ||
		    current_page._height == 0 ||
			current_page._color_depth == 0 ||
			current_page._channels == 0))
		{
			//Should be a valid image page, but the result is invalid, abort resolving.
			success = false;

			break;
		}

		if(current_page_ptr == NULL)
		{
			//The first page.
			info->_width = current_page._width;
			info->_height = current_page._height;
			info->_color_depth = current_page._color_depth;
			info->_channels = current_page._channels;

			current_page_ptr = info;
		}
		else
		{
			//The following pages.
			Image_Info *image_free_memory_ptr = (Image_Info *)malloc(sizeof(Image_Info));

			if(image_free_memory_ptr == NULL)
			{
				perror("can not allocate memory for image page.");

				success = false;

				break;
			}

			*image_free_memory_ptr = current_page;

			current_page_ptr->_next = image_free_memory_ptr;

			current_page_ptr = current_page_ptr->_next;
		}

		if(fread(&next_ifd_pos, sizeof(uint32_t), 1, file) != 1)
		{
			success = false;

			break;
		}

		if(is_same_endian == false)
			change_endian_32_bit(&next_ifd_pos);

		if(seek_file(file, (int64_t)next_ifd_pos, SEEK_SET) != 0)
		{
			success = false;

			break;
		}
	//If the next ifd position is 0 or an invalid value(locate out of file),
	//stop traversing the ifd list.
	} while(next_ifd_pos != 0 && next_ifd_pos < info->_file_size);

	//Deallocate the buffers last used.
	if(entry_list_buffer != NULL)
		free(entry_list_buffer);

	if(content_buffer != NULL)
		free(content_buffer);

	return success;
}

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
			if(alpha_bits != 0)
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
