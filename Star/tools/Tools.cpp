#include "Tools.h"
#include <direct.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>

bool readFileData(std::string& file_path, std::string& out_data)
{
	std::istream* stream = &std::cin;
	std::ifstream input_file;

	input_file.open(file_path, std::ios_base::binary);
	stream = &input_file;
	if (input_file.fail())
	{
		printf("cannot open input file %s \n", file_path.c_str());
		return false;
	}
	out_data = std::string((std::istreambuf_iterator<char>(*stream)), std::istreambuf_iterator<char>());
	return true;
}