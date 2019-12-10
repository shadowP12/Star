#pragma once
#include "CLCore.h"

RC_NAMESPACE_BEGIN
// GPUÈÝÆ÷
template <typename T>
class GPUVector
{
public:
	GPUVector(const cl::Context& context, const cl::CommandQueue& queue, cl_mem_flags flags)
		:mContext(context), mQueue(queue), mFlags(flags)
	{
		mSize = 0;
		mCapacity = 16;
		cl_int error = CL_SUCCESS;
		mBuffer = cl::Buffer(mContext, mFlags, sizeof(T) * mCapacity, nullptr, &error);
		if (error != CL_SUCCESS) throw std::runtime_error("cannot allocate opencl buffer!");
	}

	~GPUVector()
	{
	}
	const cl::Buffer& getBuffer() const
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

			cl::Buffer newBuffer = cl::Buffer(mContext, mFlags, sizeof(T) * mCapacity, nullptr, &error);
			if (error != CL_SUCCESS) throw std::runtime_error("cannot allocate opencl buffer!");

			if (mSize != 0)
			{
				error = mQueue.enqueueCopyBuffer(mBuffer, newBuffer, 0, 0, sizeof(T) * mSize);
				if (error != CL_SUCCESS) throw std::runtime_error("cannot copy opencl buffer!");
			}

			mBuffer = std::move(newBuffer);
		}
	}
	void append(const T* elements, int count)
	{
		reserve(mSize + count);

		cl_int error = CL_SUCCESS;
		error = mQueue.enqueueWriteBuffer(mBuffer, CL_TRUE, sizeof(T) * mSize, sizeof(T) * count, elements);
		if (error != CL_SUCCESS) throw std::runtime_error("cannot write opencl buffer!");

		mSize += count;
	}
private:
	cl::Context mContext;
	cl::CommandQueue mQueue;
	cl::Buffer mBuffer;
	cl_mem_flags mFlags;
	int mCapacity;
	int mSize;
};

RC_NAMESPACE_END
