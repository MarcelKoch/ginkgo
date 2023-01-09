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

#include <iostream>
#include <map>
#include <string>


#include <omp.h>
#include <Kokkos_Core.hpp>
#include <ginkgo/ginkgo.hpp>


// Creates a stencil matrix in CSR format for the given number of discretization
// points.
template <typename ValueType, typename IndexType>
void generate_stencil_matrix(gko::matrix::Csr<ValueType, IndexType>* matrix)
{
    auto exec = matrix->get_executor();
    const auto discretization_points = matrix->get_size()[0];

    // Over-allocate storage for the matrix elements. Each row has 3 entries,
    // except for the first and last one. To handle each row uniformly, we
    // allocate space for 3x the number of rows.
    gko::device_matrix_data<ValueType, IndexType> md(exec, matrix->get_size(),
                                                     discretization_points * 3);

    // Create Kokkos views on Ginkgo data.
    Kokkos::View<IndexType*> v_row_idxs(md.get_row_idxs(), md.get_num_elems());
    Kokkos::View<IndexType*> v_col_idxs(md.get_col_idxs(), md.get_num_elems());
    Kokkos::View<ValueType*> v_values(md.get_values(), md.get_num_elems());

    // Create the matrix entries. This also creates zero entries for the
    // first and second row to handle all rows uniformly.
    Kokkos::parallel_for(
        "generate_stencil_matrix", md.get_num_elems(), KOKKOS_LAMBDA(int i) {
            const ValueType coefs[] = {-1, 2, -1};
            auto ofs = static_cast<IndexType>((i % 3) - 1);
            auto row = static_cast<IndexType>(i / 3);
            auto col = row + ofs;

            // To prevent branching, a mask is used to set the entry to
            // zero, if the column is out-of-bounds
            auto mask =
                static_cast<IndexType>(0 <= col && col < discretization_points);

            v_row_idxs[i] = mask * row;
            v_col_idxs[i] = mask * col;
            v_values[i] = mask * coefs[ofs + 1];
        });

    // Add up duplicate (zero) entries.
    md.sum_duplicates();

    // Build Csr matrix.
    matrix->read(std::move(md));
}


// Generates the RHS vector given `f` and the boundary conditions.
template <typename Closure, typename ValueType>
void generate_rhs(Closure&& f, ValueType u0, ValueType u1,
                  gko::matrix::Dense<ValueType>* rhs)
{
    const auto discretization_points = rhs->get_size()[0];
    auto values = rhs->get_values();
    Kokkos::View<ValueType*> values_view(values, discretization_points);
    Kokkos::parallel_for(
        "generate_rhs", discretization_points, KOKKOS_LAMBDA(int i) {
            const ValueType h = 1.0 / (discretization_points + 1);
            const ValueType xi = ValueType(i + 1) * h;
            values_view[i] = -f(xi) * h * h;
            if (i == 0) {
                values_view[i] += u0;
            }
            if (i == discretization_points - 1) {
                values_view[i] += u1;
            }
        });
}


// Computes the 1-norm of the error given the computed `u` and the correct
// solution function `correct_u`.
template <typename Closure, typename ValueType>
double calculate_error(int discretization_points,
                       const gko::matrix::Dense<ValueType>* u,
                       Closure&& correct_u)
{
    Kokkos::View<const ValueType*> v_u(u->get_const_values(),
                                       discretization_points);
    auto error = 0.0;
    Kokkos::parallel_reduce(
        "calculate_error", discretization_points,
        KOKKOS_LAMBDA(int i, double& lsum) {
            const auto h = 1.0 / (discretization_points + 1);
            const auto xi = (i + 1) * h;
            lsum += Kokkos::Experimental::abs(
                (v_u(i) - correct_u(xi)) /
                Kokkos::Experimental::abs(correct_u(xi)));
        },
        error);
    return error;
}


int main(int argc, char* argv[])
{
    Kokkos::ScopeGuard kokkos(argc, argv);

    // Some shortcuts
    using ValueType = double;
    using RealValueType = gko::remove_complex<ValueType>;
    using IndexType = int;

    using vec = gko::matrix::Dense<ValueType>;
    using mtx = gko::matrix::Csr<ValueType, IndexType>;
    using cg = gko::solver::Cg<ValueType>;
    using bj = gko::preconditioner::Jacobi<ValueType>;

    // Print help message. For details on the kokkos-options see
    // https://kokkos.github.io/kokkos-core-wiki/ProgrammingGuide/Initialization.html#initialization-by-command-line-arguments
    if (argc == 2 && (std::string(argv[1]) == "--help")) {
        std::cerr << "Usage: " << argv[0]
                  << " [discretization_points] [kokkos-options]" << std::endl;
        std::exit(-1);
    }

    const unsigned int discretization_points =
        argc >= 2 ? std::atoi(argv[1]) : 100u;

    // chooses the executor that corresponds to the Kokkos DefaultExecutionSpace
    auto exec = []() -> std::shared_ptr<gko::Executor> {
#ifdef KOKKOS_ENABLE_SERIAL
        if (std::is_same<Kokkos::DefaultExecutionSpace,
                         Kokkos::Serial>::value) {
            return gko::ReferenceExecutor::create();
        }
#endif
#ifdef KOKKOS_ENABLE_OPENMP
        if (std::is_same<Kokkos::DefaultExecutionSpace,
                         Kokkos::OpenMP>::value) {
            return gko::OmpExecutor::create();
        }
#endif
#ifdef KOKKOS_ENABLE_CUDA
        if (std::is_same<Kokkos::DefaultExecutionSpace, Kokkos::Cuda>::value) {
            return gko::CudaExecutor::create(0,
                                             gko::ReferenceExecutor::create());
        }
#endif
#ifdef KOKKOS_ENABLE_HIP
        if (std::is_same<Kokkos::DefaultExecutionSpace, Kokkos::HIP>::value) {
            return gko::HipExecutor::create(0,
                                            gko::ReferenceExecutor::create());
        }
#endif
    }();

    // problem:
    auto correct_u = [] KOKKOS_FUNCTION(ValueType x) { return x * x * x; };
    auto f = [] KOKKOS_FUNCTION(ValueType x) { return ValueType{6} * x; };
    auto u0 = correct_u(0);
    auto u1 = correct_u(1);

    // initialize vectors
    auto rhs = vec::create(exec, gko::dim<2>(discretization_points, 1));
    generate_rhs(f, u0, u1, lend(rhs));
    auto u = vec::create(exec, gko::dim<2>(discretization_points, 1));
    for (int i = 0; i < u->get_size()[0]; ++i) {
        u->get_values()[i] = 0.0;
    }

    // initialize the stencil matrix
    auto A = share(mtx::create(
        exec, gko::dim<2>{discretization_points, discretization_points}));
    generate_stencil_matrix(lend(A));

    const RealValueType reduction_factor{1e-7};
    // Generate solver and solve the system
    cg::build()
        .with_criteria(gko::stop::Iteration::build()
                           .with_max_iters(discretization_points)
                           .on(exec),
                       gko::stop::ResidualNorm<ValueType>::build()
                           .with_reduction_factor(reduction_factor)
                           .on(exec))
        .with_preconditioner(bj::build().on(exec))
        .on(exec)
        ->generate(A)
        ->apply(lend(rhs), lend(u));

    std::cout << "\nSolve complete."
              << "\nThe average relative error is "
              << calculate_error(discretization_points, lend(u), correct_u) /
                     discretization_points
              << std::endl;
}