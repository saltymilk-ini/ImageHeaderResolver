#pragma once

#include <string>
#include <memory>

class Image_Header
{
public:
	static std::shared_ptr<Image_Header> read_image(const std::string &file_path);

	/**@brief file size(in byte) 图片文件大小（以字节计）*/
	std::size_t file_size() const;

	/**@brief image format(the format resolved) 图片格式（以实际解析的格式为准）*/
	const std::string format() const;

	/**@brief image width(in pixel) 图片宽度（以像素计）*/
	unsigned int width() const;

	/**@brief image height(in pixel) 图片高度（以像素计）*/
	unsigned int height() const;

	/**@brief number of bits each pixel takes 图片位深度（单个像素占用的位数）*/
	unsigned int color_depth() const;

	/**@brief number of channels 通道数 */
	unsigned int channels() const;

	/**
	* @brief number of pages 分页数量
	* @attention only when the picture format is tif, can page number be greater than 1
	* 仅当图片文件为tif格式时，分页数量才可能大于1
	*/
	unsigned int page_number() const;

	/**
	* @brief switch to next page(for multiple paged tif only) 切换至下一页（针对多页tif文件）
	* @return return true if switched to a valid page 如果切换至有效页则返回true
	*/
	bool next_page();

	/**@brief return to the first page(for multiple paged tif only) 返回第一页（针对多页tif文件）*/
	void reset_page();

private:
	Image_Header() = default;

private:
	class Image_Header_Impl;

	std::unique_ptr<Image_Header_Impl> _pimpl;
};