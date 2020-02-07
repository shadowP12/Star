#pragma once
//#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>

#define RC_NAMESPACE_BEGIN namespace rc {
#define RC_NAMESPACE_END };

RC_NAMESPACE_BEGIN

struct CLCore
{
	cl::Platform platform;
	cl::Device device;
	cl::Context context;
	cl::CommandQueue queue;
};

RC_NAMESPACE_END