/*******************************<GINKGO LICENSE>******************************
Copyright (c) 2017-2023, the Ginkgo authors
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
******************************<GINKGO LICENSE>*******************************/

#ifndef GKO_CUDA_BASE_KERNEL_CONFIG_HPP_
#define GKO_CUDA_BASE_KERNEL_CONFIG_HPP_


#include <cuda_runtime.h>
#include <ginkgo/core/base/exception_helpers.hpp>


namespace gko {
namespace kernels {
namespace cuda {
namespace detail {


template <typename ValueType>
class shared_memory_config_guard {
public:
    using value_type = ValueType;
    shared_memory_config_guard() : original_config_{}
    {
        GKO_ASSERT_NO_CUDA_ERRORS(
            cudaDeviceGetSharedMemConfig(&original_config_));

        if (sizeof(value_type) == 4) {
            GKO_ASSERT_NO_CUDA_ERRORS(
                cudaDeviceSetSharedMemConfig(cudaSharedMemBankSizeFourByte));
        } else if (sizeof(value_type) % 8 == 0) {
            GKO_ASSERT_NO_CUDA_ERRORS(
                cudaDeviceSetSharedMemConfig(cudaSharedMemBankSizeEightByte));
        } else {
            GKO_ASSERT_NO_CUDA_ERRORS(
                cudaDeviceSetSharedMemConfig(cudaSharedMemBankSizeDefault));
        }
    }


    ~shared_memory_config_guard()
    {
        auto error_code = cudaDeviceSetSharedMemConfig(original_config_);
        if (error_code != cudaSuccess) {
#if GKO_VERBOSE_LEVEL >= 1
            std::cerr << "Unrecoverable CUDA error while resetting the "
                         "shared memory config to "
                      << original_config_ << " in " << __func__ << ": "
                      << cudaGetErrorName(error_code) << ": "
                      << cudaGetErrorString(error_code) << std::endl
                      << "Exiting program" << std::endl;
#endif  // GKO_VERBOSE_LEVEL >= 1
            std::exit(error_code);
        }
    }

private:
    cudaSharedMemConfig original_config_;
};


}  // namespace detail
}  // namespace cuda
}  // namespace kernels
}  // namespace gko


#endif  // GKO_CUDA_BASE_KERNEL_CONFIG_HPP_
