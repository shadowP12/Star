#pragma once

#include <vector>
#include <string>

struct ShaderDesc
{
    int type;// 0:vert 1:frag 2:compute
    std::string source;
    std::string includeDir;
};

std::vector<uint32_t> compileShader(const ShaderDesc& desc);