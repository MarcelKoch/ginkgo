// SPDX-FileCopyrightText: 2017 - 2024 The Ginkgo authors
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef GKO_COMMON_CUDA_HIP_BASE_MATH_HPP_INC_
#define GKO_COMMON_CUDA_HIP_BASE_MATH_HPP_INC_


#include <thrust/complex.h>

#include <ginkgo/core/base/math.hpp>


namespace gko {


// We need this struct, because otherwise we would call a __host__ function in a
// __device__ function (even though it is constexpr)
template <typename T>
struct device_numeric_limits {
    static constexpr auto inf = std::numeric_limits<T>::infinity();
    static constexpr auto max = std::numeric_limits<T>::max();
    static constexpr auto min = std::numeric_limits<T>::min();
};


namespace detail {


template <typename T>
struct remove_complex_impl<thrust::complex<T>> {
    using type = T;
};


template <typename T>
struct is_complex_impl<thrust::complex<T>>
    : public std::integral_constant<bool, true> {};


template <typename T>
struct is_complex_or_scalar_impl<thrust::complex<T>> : std::is_scalar<T> {};


template <typename T>
struct truncate_type_impl<thrust::complex<T>> {
    using type = thrust::complex<typename truncate_type_impl<T>::type>;
};


}  // namespace detail


}  // namespace gko


#endif  // GKO_COMMON_CUDA_HIP_BASE_MATH_HPP_INC_
