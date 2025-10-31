#ifndef TIFFFUNCTIONTEMPLATE_H
#define TIFFFUNCTIONTEMPLATE_H

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

#define color_depth_accumulate(type, content_buffer, count, color_depth, is_same_endian)\
	for (uint64_t i = 0; i != (uint64_t)(count); ++i)\
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

#define create_tiff_function_instance(TIFF_TYPE, DATA_LENGTH, EC_LENGTH, DE_LENGTH)\
static inline uint##DATA_LENGTH##_t convert_de_content_##TIFF_TYPE(\
	uint16_t data_type,\
	uint8_t *content,\
	bool is_same_endian)\
{\
	uint##DATA_LENGTH##_t converted = 0;\
\
	union\
	{\
		uint16_t 	ui_16;\
		uint32_t 	ui_32;\
        uint64_t    ui_64;\
		int16_t 	i_16;\
		int32_t 	i_32;\
		int64_t 	i_64;\
	} data;\
\
	switch (data_type) \
	{\
		case IHR_DE_TYPE_BYTE:\
			converted = (uint##DATA_LENGTH##_t)*content;\
		break;\
		case IHR_DE_TYPE_SHORT:\
			data.ui_16 = *(uint16_t *)(content);\
\
			if(is_same_endian == false)\
				change_endian_16_bit(&data.ui_16);\
\
			converted = (uint##DATA_LENGTH##_t)data.ui_16;\
		break;\
		case IHR_DE_TYPE_LONG:\
			data.ui_32 = *(uint32_t *)(content);\
\
			if(is_same_endian == false)\
				change_endian_16_bit(&data.ui_32);\
\
			converted = (uint##DATA_LENGTH##_t)data.ui_32;\
		break;\
		case IHR_DE_TYPE_LONG8:\
			data.ui_64 = *(uint64_t *)(content);\
\
			if(is_same_endian == false)\
				change_endian_64_bit(&data.ui_64);\
\
            converted = (uint##DATA_LENGTH##_t)data.ui_64;\
		break;\
		case IHR_DE_TYPE_SBYTE:\
			converted = (uint##DATA_LENGTH##_t)*(int8_t *)(content);\
		break;\
		case IHR_DE_TYPE_SSHORT:\
			data.i_16 = *(int16_t *)(content);\
\
			if(is_same_endian == false)\
				change_endian_16_bit(&data.i_16);\
\
			converted = (uint##DATA_LENGTH##_t)data.i_16;\
		break;\
		case IHR_DE_TYPE_SLONG:\
			data.i_32 = *(int32_t *)(content);\
\
			if(is_same_endian == false)\
				change_endian_32_bit(&data.i_32);\
\
			converted = (uint##DATA_LENGTH##_t)data.i_32;\
		break;\
		case IHR_DE_TYPE_SLONG8:\
			data.i_64 = *(int64_t *)(content);\
\
			if(is_same_endian == false)\
				change_endian_64_bit(&data.i_64);\
\
			converted = (uint##DATA_LENGTH##_t)data.i_64;\
		break;\
		default:\
		break;\
	}\
\
	return converted;\
}\
\
static inline bool read_de_content_##TIFF_TYPE(\
	uint##DATA_LENGTH##_t pos,\
	FILE *file,\
	uint8_t *content_ptr,\
	uint##DATA_LENGTH##_t content_size)\
{\
	int64_t current = tell_file(file);\
\
	seek_file(file, (int64_t)pos, SEEK_SET);\
\
	bool success = fread(content_ptr, content_size, 1, file) == 1;\
\
	seek_file(file, current, SEEK_SET);\
\
	return success;\
}\
\
static inline uint##DATA_LENGTH##_t evaluate_de_content_size_##TIFF_TYPE(\
	uint16_t data_type,\
	uint##DATA_LENGTH##_t count)\
{\
	uint##DATA_LENGTH##_t size = count;\
\
	switch(data_type)\
	{\
		case IHR_DE_TYPE_SHORT 	:\
		case IHR_DE_TYPE_SSHORT	:\
			size <<= 1;\
		break;\
\
		case IHR_DE_TYPE_LONG 	:\
		case IHR_DE_TYPE_SLONG 	:\
			size <<= 2;\
		break;\
\
		case IHR_DE_TYPE_LONG8 	: \
		case IHR_DE_TYPE_SLONG8 :\
			size <<= 3;\
		break;\
\
		case IHR_DE_TYPE_BYTE 	:\
		case IHR_DE_TYPE_SBYTE 	:\
		default:\
		break;\
	}\
\
	return size;\
}\
\
static inline void resolve_bits_per_sample_##TIFF_TYPE(\
	Image_Info *info,\
	uint16_t data_type,\
	uint##DATA_LENGTH##_t count,\
	uint8_t *content_buffer,\
	bool is_same_endian)\
{\
	bool valid_value = true;\
\
	uint16_t color_depth = 0;\
\
	switch (data_type) \
	{\
		case IHR_DE_TYPE_BYTE:\
			color_depth_accumulate(uint8_t, content_buffer, count, color_depth, is_same_endian)\
		break;\
		case IHR_DE_TYPE_SHORT:\
			color_depth_accumulate(uint16_t, content_buffer, count, color_depth, is_same_endian)\
		break;\
		case IHR_DE_TYPE_LONG:\
			color_depth_accumulate(uint32_t, content_buffer, count, color_depth, is_same_endian)\
		break;\
		case IHR_DE_TYPE_LONG8:\
			color_depth_accumulate(uint64_t, content_buffer, count, color_depth, is_same_endian)\
		break;\
		case IHR_DE_TYPE_SBYTE:\
			color_depth_accumulate(int8_t, content_buffer, count, color_depth, is_same_endian)\
		break;\
		case IHR_DE_TYPE_SSHORT:\
			color_depth_accumulate(int16_t, content_buffer, count, color_depth, is_same_endian)\
		break;\
		case IHR_DE_TYPE_SLONG:\
			color_depth_accumulate(int32_t, content_buffer, count, color_depth, is_same_endian)\
		break;\
		case IHR_DE_TYPE_SLONG8:\
			color_depth_accumulate(int64_t, content_buffer, count, color_depth, is_same_endian)\
		break;\
		default:\
		break;\
	}\
\
	info->_color_depth = color_depth;\
}\
\
static inline bool resolve_de_content_buffer_##TIFF_TYPE(\
	Image_Info *info,\
	uint16_t tag,\
	uint16_t data_type,\
	uint##DATA_LENGTH##_t count,\
	uint8_t *content_ptr,\
	bool is_same_endian,\
	bool *is_single_page)\
{\
	bool valid_content = true;\
\
	uint##DATA_LENGTH##_t de_value = 0;\
\
	switch(tag)\
	{\
		case IHR_TIF_TAG_NEW_SUBFILE_TYPE :\
			de_value = convert_de_content_##TIFF_TYPE(data_type, content_ptr, is_same_endian);\
\
			*is_single_page = !(de_value & (IHR_TIF_NST_REDUCED_IMAGE |\
				 IHR_TIF_NST_TRANSPARENT_MASK | IHR_TIF_NST_DEPTH_MAP));\
		break;\
		case IHR_TIF_TAG_SUBFILE_TYPE :\
			de_value = convert_de_content_##TIFF_TYPE(data_type, content_ptr, is_same_endian);\
\
			*is_single_page = de_value != IHR_TIF_ST_FILETYPE_REDUCED_IMAGE;\
		break;\
		case IHR_TIF_TAG_IMAGE_WIDTH :\
			de_value = convert_de_content_##TIFF_TYPE(data_type, content_ptr, is_same_endian);\
\
			info->_width = (uint32_t)de_value;\
		break;\
		case IHR_TIF_TAG_IMAGE_HEIGHT :\
			de_value = convert_de_content_##TIFF_TYPE(data_type, content_ptr, is_same_endian);\
\
			info->_height = (uint32_t)de_value;\
		break;\
		case IHR_TIF_TAG_BITS_PER_SAMPLE :\
			if(info->_channels != 0 && info->_channels != count)\
			{\
				/*We encountered an inconsistency in tif file.*/\
				printf("error in tif : the IHR_TIF_TAG_SAMPLES_PER_PIXEL value conflicts "\
					   "with the IHR_TIF_TAG_BITS_PER_SAMPLE value.");\
\
				valid_content = false;\
			}\
			else\
			{\
				resolve_bits_per_sample_##TIFF_TYPE(info, data_type, count, content_ptr, is_same_endian);\
\
				info->_channels = (uint16_t)count;\
			}\
		break;\
		case IHR_TIF_TAG_SAMPLES_PER_PIXEL :\
			de_value = convert_de_content_##TIFF_TYPE(data_type, content_ptr, is_same_endian);\
\
			if(info->_channels != 0 && info->_channels != de_value)\
			{\
				/*We encountered an inconsistency in tif file.*/\
				printf("error in tif : the IHR_TIF_TAG_SAMPLES_PER_PIXEL value conflicts "\
					   "with the IHR_TIF_TAG_BITS_PER_SAMPLE value.");\
\
				valid_content = false;\
			}\
			else\
				info->_channels = (uint16_t)de_value;\
		break;\
		default:\
		break;\
	}\
\
	return valid_content;\
}\
\
static bool resolve_##TIFF_TYPE(\
	Image_Info *info,\
	FILE *file,\
	bool is_same_endian)\
{\
	/*The data of a directory entry, 12 bytes for normal tif, 20 bytes for big tif.*/\
	uint16_t *tag = NULL;					/*offset:0*/\
	uint16_t *data_type = NULL;				/*offset:2*/\
	uint##DATA_LENGTH##_t *count = NULL;	/*offset:4*/\
	uint8_t *content = NULL;				/*offset:8 for normal tif, 12 for big tif*/\
\
	uint##DATA_LENGTH##_t entry_list_buffer_prev_size = 0;\
	uint8_t *entry_list_buffer = NULL;\
\
	uint##DATA_LENGTH##_t content_buffer_prev_size = 0;\
	uint8_t *content_buffer = NULL;\
\
	uint##DATA_LENGTH##_t next_ifd_pos = 0;\
\
	/*Pointer of currently resolved image page.*/\
	Image_Info *current_page_ptr = NULL;\
\
	bool success = true;\
\
	/*Resolve a single image file directory.*/\
	do\
	{\
		uint##EC_LENGTH##_t entry_count = 0;\
\
		if(fread(&entry_count, sizeof(uint##EC_LENGTH##_t), 1, file) != 1)\
		{\
			success = false;\
\
			break;\
		}\
\
		if (is_same_endian == false)\
			change_endian_##EC_LENGTH##_bit(&entry_count);\
\
		/*Every directory entry is 12 bytes for normal tif, 20 bytes for big tif.*/\
		uint##DATA_LENGTH##_t current_buffer_size = DE_LENGTH * entry_count;\
\
		/*Previous buffer is not allocated or is not big enough to store current entry lists.*/\
		if(entry_list_buffer == NULL || current_buffer_size > entry_list_buffer_prev_size)\
		{\
			free(entry_list_buffer);\
\
			entry_list_buffer = (uint8_t *)malloc(current_buffer_size);\
\
			entry_list_buffer_prev_size = current_buffer_size;\
		}\
		\
		/*Memory allocation is failed.*/\
		if(entry_list_buffer == NULL)\
		{\
			perror("");\
\
			success = false;\
\
			break;\
		}\
\
		if(fread(entry_list_buffer, current_buffer_size, 1, file) != 1)\
		{\
			success = false;\
\
			break;\
		}\
\
		/*Current page resolved.*/\
		Image_Info current_page;\
		initialize_image_info(&current_page);\
\
		uint8_t *buffer_cpy = entry_list_buffer;\
\
		bool is_single_page = true;\
\
		bool entry_resolve_success = true;\
\
		for(uint##EC_LENGTH##_t i = 0; i != entry_count; ++i, buffer_cpy += DE_LENGTH)\
		{\
			/*Bind a directory entry.*/\
			tag = (uint16_t *)buffer_cpy;\
			data_type = (uint16_t *)(buffer_cpy + 2);\
			count = (uint##DATA_LENGTH##_t *)(buffer_cpy + 4);\
			content = (uint8_t *)(buffer_cpy + 4 + sizeof(uint##DATA_LENGTH##_t));\
\
			if(is_same_endian == false)\
				change_endian_16_bit(tag);\
\
			/*Our program only concerns about tags from IHR_TIF_TAG_NEW_SUBFILE_TYPE*/\
            /*to IHR_TIF_TAG_BITS_PER_SAMPLE and IHR_TIF_TAG_SAMPLES_PER_PIXEL.*/\
			if(*tag < IHR_TIF_TAG_NEW_SUBFILE_TYPE || *tag > IHR_TIF_TAG_BITS_PER_SAMPLE &&\
			   *tag != IHR_TIF_TAG_SAMPLES_PER_PIXEL)\
				continue;\
\
			if(is_same_endian == false)\
			{\
				change_endian_16_bit(data_type);\
				change_endian_##DATA_LENGTH##_bit(count);\
			}\
\
			uint##DATA_LENGTH##_t content_size = evaluate_de_content_size_##TIFF_TYPE(*data_type, *count);\
\
			uint8_t *content_ptr = NULL;\
\
			if(content_size <= sizeof(uint##DATA_LENGTH##_t))/*The value can be stored in content.*/\
				content_ptr = content;\
			else/*The value is stored otherwhere, content is just an offset.*/\
			{\
				/*We need to allocate memory for content value.*/\
				if(content_buffer == NULL || content_buffer_prev_size < content_size)\
				{\
					free(content_buffer);\
\
					content_buffer = (uint8_t *)malloc(content_size);\
\
					if(content_buffer == NULL)\
					{\
						perror("");/*Run out of memory.*/\
\
						entry_resolve_success = false;\
\
						break;\
					}\
\
					content_buffer_prev_size = content_size;\
				}\
\
				content_ptr = content_buffer;\
\
				uint##DATA_LENGTH##_t content_real_pos = convert_de_content_##TIFF_TYPE(*data_type, content, is_same_endian);\
\
				/*Read content from file stream.*/\
				if(read_de_content_##TIFF_TYPE(content_real_pos, file, content_ptr, content_size) == false)\
				{\
					entry_resolve_success = false;\
\
					break;\
				}\
			}\
\
			if(resolve_de_content_buffer_##TIFF_TYPE(&current_page, *tag, *data_type, *count,\
				content_ptr, is_same_endian, &is_single_page) == false)\
			{\
				entry_resolve_success = false;\
\
				break;\
			}\
		}\
\
		if(entry_resolve_success == false)\
		{\
			success = false;\
\
			break;\
		}\
\
		/*An ifd is resolved, check result.*/\
		if(is_single_page == true && (\
		   	current_page._width == 0 ||\
		    current_page._height == 0 ||\
			current_page._color_depth == 0 ||\
			current_page._channels == 0))\
		{\
			/*Should be a valid image page, but the result is invalid, abort resolving.*/\
			success = false;\
\
			break;\
		}\
\
		if(current_page_ptr == NULL)\
		{\
			/*The first page.*/\
			info->_width = current_page._width;\
			info->_height = current_page._height;\
			info->_color_depth = current_page._color_depth;\
			info->_channels = current_page._channels;\
\
			current_page_ptr = info;\
		}\
		else\
		{\
			/*The following pages.*/\
			Image_Info *image_free_memory_ptr = (Image_Info *)malloc(sizeof(Image_Info));\
\
			if(image_free_memory_ptr == NULL)\
			{\
				perror("can not allocate memory for image page.");\
\
				success = false;\
\
				break;\
			}\
\
			*image_free_memory_ptr = current_page;\
\
			current_page_ptr->_next = image_free_memory_ptr;\
\
			current_page_ptr = current_page_ptr->_next;\
		}\
\
		if(fread(&next_ifd_pos, sizeof(uint##DATA_LENGTH##_t), 1, file) != 1)\
		{\
			success = false;\
\
			break;\
		}\
\
		if(is_same_endian == false)\
			change_endian_##DATA_LENGTH##_bit(&next_ifd_pos);\
\
		if(seek_file(file, (int64_t)next_ifd_pos, SEEK_SET) != 0)\
		{\
			success = false;\
\
			break;\
		}\
	/*If the next ifd position is 0 or an invalid value(locate out of file),*/\
	/*stop traversing the ifd list.*/\
	} while(next_ifd_pos != 0 && next_ifd_pos < info->_file_size);\
\
	/*Deallocate the buffers last used.*/\
	if(entry_list_buffer != NULL)\
		free(entry_list_buffer);\
\
	if(content_buffer != NULL)\
		free(content_buffer);\
\
	return success;\
}

#endif