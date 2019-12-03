#include "DataCL.h"

RAY_CL_NAMESPACE_BEGIN
/*
template<typename T>
void GPUVector<T>::reserve(int reqCap)
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

template<typename T>
void GPUVector<T>::append(const T* elements, int count)
{
	reserve(mSize + count);

	cl_int error = CL_SUCCESS;
	error = mQueue.enqueueWriteBuffer(mBuffer, CL_TRUE, sizeof(T) * mSize, sizeof(T) * count, elements);
	if (error != CL_SUCCESS) throw std::runtime_error("cannot write opencl buffer!");

	mSize += count;
}
*/
RAY_CL_NAMESPACE_END