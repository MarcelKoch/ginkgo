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

#include <ginkgo/core/reorder/amd.hpp>


#include <algorithm>
#include <initializer_list>
#include <memory>


#include <gtest/gtest.h>


#include <ginkgo/core/matrix/csr.hpp>


#include "core/factorization/symbolic.hpp"
#include "core/test/utils.hpp"
#include "core/test/utils/assertions.hpp"
#include "matrices/config.hpp"


template <typename ValueIndexType>
class Amd : public ::testing::Test {
protected:
    using value_type =
        typename std::tuple_element<0, decltype(ValueIndexType())>::type;
    using index_type =
        typename std::tuple_element<1, decltype(ValueIndexType())>::type;
    using matrix_type = gko::matrix::Csr<value_type, index_type>;

    Amd() : ref(gko::ReferenceExecutor::create()), permutation_ref{ref} {}

    void setup(
        std::initializer_list<std::initializer_list<value_type>> mtx_list,
        std::initializer_list<index_type> permutation, int fillin_reduction)
    {
        mtx = gko::initialize<matrix_type>(mtx_list, ref);
        permutation_ref = gko::array<index_type>{ref, permutation};
        this->fillin_reduction = fillin_reduction;
        num_rows = mtx->get_size()[0];
    }

    void setup(const char* name_mtx,
               std::initializer_list<index_type> permutation,
               int fillin_reduction)
    {
        std::ifstream stream{name_mtx};
        mtx = gko::read<matrix_type>(stream, this->ref);
        permutation_ref = gko::array<index_type>{ref, permutation};
        this->fillin_reduction = fillin_reduction;
        num_rows = mtx->get_size()[0];
    }

