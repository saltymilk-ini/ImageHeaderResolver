#ifndef IMAGEHEADERRESOLVER_H
#define IMAGEHEADERRESOLVER_H

#include <stdint.h>
#include <stdbool.h>

/**
* For now, this program supports resolving of *JPEG*, *BMP*, *TIF*, *PNG*, *TGA* image formats. 
*/

#ifdef __cplusplus
extern "C"
{
#endif

//Image header information
typedef struct Image_Header_Info
{
    uint64_t    _file_size;                     //file size(in byte)
    char        _format[8];                     //image format(the format resolved)
    uint32_t    _width;                         //image width(in pixel)
    uint32_t    _height;                        //image height(in pixel)
    uint16_t    _color_depth;                   //number of bits each pixel takes
    uint16_t    _channels;                      //number of channels

    /**
    * If the image file is multiple paged tiff file, this points to the next page of image
    * information, otherwise, this will always be set to NULL.
    * Multiple paged tiff image information is organized like the form of a link list, you
    * can go through all the nodes till the end page which holds a NULL '_next' pointer.
    * When a multiple paged tiff file is resolved, you need to release the Image_Info you get
    * after you don't need it anymore to avoid a memory leakage, cause in this specific
    * circumstance the free storage is used.
    */
    uint32_t                   _page_number;    //number of tif pages
    struct Image_Header_Info   *_next;
} Image_Info;

/**
* @brief Get the image information of the given image file.
* @param[in] img_path the file path of the image file
* @param[out] image_info pointer of memory to hold the resolved data
* @return true for success, false for failure
*/
bool get_image_info(const char *img_path, Image_Info *image_info);

/**
* @brief Check if the given image information is valid.
* @param[in] info image information to check
* @return true for valid, false for invalid 
*/
bool is_image_info_valid(const Image_Info *info);

/**
* @brief Only needed to be called when the image file is multiple paged tiff file.
* @attention For safety reason, call it when you don't need an image_info anymore.
*/
void release_image_info(Image_Info *info);

#ifdef __cplusplus
}
#endif

#endif