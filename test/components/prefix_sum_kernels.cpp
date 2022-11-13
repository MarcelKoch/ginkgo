/*******************************<GINKGO LICENSE>******************************
Copyright (c) 2017-2022, the Ginkgo authors
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

#include "core/components/prefix_sum_kernels.hpp"


#include <memory>
#include <random>
#include <vector>


#include <gtest/gtest.h>


#include <ginkgo/core/base/array.hpp>


#include "core/test/utils.hpp"
#include "test/utils/executor.hpp"


template <typename T>
class PrefixSum : public CommonTestFixture {
protected:
    using index_type = T;

    PrefixSum()
        : rand(293), total_size(42793), vals(ref, total_size), dvals(exec)
    {
        std::uniform_int_distribution<index_type> dist(0, 1000);
        for (gko::size_type i = 0; i < total_size; ++i) {
            vals.get_data()[i] = dist(rand);
        }
        dvals = vals;
    }

    std::default_random_engine rand;
    gko::size_type total_size;
    gko::array<index_type> vals;
    gko::array<index_type> dvals;
};

TYPED_TEST_SUITE(PrefixSum, gko::test::IndexTypes, TypenameNameGenerator);


TYPED_TEST(PrefixSum, EqualsReference)
{
    using gko::size_type;
    for (auto size :
         {size_type{0}, size_type{1}, size_type{131}, this->total_size}) {
        SCOPED_TRACE(size);
        gko::kernels::reference::components::prefix_sum(
            this->ref, this->vals.get_data(), size);
        gko::kernels::EXEC_NAMESPACE::components::prefix_sum(
            this->exec, this->dvals.get_data(), size);

        GKO_ASSERT_ARRAY_EQ(this->vals, this->dvals);
    }
}
