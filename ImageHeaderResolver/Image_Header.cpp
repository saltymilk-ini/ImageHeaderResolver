#include "Image_Header.hpp"
#include "ImageHeaderResolver.h"

class Image_Header::Image_Header_Impl
{
public:
    Image_Header_Impl(Image_Info info) : _start_page(info), _current_page(info) {}
    
    ~Image_Header_Impl() { release_image_info(&_start_page); }

    std::size_t file_size() const { return _start_page._file_size; }

    std::string format() const { return _start_page._format; }

    unsigned int width() const { return _current_page._width; }

    unsigned int height() const { return _current_page._height; }

    unsigned int color_depth() const { return _current_page._color_depth; }

    unsigned int channels() const { return _current_page._channels; }

    unsigned int page_number() const { return _start_page._page_number; }

    bool next_page() 
    {
        if(_current_page._next == nullptr)
            return false;

        _current_page = *_current_page._next;

        return true;
    }

    void reset_page() { _current_page = _start_page;}

private:
    Image_Info _start_page;
    Image_Info _current_page;
};

std::size_t Image_Header::file_size() const
{
    return _pimpl->file_size();
}

const std::string Image_Header::format() const
{
    return _pimpl->format();
}

unsigned int Image_Header::width() const
{
    return _pimpl->width();
}

unsigned int Image_Header::height() const
{
    return _pimpl->height();
}

unsigned int Image_Header::color_depth() const
{
    return _pimpl->color_depth();
}

unsigned int Image_Header::channels() const
{
    return _pimpl->channels();
}

unsigned int Image_Header::page_number() const
{
    return _pimpl->page_number();
}

bool Image_Header::next_page()
{
    return _pimpl->next_page();
}

void Image_Header::reset_page()
{
    _pimpl->reset_page();
}

std::shared_ptr<Image_Header> Image_Header::read_image(const std::string &img_path)
{
    Image_Info info;
    
    if(get_image_info(img_path.c_str(), &info) == false)
        return nullptr;

    std::shared_ptr<Image_Header> ret(new Image_Header);

    ret->_pimpl.reset(new Image_Header_Impl(info));

    return ret;
}
