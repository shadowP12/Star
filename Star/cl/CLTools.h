#pragma once
#include "CLCore.h"

RC_NAMESPACE_BEGIN
// GPUÈÝÆ÷
template <typename T>
class GPUVector
{
public:
	GPUVector(CLCore* core, cl_mem_flags flags = CL_MEM_READ_ONLY)
		:mCore(core), mFlags(flags)
	{
		mSize = 0;
		mCapacity = 16;
		cl_int error = CL_SUCCESS;
		mBuffer = cl::Buffer(mCore->context, mFlags, sizeof(T) * mCapacity, nullptr, &error);
		if (error != CL_SUCCESS) throw std::runtime_error("cannot allocate opencl buffer!");
	}

	~GPUVector()
	{
	}
	const cl::Buffer getBuffer() const
	{
		return mBuffer;
	}
	int getSize() const
	{
		return mSize;
	}
	void pushBack(const T& e)
	{
		append(&e, 1);
	}
	void clear()
	{
		mSize = 0;
	}
private:
	void reserve(int reqCap)
	{
		if (mCapacity < reqCap)
		{
			cl_int error = CL_SUCCESS;

			while (mCapacity < reqCap) mCapacity *= 2;

			cl::Buffer newBuffer = cl::Buffer(mCore->context, mFlags, sizeof(T) * mCapacity, nullptr, &error);
			if (error != CL_SUCCESS) throw std::runtime_error("cannot allocate opencl buffer!");

			if (mSize != 0)
			{
				error = mCore->queue.enqueueCopyBuffer(mBuffer, newBuffer, 0, 0, sizeof(T) * mSize);
				if (error != CL_SUCCESS) throw std::runtime_error("cannot copy opencl buffer!");
			}

			mBuffer = std::move(newBuffer);
		}
	}
	void append(const T* elements, int count)
	{
		reserve(mSize + count);

		cl_int error = CL_SUCCESS;
		error = mCore->queue.enqueueWriteBuffer(mBuffer, CL_TRUE, sizeof(T) * mSize, sizeof(T) * count, elements);
		if (error != CL_SUCCESS) throw std::runtime_error("cannot write opencl buffer!");

		mSize += count;
	}
private:
	CLCore* mCore;
	cl::Buffer mBuffer;
	cl_mem_flags mFlags;
	int mCapacity;
	int mSize;
};

RC_NAMESPACE_END
