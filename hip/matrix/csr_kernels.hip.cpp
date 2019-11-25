/*******************************<GINKGO LICENSE>******************************
Copyright (c) 2017-2019, the Ginkgo authors
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

#include "core/matrix/csr_kernels.hpp"


#include <algorithm>


#include <hip/hip_runtime.h>


#include <ginkgo/core/base/array.hpp>
#include <ginkgo/core/base/exception_helpers.hpp>
#include <ginkgo/core/base/math.hpp>
#include <ginkgo/core/matrix/coo.hpp>
#include <ginkgo/core/matrix/dense.hpp>
#include <ginkgo/core/matrix/ell.hpp>
#include <ginkgo/core/matrix/hybrid.hpp>
#include <ginkgo/core/matrix/sellp.hpp>


#include "core/matrix/dense_kernels.hpp"
#include "core/synthesizer/implementation_selection.hpp"
#include "hip/base/config.hip.hpp"
#include "hip/base/hipsparse_bindings.hip.hpp"
#include "hip/base/math.hip.hpp"
#include "hip/base/pointer_mode_guard.hip.hpp"
#include "hip/base/types.hip.hpp"
#include "hip/components/atomic.hip.hpp"
#include "hip/components/cooperative_groups.hip.hpp"
#include "hip/components/prefix_sum.hip.hpp"
#include "hip/components/reduction.hip.hpp"
#include "hip/components/segment_scan.hip.hpp"
#include "hip/components/uninitialized_array.hip.hpp"
#include "hip/components/zero_array.hip.hpp"


namespace gko {
namespace kernels {
namespace hip {
/**
 * @brief The Compressed sparse row matrix format namespace.
 *
 * @ingroup csr
 */