    void forall_matrices(std::function<void()> fn)
    {
        {
            SCOPED_TRACE("ani1");
            this->setup(gko::matrices::location_ani1_mtx,
                        {23, 16, 15, 22, 17, 30, 35, 34, 31, 27, 32, 33,
                         28, 25, 4,  12, 5,  11, 20, 10, 3,  0,  2,  6,
                         1,  7,  14, 19, 26, 8,  9,  13, 18, 21, 29, 24},
                        60);
            fn();
        }
        {
            SCOPED_TRACE("ani1_amd");
            this->setup(gko::matrices::location_ani1_amd_mtx,
                        {1,  3,  2,  0,  29, 4,  5,  13, 12, 11, 10, 14,
                         18, 6,  7,  8,  15, 20, 19, 22, 21, 25, 26, 23,
                         24, 27, 28, 16, 17, 9,  30, 31, 33, 34, 35, 32},
                        -10);
            fn();
        }
        {
            SCOPED_TRACE("example");
            this->setup({{1, 0, 1, 0, 0, 0, 0, 1, 0, 0},
                         {0, 1, 0, 0, 1, 0, 0, 0, 0, 1},
                         {1, 0, 1, 0, 0, 0, 1, 0, 0, 0},
                         {0, 0, 0, 1, 0, 0, 0, 0, 1, 1},
                         {0, 1, 0, 0, 1, 0, 0, 0, 1, 1},
                         {0, 0, 0, 0, 0, 1, 1, 1, 0, 0},
                         {0, 0, 1, 0, 0, 1, 1, 0, 0, 0},
                         {1, 0, 0, 0, 0, 1, 0, 1, 1, 1},
                         {0, 0, 0, 1, 1, 0, 0, 1, 1, 0},
                         {0, 1, 0, 1, 1, 0, 0, 1, 0, 1}},
                        {6, 5, 0, 2, 7, 3, 8, 1, 9, 4}, 0);
            fn();
        }
        {
            SCOPED_TRACE("separable");
            this->setup({{1, 0, 1, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, 1, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
                         {0, 0, 0, 1, 1, 1, 0, 0, 0, 1},
                         {0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 1, 1, 0, 1},
                         {0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
                         {0, 0, 0, 0, 1, 0, 1, 0, 1, 1}},
                        {1, 0, 2, 7, 8, 6, 9, 5, 3, 4}, 0);
            fn();
        }
        {
            SCOPED_TRACE("missing diagonal");
            this->setup({{1, 0, 1, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, 1, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, 0, 1, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
                         {0, 0, 0, 1, 0, 1, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 1, 1, 0, 0, 0},
                         {0, 0, 0, 0, 0, 1, 1, 1, 0, 1},
                         {0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
                         {0, 0, 0, 0, 0, 0, 1, 0, 1, 0}},
                        {8, 9, 7, 6, 5, 4, 3, 1, 0, 2}, -5);
            fn();
        }
    }

    std::shared_ptr<const gko::ReferenceExecutor> ref;
    int fillin_reduction;
    gko::size_type num_rows;
    gko::array<index_type> permutation_ref;
    std::shared_ptr<matrix_type> mtx;
};

TYPED_TEST_SUITE(Amd, gko::test::ValueIndexTypes, PairTypenameNameGenerator);


TYPED_TEST(Amd, WorksAndReducesFillIn)
{
    using matrix_type = typename TestFixture::matrix_type;
    using index_type = typename TestFixture::index_type;
    this->forall_matrices([this] {
        auto amd =
            gko::experimental::reorder::Amd<index_type>::build().on(this->ref);

        auto perm = amd->generate(this->mtx);

        auto perm_array = gko::make_array_view(this->ref, this->num_rows,
                                               perm->get_permutation());
        GKO_ASSERT_ARRAY_EQ(perm_array, this->permutation_ref);

        auto permuted_mtx =
            gko::as<matrix_type>(this->mtx->permute(&perm_array));
        std::unique_ptr<gko::factorization::elimination_forest<index_type>>
            forest;
        std::unique_ptr<matrix_type> factorized_mtx;
        std::unique_ptr<matrix_type> factorized_permuted_mtx;
        gko::factorization::symbolic_cholesky(this->mtx.get(), true,
                                              factorized_mtx, forest);
        gko::factorization::symbolic_cholesky(permuted_mtx.get(), true,
                                              factorized_permuted_mtx, forest);
        int fillin_mtx = factorized_mtx->get_num_stored_elements() -
                         this->mtx->get_num_stored_elements();
        int fillin_permuted =
            factorized_permuted_mtx->get_num_stored_elements() -
            permuted_mtx->get_num_stored_elements();
        ASSERT_LE(fillin_permuted, fillin_mtx - this->fillin_reduction);
    });
}


TYPED_TEST(Amd, ReducesFillInAni4)
{
    using matrix_type = typename TestFixture::matrix_type;
    using index_type = typename TestFixture::index_type;
    this->mtx = gko::read<matrix_type>(
        std::ifstream{gko::matrices::location_ani4_mtx}, this->ref);
    this->num_rows = this->mtx->get_size()[0];
    auto amd =
        gko::experimental::reorder::Amd<index_type>::build().on(this->ref);

    auto perm = amd->generate(this->mtx);

    auto perm_array = gko::make_array_view(this->ref, this->num_rows,
                                           perm->get_permutation());
    auto permuted_mtx = gko::as<matrix_type>(this->mtx->permute(&perm_array));
    std::unique_ptr<gko::factorization::elimination_forest<index_type>> forest;
    std::unique_ptr<matrix_type> factorized_mtx;
    std::unique_ptr<matrix_type> factorized_permuted_mtx;
    gko::factorization::symbolic_cholesky(this->mtx.get(), true, factorized_mtx,
                                          forest);
    gko::factorization::symbolic_cholesky(permuted_mtx.get(), true,
                                          factorized_permuted_mtx, forest);
    int fillin_mtx = factorized_mtx->get_num_stored_elements() -
                     this->mtx->get_num_stored_elements();
    int fillin_permuted = factorized_permuted_mtx->get_num_stored_elements() -
                          permuted_mtx->get_num_stored_elements();
    ASSERT_LE(fillin_permuted, fillin_mtx * 2 / 5);
}
