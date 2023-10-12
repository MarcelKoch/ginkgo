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

#include <ginkgo/core/matrix/scaled_permutation.hpp>


#include <ginkgo/core/base/exception_helpers.hpp>
#include <ginkgo/core/base/executor.hpp>
#include <ginkgo/core/base/precision_dispatch.hpp>


#include "core/matrix/scaled_permutation_kernels.hpp"


namespace gko {
namespace matrix {
namespace scaled_permutation {
namespace {


GKO_REGISTER_OPERATION(invert, scaled_permutation::invert);
GKO_REGISTER_OPERATION(combine, scaled_permutation::combine);


}  // namespace
}  // namespace scaled_permutation


template <typename ValueType, typename IndexType>
ScaledPermutation<ValueType, IndexType>::ScaledPermutation(
    std::shared_ptr<const Executor> exec, size_type size)
    : ScaledPermutation{exec, array<ValueType>{exec, size},
                        array<IndexType>{exec, size}}
{}


template <typename ValueType, typename IndexType>
ScaledPermutation<ValueType, IndexType>::ScaledPermutation(
    std::shared_ptr<const Executor> exec, array<value_type> scaling_factors,
    array<index_type> permutation_indices)
    : EnableLinOp<ScaledPermutation>(exec,
                                     dim<2>{scaling_factors.get_num_elems(),
                                            scaling_factors.get_num_elems()}),
      scale_{exec, std::move(scaling_factors)},
      permutation_{exec, std::move(permutation_indices)}
{
    GKO_ASSERT_EQ(scale_.get_num_elems(), permutation_.get_num_elems());
}


template <typename ValueType, typename IndexType>
std::unique_ptr<ScaledPermutation<ValueType, IndexType>>
ScaledPermutation<ValueType, IndexType>::create(
    std::shared_ptr<const Executor> exec, size_type size)
{
    return std::unique_ptr<ScaledPermutation>(
        new ScaledPermutation{exec, size});
}


template <typename ValueType, typename IndexType>
std::unique_ptr<ScaledPermutation<ValueType, IndexType>>
ScaledPermutation<ValueType, IndexType>::create(
    ptr_param<const Permutation<IndexType>> permutation)
{
    const auto exec = permutation->get_executor();
    const auto size = permutation->get_size()[0];
    array<value_type> scale{exec, size};
    array<index_type> perm{exec, size};
    exec->copy(size, permutation->get_const_permutation(), perm.get_data());
    scale.fill(one<ValueType>());
    return create(exec, std::move(scale), std::move(perm));
}


template <typename ValueType, typename IndexType>
std::unique_ptr<ScaledPermutation<ValueType, IndexType>>
ScaledPermutation<ValueType, IndexType>::create(
    std::shared_ptr<const Executor> exec, array<value_type> scaling_factors,
    array<index_type> permutation_indices)
{
    return std::unique_ptr<ScaledPermutation>(new ScaledPermutation{
        exec, std::move(scaling_factors), std::move(permutation_indices)});
}


template <typename ValueType, typename IndexType>
std::unique_ptr<const ScaledPermutation<ValueType, IndexType>>
ScaledPermutation<ValueType, IndexType>::create_const(
    std::shared_ptr<const Executor> exec,
    gko::detail::const_array_view<value_type>&& scale,
    gko::detail::const_array_view<index_type>&& perm_idxs)
{
    return create(exec, gko::detail::array_const_cast(std::move(scale)),
                  gko::detail::array_const_cast(std::move(perm_idxs)));
}


template <typename ValueType, typename IndexType>
std::unique_ptr<ScaledPermutation<ValueType, IndexType>>
ScaledPermutation<ValueType, IndexType>::invert() const
{
    const auto exec = this->get_executor();
    const auto size = this->get_size()[0];
    auto result = ScaledPermutation::create(exec, size);
    exec->run(scaled_permutation::make_invert(
        this->get_const_scale(), this->get_const_permutation(), size,
        result->get_scale(), result->get_permutation()));
    return result;
}


template <typename ValueType, typename IndexType>
std::unique_ptr<ScaledPermutation<ValueType, IndexType>>
ScaledPermutation<ValueType, IndexType>::combine(
    ptr_param<const ScaledPermutation> other) const
{
    GKO_ASSERT_EQUAL_DIMENSIONS(this, other);
    const auto exec = this->get_executor();
    const auto size = this->get_size()[0];
    const auto local_other = make_temporary_clone(exec, other);
    auto result = ScaledPermutation::create(exec, size);
    exec->run(scaled_permutation::make_combine(
        this->get_const_scale(), this->get_const_permutation(),
        local_other->get_const_scale(), local_other->get_const_permutation(),
        size, result->get_scale(), result->get_permutation()));
    return result;
}


template <typename ValueType, typename IndexType>
void ScaledPermutation<ValueType, IndexType>::apply_impl(const LinOp* b,
                                                         LinOp* x) const
{
    precision_dispatch_real_complex<ValueType>(
        [this](auto dense_b, auto dense_x) {
            dense_b->scale_permute(this, dense_x, permute_mode::rows);
        },
        b, x);
}


template <typename ValueType, typename IndexType>
void ScaledPermutation<ValueType, IndexType>::apply_impl(const LinOp* alpha,
                                                         const LinOp* b,
                                                         const LinOp* beta,
                                                         LinOp* x) const
{
    precision_dispatch_real_complex<ValueType>(
        [this](auto dense_alpha, auto dense_b, auto dense_beta, auto dense_x) {
            auto tmp = dense_b->scale_permute(this, permute_mode::rows);
            dense_x->scale(dense_beta);
            dense_x->add_scaled(dense_alpha, tmp);
        },
        alpha, b, beta, x);
}


template <typename ValueType, typename IndexType>
void ScaledPermutation<ValueType, IndexType>::write(
    gko::matrix_data<value_type, index_type>& data) const
{
    const auto host_this =
        make_temporary_clone(this->get_executor()->get_master(), this);
    data.size = this->get_size();
    data.nonzeros.clear();
    data.nonzeros.reserve(data.size[0]);
    for (IndexType row = 0; row < this->get_size()[0]; row++) {
        auto col = host_this->get_const_permutation()[row];
        data.nonzeros.emplace_back(row, col, host_this->get_const_scale()[col]);
    }
}


#define GKO_DECLARE_SCALED_PERMUTATION_MATRIX(ValueType, IndexType) \
    class ScaledPermutation<ValueType, IndexType>
GKO_INSTANTIATE_FOR_EACH_VALUE_AND_INDEX_TYPE(
    GKO_DECLARE_SCALED_PERMUTATION_MATRIX);


}  // namespace matrix
}  // namespace gko