namespace csr {


constexpr int default_block_size = 512;
constexpr int warps_in_block = 4;
constexpr int spmv_block_size = warps_in_block * config::warp_size;
constexpr int classical_block_size = 64;
constexpr int wsize = config::warp_size;


/**
 * A compile-time list of the number items per threads for which spmv kernel
 * should be compiled.
 */
using compiled_kernels = syn::value_list<int, 3, 4, 6, 7, 8, 12, 14>;


#include "common/matrix/csr_kernels.hpp.inc"


namespace host_kernel {


template <int items_per_thread, typename ValueType, typename IndexType>
void merge_path_spmv(syn::value_list<int, items_per_thread>,
                     std::shared_ptr<const HipExecutor> exec,
                     const matrix::Csr<ValueType, IndexType> *a,
                     const matrix::Dense<ValueType> *b,
                     matrix::Dense<ValueType> *c,
                     const matrix::Dense<ValueType> *alpha = nullptr,
                     const matrix::Dense<ValueType> *beta = nullptr)
{
    const IndexType total = a->get_size()[0] + a->get_num_stored_elements();
    const IndexType grid_num =
        ceildiv(total, spmv_block_size * items_per_thread);
    const dim3 grid(grid_num);
    const dim3 block(spmv_block_size);
    Array<IndexType> row_out(exec, grid_num);
    Array<ValueType> val_out(exec, grid_num);

    for (IndexType column_id = 0; column_id < b->get_size()[1]; column_id++) {
        if (alpha == nullptr && beta == nullptr) {
            const auto b_vals = b->get_const_values() + column_id;
            auto c_vals = c->get_values() + column_id;
            hipLaunchKernelGGL(
                HIP_KERNEL_NAME(
                    kernel::abstract_merge_path_spmv<items_per_thread>),
                dim3(grid), dim3(block), 0, 0,
                static_cast<IndexType>(a->get_size()[0]),
                as_hip_type(a->get_const_values()), a->get_const_col_idxs(),
                as_hip_type(a->get_const_row_ptrs()),
                as_hip_type(a->get_const_srow()), as_hip_type(b_vals),
                b->get_stride(), as_hip_type(c_vals), c->get_stride(),
                as_hip_type(row_out.get_data()),
                as_hip_type(val_out.get_data()));
            hipLaunchKernelGGL(kernel::abstract_reduce, dim3(1),
                               dim3(spmv_block_size), 0, 0, grid_num,
                               as_hip_type(val_out.get_data()),
                               as_hip_type(row_out.get_data()),
                               as_hip_type(c_vals), c->get_stride());

        } else if (alpha != nullptr && beta != nullptr) {
            const auto b_vals = b->get_const_values() + column_id;
            auto c_vals = c->get_values() + column_id;
            hipLaunchKernelGGL(
                HIP_KERNEL_NAME(
                    kernel::abstract_merge_path_spmv<items_per_thread>),
                dim3(grid), dim3(block), 0, 0,
                static_cast<IndexType>(a->get_size()[0]),
                as_hip_type(alpha->get_const_values()),
                as_hip_type(a->get_const_values()), a->get_const_col_idxs(),
                as_hip_type(a->get_const_row_ptrs()),
                as_hip_type(a->get_const_srow()), as_hip_type(b_vals),
                b->get_stride(), as_hip_type(beta->get_const_values()),
                as_hip_type(c_vals), c->get_stride(),
                as_hip_type(row_out.get_data()),
                as_hip_type(val_out.get_data()));
            hipLaunchKernelGGL(kernel::abstract_reduce, dim3(1),
                               dim3(spmv_block_size), 0, 0, grid_num,
                               as_hip_type(val_out.get_data()),
                               as_hip_type(row_out.get_data()),
                               as_hip_type(alpha->get_const_values()),
                               as_hip_type(c_vals), c->get_stride());
        } else {
            GKO_KERNEL_NOT_FOUND;
        }
    }
}

GKO_ENABLE_IMPLEMENTATION_SELECTION(select_merge_path_spmv, merge_path_spmv);


template <typename ValueType, typename IndexType>
int compute_items_per_thread(std::shared_ptr<const HipExecutor> exec)
{
#if GINKGO_HIP_PLATFORM_NVCC


    const int version = exec->get_major_version()
                        << 4 + exec->get_minor_version();
    // The num_item is decided to make the occupancy 100%
    // TODO: Extend this list when new GPU is released
    //       Tune this parameter
    // 128 threads/block the number of items per threads
    // 3.0 3.5: 6
    // 3.7: 14
    // 5.0, 5.3, 6.0, 6.2: 8
    // 5.2, 6.1, 7.0: 12
    int num_item = 6;
    switch (version) {
    case 0x50:
    case 0x53:
    case 0x60:
    case 0x62:
        num_item = 8;
        break;
    case 0x52:
    case 0x61:
    case 0x70:
        num_item = 12;
        break;
    case 0x37:
        num_item = 14;
    }


#else


    // HIP uses the minimal num_item to make the code work correctly.
    // TODO: this parameter should be tuned.
    int num_item = 6;


#endif  // GINKGO_HIP_PLATFORM_NVCC


    // Ensure that the following is satisfied:
    // sizeof(IndexType) + sizeof(ValueType)
    // <= items_per_thread * sizeof(IndexType)
    constexpr int minimal_num =
        ceildiv(sizeof(IndexType) + sizeof(ValueType), sizeof(IndexType));
    int items_per_thread = num_item * 4 / sizeof(IndexType);
    return std::max(minimal_num, items_per_thread);
}


}  // namespace host_kernel


template <typename ValueType, typename IndexType>
void spmv(std::shared_ptr<const HipExecutor> exec,
          const matrix::Csr<ValueType, IndexType> *a,
          const matrix::Dense<ValueType> *b, matrix::Dense<ValueType> *c)
{
    if (a->get_strategy()->get_name() == "load_balance") {
        zero_array(c->get_num_stored_elements(), c->get_values());
        const IndexType nwarps = a->get_num_srow_elements();
        if (nwarps > 0) {
            const dim3 csr_block(config::warp_size, warps_in_block, 1);
            const dim3 csr_grid(ceildiv(nwarps, warps_in_block),
                                b->get_size()[1]);
            hipLaunchKernelGGL(
                kernel::abstract_spmv, dim3(csr_grid), dim3(csr_block), 0, 0,
                nwarps, static_cast<IndexType>(a->get_size()[0]),
                as_hip_type(a->get_const_values()), a->get_const_col_idxs(),
                as_hip_type(a->get_const_row_ptrs()),
                as_hip_type(a->get_const_srow()),
                as_hip_type(b->get_const_values()),
                as_hip_type(b->get_stride()), as_hip_type(c->get_values()),
                as_hip_type(c->get_stride()));
        } else {
            GKO_NOT_SUPPORTED(nwarps);
        }
    } else if (a->get_strategy()->get_name() == "merge_path") {
        int items_per_thread =
            host_kernel::compute_items_per_thread<ValueType, IndexType>(exec);
        host_kernel::select_merge_path_spmv(
            compiled_kernels(),
            [&items_per_thread](int compiled_info) {
                return items_per_thread == compiled_info;
            },
            syn::value_list<int>(), syn::type_list<>(), exec, a, b, c);
    } else if (a->get_strategy()->get_name() == "classical") {
        const dim3 grid(ceildiv(a->get_size()[0], classical_block_size),
                        b->get_size()[1]);
        hipLaunchKernelGGL(kernel::abstract_classical_spmv, dim3(grid),
                           dim3(classical_block_size), 0, 0, a->get_size()[0],
                           as_hip_type(a->get_const_values()),
                           a->get_const_col_idxs(),
                           as_hip_type(a->get_const_row_ptrs()),
                           as_hip_type(b->get_const_values()), b->get_stride(),
                           as_hip_type(c->get_values()), c->get_stride());
    } else if (a->get_strategy()->get_name() == "sparselib" ||
               a->get_strategy()->get_name() == "cusparse") {
        if (hipsparse::is_supported<ValueType, IndexType>::value) {
            // TODO: add implementation for int64 and multiple RHS
            auto handle = exec->get_hipsparse_handle();
            auto descr = hipsparse::create_mat_descr();
            {
                hipsparse::pointer_mode_guard pm_guard(handle);
                auto row_ptrs = a->get_const_row_ptrs();
                auto col_idxs = a->get_const_col_idxs();
                auto alpha = one<ValueType>();
                auto beta = zero<ValueType>();
                if (b->get_stride() != 1 || c->get_stride() != 1) {
                    GKO_NOT_IMPLEMENTED;
                }
                hipsparse::spmv(handle, HIPSPARSE_OPERATION_NON_TRANSPOSE,
                                a->get_size()[0], a->get_size()[1],
                                a->get_num_stored_elements(), &alpha, descr,
                                a->get_const_values(), row_ptrs, col_idxs,
                                b->get_const_values(), &beta, c->get_values());
            }
            hipsparse::destroy(descr);
        } else {
            GKO_NOT_IMPLEMENTED;
        }
    } else {
        GKO_NOT_IMPLEMENTED;
    }
}

GKO_INSTANTIATE_FOR_EACH_VALUE_AND_INDEX_TYPE(GKO_DECLARE_CSR_SPMV_KERNEL);


template <typename ValueType, typename IndexType>
void advanced_spmv(std::shared_ptr<const HipExecutor> exec,
                   const matrix::Dense<ValueType> *alpha,
                   const matrix::Csr<ValueType, IndexType> *a,
                   const matrix::Dense<ValueType> *b,
                   const matrix::Dense<ValueType> *beta,
                   matrix::Dense<ValueType> *c)
{
    if (a->get_strategy()->get_name() == "load_balance") {
        dense::scale(exec, beta, c);

        const IndexType nwarps = a->get_num_srow_elements();

        if (nwarps > 0) {
            const dim3 csr_block(config::warp_size, warps_in_block, 1);
            const dim3 csr_grid(ceildiv(nwarps, warps_in_block),
                                b->get_size()[1]);
            hipLaunchKernelGGL(
                kernel::abstract_spmv, dim3(csr_grid), dim3(csr_block), 0, 0,
                nwarps, static_cast<IndexType>(a->get_size()[0]),
                as_hip_type(alpha->get_const_values()),
                as_hip_type(a->get_const_values()), a->get_const_col_idxs(),
                as_hip_type(a->get_const_row_ptrs()),
                as_hip_type(a->get_const_srow()),
                as_hip_type(b->get_const_values()),
                as_hip_type(b->get_stride()), as_hip_type(c->get_values()),
                as_hip_type(c->get_stride()));
        } else {
            GKO_NOT_SUPPORTED(nwarps);
        }
    } else if (a->get_strategy()->get_name() == "sparselib" ||
               a->get_strategy()->get_name() == "cusparse") {
        if (hipsparse::is_supported<ValueType, IndexType>::value) {
            // TODO: add implementation for int64 and multiple RHS
            auto descr = hipsparse::create_mat_descr();

            auto row_ptrs = a->get_const_row_ptrs();
            auto col_idxs = a->get_const_col_idxs();

            if (b->get_stride() != 1 || c->get_stride() != 1)
                GKO_NOT_IMPLEMENTED;

            hipsparse::spmv(exec->get_hipsparse_handle(),
                            HIPSPARSE_OPERATION_NON_TRANSPOSE, a->get_size()[0],
                            a->get_size()[1], a->get_num_stored_elements(),
                            alpha->get_const_values(), descr,
                            a->get_const_values(), row_ptrs, col_idxs,
                            b->get_const_values(), beta->get_const_values(),
                            c->get_values());

            hipsparse::destroy(descr);
        } else {
            GKO_NOT_IMPLEMENTED;
        }
    } else if (a->get_strategy()->get_name() == "classical") {
        const dim3 grid(ceildiv(a->get_size()[0], classical_block_size),
                        b->get_size()[1]);
        hipLaunchKernelGGL(kernel::abstract_classical_spmv, dim3(grid),
                           dim3(classical_block_size), 0, 0, a->get_size()[0],
                           as_hip_type(alpha->get_const_values()),
                           as_hip_type(a->get_const_values()),
                           a->get_const_col_idxs(),
                           as_hip_type(a->get_const_row_ptrs()),
                           as_hip_type(b->get_const_values()), b->get_stride(),
                           as_hip_type(beta->get_const_values()),
                           as_hip_type(c->get_values()), c->get_stride());
    } else if (a->get_strategy()->get_name() == "merge_path") {
        int items_per_thread =
            host_kernel::compute_items_per_thread<ValueType, IndexType>(exec);
        host_kernel::select_merge_path_spmv(
            compiled_kernels(),
            [&items_per_thread](int compiled_info) {
                return items_per_thread == compiled_info;
            },
            syn::value_list<int>(), syn::type_list<>(), exec, a, b, c, alpha,
            beta);
    } else {
        GKO_NOT_IMPLEMENTED;
    }
}

GKO_INSTANTIATE_FOR_EACH_VALUE_AND_INDEX_TYPE(
    GKO_DECLARE_CSR_ADVANCED_SPMV_KERNEL);


template <typename ValueType, typename IndexType>
void spgemm(std::shared_ptr<const HipExecutor> exec,
            const matrix::Csr<ValueType, IndexType> *a,
            const matrix::Csr<ValueType, IndexType> *b,
            const matrix::Csr<ValueType, IndexType> *c,
            Array<IndexType> &c_row_ptrs, Array<IndexType> &c_col_idxs,
            Array<ValueType> &c_vals) GKO_NOT_IMPLEMENTED;

GKO_INSTANTIATE_FOR_EACH_VALUE_AND_INDEX_TYPE(GKO_DECLARE_CSR_SPGEMM_KERNEL);


template <typename ValueType, typename IndexType>
void advanced_spgemm(std::shared_ptr<const HipExecutor> exec,
                     const matrix::Dense<ValueType> *alpha,
                     const matrix::Csr<ValueType, IndexType> *a,
                     const matrix::Csr<ValueType, IndexType> *b,
                     const matrix::Dense<ValueType> *beta,
                     const matrix::Csr<ValueType, IndexType> *c,
                     Array<IndexType> &c_row_ptrs, Array<IndexType> &c_col_idxs,
                     Array<ValueType> &c_vals) GKO_NOT_IMPLEMENTED;

GKO_INSTANTIATE_FOR_EACH_VALUE_AND_INDEX_TYPE(
    GKO_DECLARE_CSR_ADVANCED_SPGEMM_KERNEL);


template <typename IndexType>
void convert_row_ptrs_to_idxs(std::shared_ptr<const HipExecutor> exec,
                              const IndexType *ptrs, size_type num_rows,
                              IndexType *idxs)
{
    const auto grid_dim = ceildiv(num_rows, default_block_size);

    hipLaunchKernelGGL(kernel::convert_row_ptrs_to_idxs, dim3(grid_dim),
                       dim3(default_block_size), 0, 0, num_rows,
                       as_hip_type(ptrs), as_hip_type(idxs));
}


template <typename ValueType, typename IndexType>
void convert_to_coo(std::shared_ptr<const HipExecutor> exec,
                    matrix::Coo<ValueType, IndexType> *result,
                    const matrix::Csr<ValueType, IndexType> *source)
{
    auto num_rows = result->get_size()[0];

    auto row_idxs = result->get_row_idxs();
    const auto source_row_ptrs = source->get_const_row_ptrs();

    convert_row_ptrs_to_idxs(exec, source_row_ptrs, num_rows, row_idxs);
}

GKO_INSTANTIATE_FOR_EACH_VALUE_AND_INDEX_TYPE(
    GKO_DECLARE_CSR_CONVERT_TO_COO_KERNEL);


template <typename ValueType, typename IndexType>
void convert_to_dense(std::shared_ptr<const HipExecutor> exec,
                      matrix::Dense<ValueType> *result,
                      const matrix::Csr<ValueType, IndexType> *source)
{
    const auto num_rows = result->get_size()[0];
    const auto num_cols = result->get_size()[1];
    const auto stride = result->get_stride();
    const auto row_ptrs = source->get_const_row_ptrs();
    const auto col_idxs = source->get_const_col_idxs();
    const auto vals = source->get_const_values();

    const dim3 block_size(config::warp_size,
                          config::max_block_size / config::warp_size, 1);
    const dim3 init_grid_dim(ceildiv(stride, block_size.x),
                             ceildiv(num_rows, block_size.y), 1);
    hipLaunchKernelGGL(kernel::initialize_zero_dense, dim3(init_grid_dim),
                       dim3(block_size), 0, 0, num_rows, num_cols, stride,
                       as_hip_type(result->get_values()));

    auto grid_dim = ceildiv(num_rows, default_block_size);
    hipLaunchKernelGGL(
        kernel::fill_in_dense, dim3(grid_dim), dim3(default_block_size), 0, 0,
        num_rows, as_hip_type(row_ptrs), as_hip_type(col_idxs),
        as_hip_type(vals), stride, as_hip_type(result->get_values()));
}

GKO_INSTANTIATE_FOR_EACH_VALUE_AND_INDEX_TYPE(
    GKO_DECLARE_CSR_CONVERT_TO_DENSE_KERNEL);


template <typename ValueType, typename IndexType>
void convert_to_sellp(std::shared_ptr<const HipExecutor> exec,
                      matrix::Sellp<ValueType, IndexType> *result,
                      const matrix::Csr<ValueType, IndexType> *source)
{
    const auto num_rows = result->get_size()[0];
    const auto num_cols = result->get_size()[1];

    auto result_values = result->get_values();
    auto result_col_idxs = result->get_col_idxs();
    auto slice_lengths = result->get_slice_lengths();
    auto slice_sets = result->get_slice_sets();

    const auto slice_size = (result->get_slice_size() == 0)
                                ? matrix::default_slice_size
                                : result->get_slice_size();
    const auto stride_factor = (result->get_stride_factor() == 0)
                                   ? matrix::default_stride_factor
                                   : result->get_stride_factor();
    const int slice_num = ceildiv(num_rows, slice_size);

    const auto source_values = source->get_const_values();
    const auto source_row_ptrs = source->get_const_row_ptrs();
    const auto source_col_idxs = source->get_const_col_idxs();

    auto nnz_per_row = Array<size_type>(exec, num_rows);
    auto grid_dim = ceildiv(num_rows, default_block_size);

    hipLaunchKernelGGL(kernel::calculate_nnz_per_row, dim3(grid_dim),
                       dim3(default_block_size), 0, 0, num_rows,
                       as_hip_type(source_row_ptrs),
                       as_hip_type(nnz_per_row.get_data()));

    grid_dim = slice_num;

    hipLaunchKernelGGL(kernel::calculate_slice_lengths, dim3(grid_dim),
                       dim3(config::warp_size), 0, 0, num_rows, slice_size,
                       stride_factor, as_hip_type(nnz_per_row.get_const_data()),
                       as_hip_type(slice_lengths), as_hip_type(slice_sets));

    auto add_values =
        Array<size_type>(exec, ceildiv(slice_num + 1, default_block_size));
    grid_dim = ceildiv(slice_num + 1, default_block_size);

    hipLaunchKernelGGL(HIP_KERNEL_NAME(start_prefix_sum<default_block_size>),
                       dim3(grid_dim), dim3(default_block_size), 0, 0,
                       slice_num + 1, as_hip_type(slice_sets),
                       as_hip_type(add_values.get_data()));

    hipLaunchKernelGGL(HIP_KERNEL_NAME(finalize_prefix_sum<default_block_size>),
                       dim3(grid_dim), dim3(default_block_size), 0, 0,
                       slice_num + 1, as_hip_type(slice_sets),
                       as_hip_type(add_values.get_const_data()));

    grid_dim = ceildiv(num_rows, default_block_size);
    hipLaunchKernelGGL(kernel::fill_in_sellp, dim3(grid_dim),
                       dim3(default_block_size), 0, 0, num_rows, slice_size,
                       as_hip_type(source_values), as_hip_type(source_row_ptrs),
                       as_hip_type(source_col_idxs), as_hip_type(slice_lengths),
                       as_hip_type(slice_sets), as_hip_type(result_col_idxs),
                       as_hip_type(result_values));

    nnz_per_row.clear();
    add_values.clear();
}

GKO_INSTANTIATE_FOR_EACH_VALUE_AND_INDEX_TYPE(
    GKO_DECLARE_CSR_CONVERT_TO_SELLP_KERNEL);


template <typename ValueType, typename IndexType>
void convert_to_ell(std::shared_ptr<const HipExecutor> exec,
                    matrix::Ell<ValueType, IndexType> *result,
                    const matrix::Csr<ValueType, IndexType> *source)
{
    const auto source_values = source->get_const_values();
    const auto source_row_ptrs = source->get_const_row_ptrs();
    const auto source_col_idxs = source->get_const_col_idxs();

    auto result_values = result->get_values();
    auto result_col_idxs = result->get_col_idxs();
    const auto stride = result->get_stride();
    const auto max_nnz_per_row = result->get_num_stored_elements_per_row();
    const auto num_rows = result->get_size()[0];
    const auto num_cols = result->get_size()[1];

    const auto init_grid_dim =
        ceildiv(max_nnz_per_row * num_rows, default_block_size);

    hipLaunchKernelGGL(kernel::initialize_zero_ell, dim3(init_grid_dim),
                       dim3(default_block_size), 0, 0, max_nnz_per_row, stride,
                       as_hip_type(result_values),
                       as_hip_type(result_col_idxs));

    const auto grid_dim =
        ceildiv(num_rows * config::warp_size, default_block_size);

    hipLaunchKernelGGL(kernel::fill_in_ell, dim3(grid_dim),
                       dim3(default_block_size), 0, 0, num_rows, stride,
                       as_hip_type(source_values), as_hip_type(source_row_ptrs),
                       as_hip_type(source_col_idxs), as_hip_type(result_values),
                       as_hip_type(result_col_idxs));
}

GKO_INSTANTIATE_FOR_EACH_VALUE_AND_INDEX_TYPE(
    GKO_DECLARE_CSR_CONVERT_TO_ELL_KERNEL);


template <typename ValueType, typename IndexType>
void calculate_total_cols(std::shared_ptr<const HipExecutor> exec,
                          const matrix::Csr<ValueType, IndexType> *source,
                          size_type *result, size_type stride_factor,
                          size_type slice_size)
{
    const auto num_rows = source->get_size()[0];
    const auto slice_num = ceildiv(num_rows, slice_size);
    const auto row_ptrs = source->get_const_row_ptrs();

    auto nnz_per_row = Array<size_type>(exec, num_rows);
    auto grid_dim = ceildiv(num_rows, default_block_size);

    hipLaunchKernelGGL(kernel::calculate_nnz_per_row, dim3(grid_dim),
                       dim3(default_block_size), 0, 0, num_rows,
                       as_hip_type(row_ptrs),
                       as_hip_type(nnz_per_row.get_data()));

    grid_dim = ceildiv(slice_num * config::warp_size, default_block_size);
    auto max_nnz_per_slice = Array<size_type>(exec, slice_num);

    hipLaunchKernelGGL(kernel::reduce_max_nnz_per_slice, dim3(grid_dim),
                       dim3(default_block_size), 0, 0, num_rows, slice_size,
                       stride_factor, as_hip_type(nnz_per_row.get_const_data()),
                       as_hip_type(max_nnz_per_slice.get_data()));

    grid_dim = ceildiv(slice_num, default_block_size);
    auto block_results = Array<size_type>(exec, grid_dim);

    hipLaunchKernelGGL(kernel::reduce_total_cols, dim3(grid_dim),
                       dim3(default_block_size), 0, 0, slice_num,
                       as_hip_type(max_nnz_per_slice.get_const_data()),
                       as_hip_type(block_results.get_data()));

    auto d_result = Array<size_type>(exec, 1);

    hipLaunchKernelGGL(kernel::reduce_total_cols, dim3(1),
                       dim3(default_block_size), 0, 0, grid_dim,
                       as_hip_type(block_results.get_const_data()),
                       as_hip_type(d_result.get_data()));

    exec->get_master()->copy_from(exec.get(), 1, d_result.get_const_data(),
                                  result);

    block_results.clear();
    nnz_per_row.clear();
    max_nnz_per_slice.clear();
    d_result.clear();
}

GKO_INSTANTIATE_FOR_EACH_VALUE_AND_INDEX_TYPE(
    GKO_DECLARE_CSR_CALCULATE_TOTAL_COLS_KERNEL);


template <typename ValueType, typename IndexType>
void transpose(std::shared_ptr<const HipExecutor> exec,
               matrix::Csr<ValueType, IndexType> *trans,
               const matrix::Csr<ValueType, IndexType> *orig)
{
    if (hipsparse::is_supported<ValueType, IndexType>::value) {
        hipsparseAction_t copyValues = HIPSPARSE_ACTION_NUMERIC;
        hipsparseIndexBase_t idxBase = HIPSPARSE_INDEX_BASE_ZERO;

        hipsparse::transpose(
            exec->get_hipsparse_handle(), orig->get_size()[0],
            orig->get_size()[1], orig->get_num_stored_elements(),
            orig->get_const_values(), orig->get_const_row_ptrs(),
            orig->get_const_col_idxs(), trans->get_values(),
            trans->get_col_idxs(), trans->get_row_ptrs(), copyValues, idxBase);
    } else {
        GKO_NOT_IMPLEMENTED;
    }
}

GKO_INSTANTIATE_FOR_EACH_VALUE_AND_INDEX_TYPE(GKO_DECLARE_CSR_TRANSPOSE_KERNEL);


template <typename ValueType, typename IndexType>
void conj_transpose(std::shared_ptr<const HipExecutor> exec,
                    matrix::Csr<ValueType, IndexType> *trans,
                    const matrix::Csr<ValueType, IndexType> *orig)
{
    if (hipsparse::is_supported<ValueType, IndexType>::value) {
        const dim3 block_size(default_block_size, 1, 1);
        const dim3 grid_size(
            ceildiv(trans->get_num_stored_elements(), block_size.x), 1, 1);

        hipsparseAction_t copyValues = HIPSPARSE_ACTION_NUMERIC;
        hipsparseIndexBase_t idxBase = HIPSPARSE_INDEX_BASE_ZERO;

        hipsparse::transpose(
            exec->get_hipsparse_handle(), orig->get_size()[0],
            orig->get_size()[1], orig->get_num_stored_elements(),
            orig->get_const_values(), orig->get_const_row_ptrs(),
            orig->get_const_col_idxs(), trans->get_values(),
            trans->get_col_idxs(), trans->get_row_ptrs(), copyValues, idxBase);

        hipLaunchKernelGGL(conjugate_kernel, dim3(grid_size), dim3(block_size),
                           0, 0, trans->get_num_stored_elements(),
                           as_hip_type(trans->get_values()));
    } else {
        GKO_NOT_IMPLEMENTED;
    }
}

GKO_INSTANTIATE_FOR_EACH_VALUE_AND_INDEX_TYPE(
    GKO_DECLARE_CSR_CONJ_TRANSPOSE_KERNEL);


template <typename ValueType, typename IndexType>
void row_permute(std::shared_ptr<const HipExecutor> exec,
                 const Array<IndexType> *permutation_indices,
                 matrix::Csr<ValueType, IndexType> *row_permuted,
                 const matrix::Csr<ValueType, IndexType> *orig)
    GKO_NOT_IMPLEMENTED;

GKO_INSTANTIATE_FOR_EACH_VALUE_AND_INDEX_TYPE(
    GKO_DECLARE_CSR_ROW_PERMUTE_KERNEL);


template <typename ValueType, typename IndexType>
void column_permute(std::shared_ptr<const HipExecutor> exec,
                    const Array<IndexType> *permutation_indices,
                    matrix::Csr<ValueType, IndexType> *column_permuted,
                    const matrix::Csr<ValueType, IndexType> *orig)
    GKO_NOT_IMPLEMENTED;

GKO_INSTANTIATE_FOR_EACH_VALUE_AND_INDEX_TYPE(
    GKO_DECLARE_CSR_COLUMN_PERMUTE_KERNEL);


template <typename ValueType, typename IndexType>
void inverse_row_permute(std::shared_ptr<const HipExecutor> exec,
                         const Array<IndexType> *permutation_indices,
                         matrix::Csr<ValueType, IndexType> *row_permuted,
                         const matrix::Csr<ValueType, IndexType> *orig)
    GKO_NOT_IMPLEMENTED;

GKO_INSTANTIATE_FOR_EACH_VALUE_AND_INDEX_TYPE(
    GKO_DECLARE_CSR_INVERSE_ROW_PERMUTE_KERNEL);


template <typename ValueType, typename IndexType>
void inverse_column_permute(std::shared_ptr<const HipExecutor> exec,
                            const Array<IndexType> *permutation_indices,
                            matrix::Csr<ValueType, IndexType> *column_permuted,
                            const matrix::Csr<ValueType, IndexType> *orig)
    GKO_NOT_IMPLEMENTED;

GKO_INSTANTIATE_FOR_EACH_VALUE_AND_INDEX_TYPE(
    GKO_DECLARE_CSR_INVERSE_COLUMN_PERMUTE_KERNEL);


template <typename ValueType, typename IndexType>
void calculate_max_nnz_per_row(std::shared_ptr<const HipExecutor> exec,
                               const matrix::Csr<ValueType, IndexType> *source,
                               size_type *result)
{
    const auto num_rows = source->get_size()[0];

    auto nnz_per_row = Array<size_type>(exec, num_rows);
    auto block_results = Array<size_type>(exec, default_block_size);
    auto d_result = Array<size_type>(exec, 1);

    const auto grid_dim = ceildiv(num_rows, default_block_size);
    hipLaunchKernelGGL(kernel::calculate_nnz_per_row, dim3(grid_dim),
                       dim3(default_block_size), 0, 0, num_rows,
                       as_hip_type(source->get_const_row_ptrs()),
                       as_hip_type(nnz_per_row.get_data()));

    const auto n = ceildiv(num_rows, default_block_size);
    const auto reduce_dim = n <= default_block_size ? n : default_block_size;
    hipLaunchKernelGGL(kernel::reduce_max_nnz, dim3(reduce_dim),
                       dim3(default_block_size), 0, 0, num_rows,
                       as_hip_type(nnz_per_row.get_const_data()),
                       as_hip_type(block_results.get_data()));

    hipLaunchKernelGGL(kernel::reduce_max_nnz, dim3(1),
                       dim3(default_block_size), 0, 0, reduce_dim,
                       as_hip_type(block_results.get_const_data()),
                       as_hip_type(d_result.get_data()));

    exec->get_master()->copy_from(exec.get(), 1, d_result.get_const_data(),
                                  result);

    nnz_per_row.clear();
    block_results.clear();
    d_result.clear();
}

GKO_INSTANTIATE_FOR_EACH_VALUE_AND_INDEX_TYPE(
    GKO_DECLARE_CSR_CALCULATE_MAX_NNZ_PER_ROW_KERNEL);


template <typename ValueType, typename IndexType>
void convert_to_hybrid(std::shared_ptr<const HipExecutor> exec,
                       matrix::Hybrid<ValueType, IndexType> *result,
                       const matrix::Csr<ValueType, IndexType> *source)
{
    auto ell_val = result->get_ell_values();
    auto ell_col = result->get_ell_col_idxs();
    auto coo_val = result->get_coo_values();
    auto coo_col = result->get_coo_col_idxs();
    auto coo_row = result->get_coo_row_idxs();
    const auto stride = result->get_ell_stride();
    const auto max_nnz_per_row = result->get_ell_num_stored_elements_per_row();
    const auto num_rows = result->get_size()[0];
    const auto coo_num_stored_elements = result->get_coo_num_stored_elements();
    auto grid_dim = ceildiv(max_nnz_per_row * num_rows, default_block_size);

    hipLaunchKernelGGL(kernel::initialize_zero_ell, dim3(grid_dim),
                       dim3(default_block_size), 0, 0, max_nnz_per_row, stride,
                       as_hip_type(ell_val), as_hip_type(ell_col));

    grid_dim = ceildiv(num_rows, default_block_size);
    auto coo_offset = Array<size_type>(exec, num_rows);
    hipLaunchKernelGGL(kernel::calculate_hybrid_coo_row_nnz, dim3(grid_dim),
                       dim3(default_block_size), 0, 0, num_rows,
                       max_nnz_per_row,
                       as_hip_type(source->get_const_row_ptrs()),
                       as_hip_type(coo_offset.get_data()));

    auto add_values =
        Array<size_type>(exec, ceildiv(num_rows, default_block_size));
    grid_dim = ceildiv(num_rows, default_block_size);
    hipLaunchKernelGGL(HIP_KERNEL_NAME(start_prefix_sum<default_block_size>),
                       dim3(grid_dim), dim3(default_block_size), 0, 0, num_rows,
                       as_hip_type(coo_offset.get_data()),
                       as_hip_type(add_values.get_data()));
    hipLaunchKernelGGL(HIP_KERNEL_NAME(finalize_prefix_sum<default_block_size>),
                       dim3(grid_dim), dim3(default_block_size), 0, 0, num_rows,
                       as_hip_type(coo_offset.get_data()),
                       as_hip_type(add_values.get_const_data()));

    grid_dim = ceildiv(num_rows * config::warp_size, default_block_size);
    hipLaunchKernelGGL(kernel::fill_in_hybrid, dim3(grid_dim),
                       dim3(default_block_size), 0, 0, num_rows, stride,
                       max_nnz_per_row, as_hip_type(source->get_const_values()),
                       as_hip_type(source->get_const_row_ptrs()),
                       as_hip_type(source->get_const_col_idxs()),
                       as_hip_type(coo_offset.get_const_data()),
                       as_hip_type(ell_val), as_hip_type(ell_col),
                       as_hip_type(coo_val), as_hip_type(coo_col),
                       as_hip_type(coo_row));
}

GKO_INSTANTIATE_FOR_EACH_VALUE_AND_INDEX_TYPE(
    GKO_DECLARE_CSR_CONVERT_TO_HYBRID_KERNEL);


template <typename ValueType, typename IndexType>
void calculate_nonzeros_per_row(std::shared_ptr<const HipExecutor> exec,
                                const matrix::Csr<ValueType, IndexType> *source,
                                Array<size_type> *result)
{
    const auto num_rows = source->get_size()[0];
    auto row_ptrs = source->get_const_row_ptrs();
    auto grid_dim = ceildiv(num_rows, default_block_size);

    hipLaunchKernelGGL(kernel::calculate_nnz_per_row, dim3(grid_dim),
                       dim3(default_block_size), 0, 0, num_rows,
                       as_hip_type(row_ptrs), as_hip_type(result->get_data()));
}

GKO_INSTANTIATE_FOR_EACH_VALUE_AND_INDEX_TYPE(
    GKO_DECLARE_CSR_CALCULATE_NONZEROS_PER_ROW_KERNEL);


template <typename ValueType, typename IndexType>
void sort_by_column_index(std::shared_ptr<const HipExecutor> exec,
                          matrix::Csr<ValueType, IndexType> *to_sort)
    GKO_NOT_IMPLEMENTED;

GKO_INSTANTIATE_FOR_EACH_VALUE_AND_INDEX_TYPE(
    GKO_DECLARE_CSR_SORT_BY_COLUMN_INDEX);


template <typename ValueType, typename IndexType>
void is_sorted_by_column_index(
    std::shared_ptr<const HipExecutor> exec,
    const matrix::Csr<ValueType, IndexType> *to_check,
    bool *is_sorted) GKO_NOT_IMPLEMENTED;

GKO_INSTANTIATE_FOR_EACH_VALUE_AND_INDEX_TYPE(
    GKO_DECLARE_CSR_IS_SORTED_BY_COLUMN_INDEX);


}  // namespace csr
}  // namespace hip
}  // namespace kernels
}  // namespace gko