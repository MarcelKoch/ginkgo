// SPDX-FileCopyrightText: 2017 - 2024 The Ginkgo authors
//
// SPDX-License-Identifier: BSD-3-Clause

#include "core/matrix/batch_ell_kernels.hpp"


#include <thrust/functional.h>


#include <ginkgo/core/base/batch_multi_vector.hpp>
#include <ginkgo/core/base/types.hpp>
#include <ginkgo/core/matrix/batch_ell.hpp>


#include "common/cuda_hip/base/config.hpp"
#include "common/cuda_hip/base/runtime.hpp"
#include "common/cuda_hip/components/cooperative_groups.hpp"
#include "core/base/batch_struct.hpp"
#include "core/matrix/batch_struct.hpp"
#include "hip/base/batch_struct.hip.hpp"
#include "hip/base/thrust.hip.hpp"
#include "hip/components/reduction.hip.hpp"
#include "hip/components/thread_ids.hip.hpp"
#include "hip/components/uninitialized_array.hip.hpp"
#include "hip/matrix/batch_struct.hip.hpp"


namespace gko {
namespace kernels {
namespace hip {
/**
 * @brief The Ell matrix format namespace.
 * @ref Ell
 * @ingroup batch_ell
 */
namespace batch_ell {


constexpr auto default_block_size = 256;
constexpr int sm_oversubscription = 4;

// clang-format off

// NOTE: DO NOT CHANGE THE ORDERING OF THE INCLUDES

#include "common/cuda_hip/matrix/batch_ell_kernels.hpp.inc"


#include "common/cuda_hip/matrix/batch_ell_kernel_launcher.hpp.inc"

// clang-format on


}  // namespace batch_ell
}  // namespace hip
}  // namespace kernels
}  // namespace gko
