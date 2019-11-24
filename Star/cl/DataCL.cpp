#include "DataCL.h"

RAY_CL_NAMESPACE_BEGIN

template<typename T>
GPUMemPool<T>::GPUMemPool()
{
}

template<typename T>
GPUMemPool<T>::~GPUMemPool()
{
}

template<typename T>
inline void GPUMemPool<T>::reserve(size_t reqCap)
{
}
RAY_CL_NAMESPACE_END