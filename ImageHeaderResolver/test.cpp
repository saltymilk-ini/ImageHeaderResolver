#include "ImageHeader.hpp"
#include <iostream>
#include <string>
#include <chrono>

void printInfo(const Image_Header *info, long long span)
{
	std::cout << 
		"format      :" << info->format() << '\n' <<
		"file size   :" << info->file_size() << '\n' <<
		"width       :" << info->width() << '\n' <<
		"height      :" << info->height() << '\n' <<
		"color depth :" << info->color_depth() << '\n' <<
		"channels    :" << info->channels() << '\n' <<
		"time cost   :" << span << "us" << std::endl;
}

int main(int argc, char *argv[])
{
	if (argc == 2)
	{
		auto start = std::chrono::high_resolution_clock::now();

		std::shared_ptr<Image_Header> info = Image_Header::read_image(argv[1]);

		if (info == nullptr)
		{
			std::cout << "file \'" << argv[1] << "\' : failed to resolve.\n";

			return 1;
		}	

		auto end = std::chrono::high_resolution_clock::now();

		auto span = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

		printInfo(info.get(), span.count());
	}
	else if (argc == 1)
	{
		std::string console_input;

		while (1)
		{
			std::cout << "input file path to resolve, or q to quit.\n";

			std::cin >> console_input;

			if (console_input == "q")
				break;

			auto start = std::chrono::high_resolution_clock::now();

			std::shared_ptr<Image_Header> info = Image_Header::read_image(console_input);

			if (info == nullptr)
			{
				std::cout << "file \'" << console_input << "\' : failed to resolve.\n";

				continue;
			}

			auto end = std::chrono::high_resolution_clock::now();

			auto span = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

			printInfo(info.get(), span.count());
		}
	}

	return 0;
}