// SPDX-FileCopyrightText: 2017-2023 The Ginkgo authors
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef GKO_HIP_COMPONENTS_MEMORY_HIP_HPP_
#define GKO_HIP_COMPONENTS_MEMORY_HIP_HPP_


#include <cstring>
#include <type_traits>


#include <ginkgo/core/base/math.hpp>


#include "hip/base/types.hip.hpp"


namespace gko {
namespace kernels {
namespace hip {


#if GINKGO_HIP_PLATFORM_NVCC


#include "common/cuda_hip/components/memory.nvidia.hpp.inc"


#else


template <typename T>
struct gcc_atomic_intrinsic_type_map {};


template <>
struct gcc_atomic_intrinsic_type_map<int32> {
    using type = int32;
};


template <>
struct gcc_atomic_intrinsic_type_map<float> {
    using type = int32;
};


template <>
struct gcc_atomic_intrinsic_type_map<int64> {
    using type = int64;
};


template <>
struct gcc_atomic_intrinsic_type_map<double> {
    using type = int64;
};


template <int memorder, int scope, typename ValueType>
__device__ __forceinline__ ValueType load_generic(const ValueType* ptr)
{
    using atomic_type = typename gcc_atomic_intrinsic_type_map<ValueType>::type;
    static_assert(sizeof(atomic_type) == sizeof(ValueType), "invalid map");
    static_assert(alignof(atomic_type) == sizeof(ValueType), "invalid map");
    auto cast_value = __hip_atomic_load(
        reinterpret_cast<const atomic_type*>(ptr), memorder, scope);
    ValueType result{};
    memcpy(&result, &cast_value, sizeof(ValueType));
    return result;
}


template <int memorder, int scope, typename ValueType>
__device__ __forceinline__ void store_generic(ValueType* ptr, ValueType value)
{
    using atomic_type = typename gcc_atomic_intrinsic_type_map<ValueType>::type;
    static_assert(sizeof(atomic_type) == sizeof(ValueType), "invalid map");
    static_assert(alignof(atomic_type) == sizeof(ValueType), "invalid map");
    atomic_type cast_value{};
    memcpy(&cast_value, &value, sizeof(ValueType));
    return __hip_atomic_store(reinterpret_cast<atomic_type*>(ptr), cast_value,
                              memorder, scope);
}


template <typename ValueType,
          gcc_atomic_intrinsic_type_map<ValueType>* = nullptr>
__device__ __forceinline__ ValueType load_relaxed(const ValueType* ptr)
{
    return load_generic<__ATOMIC_RELAXED, __HIP_MEMORY_SCOPE_AGENT>(ptr);
}


template <typename ValueType,
          gcc_atomic_intrinsic_type_map<ValueType>* = nullptr>
__device__ __forceinline__ ValueType load_relaxed_shared(const ValueType* ptr)
{
    return load_generic<__ATOMIC_RELAXED, __HIP_MEMORY_SCOPE_WORKGROUP>(ptr);
}


template <typename ValueType,
          gcc_atomic_intrinsic_type_map<ValueType>* = nullptr>
__device__ __forceinline__ ValueType load_acquire(const ValueType* ptr)
{
    return load_generic<__ATOMIC_ACQUIRE, __HIP_MEMORY_SCOPE_AGENT>(ptr);
}


template <typename ValueType,
          gcc_atomic_intrinsic_type_map<ValueType>* = nullptr>
__device__ __forceinline__ ValueType load_acquire_shared(const ValueType* ptr)
{
    return load_generic<__ATOMIC_ACQUIRE, __HIP_MEMORY_SCOPE_WORKGROUP>(ptr);
}


template <typename ValueType,
          gcc_atomic_intrinsic_type_map<ValueType>* = nullptr>
__device__ __forceinline__ void store_relaxed(ValueType* ptr, ValueType value)
{
    return store_generic<__ATOMIC_RELAXED, __HIP_MEMORY_SCOPE_AGENT>(ptr,
                                                                     value);
}


template <typename ValueType,
          gcc_atomic_intrinsic_type_map<ValueType>* = nullptr>
__device__ __forceinline__ void store_relaxed_shared(ValueType* ptr,
                                                     ValueType value)
{
    return store_generic<__ATOMIC_RELAXED, __HIP_MEMORY_SCOPE_WORKGROUP>(ptr,
                                                                         value);
}


template <typename ValueType,
          gcc_atomic_intrinsic_type_map<ValueType>* = nullptr>
__device__ __forceinline__ void store_release(ValueType* ptr, ValueType value)
{
    return store_generic<__ATOMIC_RELEASE, __HIP_MEMORY_SCOPE_AGENT>(ptr,
                                                                     value);
}


template <typename ValueType,
          gcc_atomic_intrinsic_type_map<ValueType>* = nullptr>
__device__ __forceinline__ void store_release_shared(ValueType* ptr,
                                                     ValueType value)
{
    return store_generic<__ATOMIC_RELEASE, __HIP_MEMORY_SCOPE_WORKGROUP>(ptr,
                                                                         value);
}


template <typename ValueType>
__device__ __forceinline__ thrust::complex<ValueType> load_relaxed(
    const thrust::complex<ValueType>* ptr)
{
    auto real_ptr = reinterpret_cast<const ValueType*>(ptr);
    auto real = load_relaxed(real_ptr);
    auto imag = load_relaxed(real_ptr + 1);
    return {real, imag};
}


template <typename ValueType>
__device__ __forceinline__ thrust::complex<ValueType> load_relaxed_shared(
    const thrust::complex<ValueType>* ptr)
{
    auto real_ptr = reinterpret_cast<const ValueType*>(ptr);
    auto real = load_relaxed_shared(real_ptr);
    auto imag = load_relaxed_shared(real_ptr + 1);
    return {real, imag};
}


template <typename ValueType>
__device__ __forceinline__ void store_relaxed(thrust::complex<ValueType>* ptr,
                                              thrust::complex<ValueType> value)
{
    auto real_ptr = reinterpret_cast<ValueType*>(ptr);
    store_relaxed(real_ptr, value.real());
    store_relaxed(real_ptr + 1, value.imag());
}


template <typename ValueType>
__device__ __forceinline__ void store_relaxed_shared(
    thrust::complex<ValueType>* ptr, thrust::complex<ValueType> value)
{
    auto real_ptr = reinterpret_cast<ValueType*>(ptr);
    store_relaxed_shared(real_ptr, value.real());
    store_relaxed_shared(real_ptr + 1, value.imag());
}


#endif  // !GINKGO_HIP_PLATFORM_NVCC


}  // namespace hip
}  // namespace kernels
}  // namespace gko

#endif  // GKO_HIP_COMPONENTS_MEMORY_HIP_HPP_
