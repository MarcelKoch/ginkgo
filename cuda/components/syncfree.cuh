// SPDX-FileCopyrightText: 2017 - 2024 The Ginkgo authors
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef GKO_CUDA_COMPONENTS_SYNCFREE_CUH_
#define GKO_CUDA_COMPONENTS_SYNCFREE_CUH_


#include <ginkgo/core/base/array.hpp>


#include "common/cuda_hip/base/config.hpp"
#include "common/cuda_hip/components/cooperative_groups.hpp"
#include "common/cuda_hip/components/memory.hpp"
#include "core/components/fill_array_kernels.hpp"
#include "cuda/components/atomic.cuh"


namespace gko {
namespace kernels {
namespace cuda {


#include "common/cuda_hip/components/syncfree.hpp.inc"


}  // namespace cuda
}  // namespace kernels
}  // namespace gko


#endif  // GKO_CUDA_COMPONENTS_SYNCFREE_CUH_
