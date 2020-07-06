#pragma once
#include <string>

std::string getCurrentPath();

std::string getFilePath(const std::string& str);

bool readFileData(const std::string& file_path, std::string& out_data);