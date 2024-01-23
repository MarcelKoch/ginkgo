// SPDX-FileCopyrightText: 2017 - 2024 The Ginkgo authors
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef GKO_PUBLIC_CORE_MATRIX_FFT_HPP_
#define GKO_PUBLIC_CORE_MATRIX_FFT_HPP_


#include <ginkgo/core/base/array.hpp>
#include <ginkgo/core/base/lin_op.hpp>


namespace gko {
namespace matrix {


/**
 * This LinOp implements a 1D Fourier matrix using the FFT algorithm.
 *
 * It implements forward and inverse DFT.
 *
 * For a power-of-two size n with corresponding root of unity
 * $\omega = e^{-2\pi i / n}$ for forward DFT and $\omega = e^{2 \pi i / n}$
 * for inverse DFT it computes
 *
 * \f[
 *     x_k = \sum_{j=0}^{n-1} \omega^{jk} b_j
 * \f]
 *
 * without normalization factors.
 *
 * The Reference and OpenMP implementations support only power-of-two input
 * sizes, as they use the Radix-2 algorithm by J. W. Cooley and J. W. Tukey,
 * "An Algorithm for the Machine Calculation of Complex Fourier Series,"
 * Mathematics of Computation, vol. 19, no. 90, pp. 297–301, 1965,
 * doi: 10.2307/2003354.
 * The CUDA and HIP implementations use cuSPARSE/hipSPARSE with full support for
 * non-power-of-two input sizes and special optimizations for products of
 * small prime powers.
 *
 * @ingroup identity
 * @ingroup mat_formats
 * @ingroup LinOp
 */
class Fft : public EnableLinOp<Fft>,
            public WritableToMatrixData<std::complex<float>, int32>,
            public WritableToMatrixData<std::complex<float>, int64>,
            public WritableToMatrixData<std::complex<double>, int32>,
            public WritableToMatrixData<std::complex<double>, int64>,
            public Transposable {
    friend class EnablePolymorphicObject<Fft, LinOp>;

public:
    using EnableLinOp<Fft>::convert_to;
    using EnableLinOp<Fft>::move_to;

    using value_type = std::complex<double>;
    using index_type = int64;
    using transposed_type = Fft;

    std::unique_ptr<LinOp> transpose() const override;

    std::unique_ptr<LinOp> conj_transpose() const override;

    void write(matrix_data<std::complex<float>, int32>& data) const override;

    void write(matrix_data<std::complex<float>, int64>& data) const override;

    void write(matrix_data<std::complex<double>, int32>& data) const override;

    void write(matrix_data<std::complex<double>, int64>& data) const override;

    dim<1> get_fft_size() const;

    bool is_inverse() const;

    /**
     * Creates an empty Fourier matrix.
     *
     * @param exec  Executor associated to the matrix
     */
    static std::unique_ptr<Fft> create(std::shared_ptr<const Executor> exec);

    /**
     * Creates an Fourier matrix with the given dimensions.
     *
     * @param size  size of the matrix
     * @param inverse  true to compute an inverse DFT instead of a normal DFT
     */
    static std::unique_ptr<Fft> create(std::shared_ptr<const Executor> exec,
                                       size_type size = 0,
                                       bool inverse = false);

protected:
    explicit Fft(std::shared_ptr<const Executor> exec);

    Fft(std::shared_ptr<const Executor> exec, size_type size,
        bool inverse = false);

    void apply_impl(const LinOp* b, LinOp* x) const override;

    void apply_impl(const LinOp* alpha, const LinOp* b, const LinOp* beta,
                    LinOp* x) const override;

private:
    mutable array<char> buffer_;
    bool inverse_;
};


/**
 * This LinOp implements a 2D Fourier matrix using the FFT algorithm.
 * For indexing purposes, the first dimension is the major axis.
 *
 * It implements complex-to-complex forward and inverse FFT.
 *
 * For a power-of-two sizes $n_1, n_2$ with corresponding root of unity
 * $\omega = e^{-2\pi i / (n_1 n_2)}$ for forward DFT and
 * $\omega = e^{2 \pi i / (n_1 n_2)}$ for inverse DFT it computes
 *
 * \f[
 *     x_{k_1 n_2 + k_2} = \sum_{i_1=0}^{n_1-1} \sum_{i_2=0}^{n_2-1}
 *                           \omega^{i_1 k_1 + i_2 k_2} b_{i_1 n_2 + i_2}
 * \f]
 *
 * without normalization factors.
 *
 * The Reference and OpenMP implementations support only power-of-two input
 * sizes, as they use the Radix-2 algorithm by J. W. Cooley and J. W. Tukey,
 * "An Algorithm for the Machine Calculation of Complex Fourier Series,"
 * Mathematics of Computation, vol. 19, no. 90, pp. 297–301, 1965,
 * doi: 10.2307/2003354.
 * The CUDA and HIP implementations use cuSPARSE/hipSPARSE with full support for
 * non-power-of-two input sizes and special optimizations for products of
 * small prime powers.
 *
 * @ingroup identity
 * @ingroup mat_formats
 * @ingroup LinOp
 */
class Fft2 : public EnableLinOp<Fft2>,
             public WritableToMatrixData<std::complex<float>, int32>,
             public WritableToMatrixData<std::complex<float>, int64>,
             public WritableToMatrixData<std::complex<double>, int32>,
             public WritableToMatrixData<std::complex<double>, int64>,
             public Transposable {
    friend class EnablePolymorphicObject<Fft2, LinOp>;

public:
    using EnableLinOp<Fft2>::convert_to;
    using EnableLinOp<Fft2>::move_to;

    using value_type = std::complex<double>;
    using index_type = int64;
    using transposed_type = Fft2;

    std::unique_ptr<LinOp> transpose() const override;

    std::unique_ptr<LinOp> conj_transpose() const override;

    void write(matrix_data<std::complex<float>, int32>& data) const override;

    void write(matrix_data<std::complex<float>, int64>& data) const override;

    void write(matrix_data<std::complex<double>, int32>& data) const override;

    void write(matrix_data<std::complex<double>, int64>& data) const override;

    dim<2> get_fft_size() const;

    bool is_inverse() const;

    /**
     * Creates an empty Fourier matrix.
     *
     * @param exec  Executor associated to the matrix
     */
    static std::unique_ptr<Fft2> create(std::shared_ptr<const Executor> exec);

    /**
     * Creates an Fourier matrix with the given dimensions.
     *
     * @param size  size of both FFT dimensions
     */
    static std::unique_ptr<Fft2> create(std::shared_ptr<const Executor> exec,
                                        size_type size);

    /**
     * Creates an Fourier matrix with the given dimensions.
     *
     * @param size1  size of the first FFT dimension
     * @param size2  size of the second FFT dimension
     * @param inverse  true to compute an inverse DFT instead of a normal DFT
     */
    static std::unique_ptr<Fft2> create(std::shared_ptr<const Executor> exec,
                                        size_type size1, size_type size2,
                                        bool inverse = false);

protected:
    explicit Fft2(std::shared_ptr<const Executor> exec);

    Fft2(std::shared_ptr<const Executor> exec, size_type size);

    Fft2(std::shared_ptr<const Executor> exec, size_type size1, size_type size2,
         bool inverse = false);

    void apply_impl(const LinOp* b, LinOp* x) const override;

    void apply_impl(const LinOp* alpha, const LinOp* b, const LinOp* beta,
                    LinOp* x) const override;

private:
    mutable array<char> buffer_;
    dim<2> fft_size_;
    bool inverse_;
};


/**
 * This LinOp implements a 3D Fourier matrix using the FFT algorithm.
 * For indexing purposes, the first dimension is the major axis.
 *
 * It implements complex-to-complex forward and inverse FFT.
 *
 * For a power-of-two sizes $n_1, n_2, n_3$ with corresponding root of unity
 * $\omega = e^{-2\pi i / (n_1 n_2 n_3)}$ for forward DFT and
 * $\omega = e^{2 \pi i / (n_1 n_2 n_3)}$ for inverse DFT it computes
 *
 * \f[
 *     x_{k_1 n_2 n_3 + k_2 n_3 + k_3} = \sum_{i_1=0}^{n_1-1}
 *                     \sum_{i_2=0}^{n_2-1} \sum_{i_3=0}^{n_3-1}
 *                     \omega^{i_1 k_1 + i_2 k_2 + i_3 k_3}
 *                     b_{i_1 n_2 n_3 + i_2 n_3 + i_3}
 * \f]
 *
 * without normalization factors.
 *
 * The Reference and OpenMP implementations support only power-of-two input
 * sizes, as they use the Radix-2 algorithm by J. W. Cooley and J. W. Tukey,
 * "An Algorithm for the Machine Calculation of Complex Fourier Series,"
 * Mathematics of Computation, vol. 19, no. 90, pp. 297–301, 1965,
 * doi: 10.2307/2003354.
 * The CUDA and HIP implementations use cuSPARSE/hipSPARSE with full support for
 * non-power-of-two input sizes and special optimizations for products of
 * small prime powers.
 *
 * @ingroup identity
 * @ingroup mat_formats
 * @ingroup LinOp
 */
class Fft3 : public EnableLinOp<Fft3>,
             public WritableToMatrixData<std::complex<float>, int32>,
             public WritableToMatrixData<std::complex<float>, int64>,
             public WritableToMatrixData<std::complex<double>, int32>,
             public WritableToMatrixData<std::complex<double>, int64>,
             public Transposable {
    friend class EnablePolymorphicObject<Fft3, LinOp>;

public:
    using EnableLinOp<Fft3>::convert_to;
    using EnableLinOp<Fft3>::move_to;

    using value_type = std::complex<double>;
    using index_type = int64;
    using transposed_type = Fft3;

    std::unique_ptr<LinOp> transpose() const override;

    std::unique_ptr<LinOp> conj_transpose() const override;

    void write(matrix_data<std::complex<float>, int32>& data) const override;

    void write(matrix_data<std::complex<float>, int64>& data) const override;

    void write(matrix_data<std::complex<double>, int32>& data) const override;

    void write(matrix_data<std::complex<double>, int64>& data) const override;

    dim<3> get_fft_size() const;

    bool is_inverse() const;

    /**
     * Creates an empty Fourier matrix.
     *
     * @param exec  Executor associated to the matrix
     */
    static std::unique_ptr<Fft3> create(std::shared_ptr<const Executor> exec);

    /**
     * Creates an Fourier matrix with the given dimensions.
     *
     * @param size  size of all FFT dimensions
     */
    static std::unique_ptr<Fft3> create(std::shared_ptr<const Executor> exec,
                                        size_type size);

    /**
     * Creates an Fourier matrix with the given dimensions.
     *
     * @param size1  size of the first FFT dimension
     * @param size2  size of the second FFT dimension
     * @param size3  size of the third FFT dimension
     * @param inverse  true to compute an inverse DFT instead of a normal DFT
     */
    static std::unique_ptr<Fft3> create(std::shared_ptr<const Executor> exec,
                                        size_type size1, size_type size2,
                                        size_type size3, bool inverse = false);

protected:
    explicit Fft3(std::shared_ptr<const Executor> exec);

    Fft3(std::shared_ptr<const Executor> exec, size_type size);

    Fft3(std::shared_ptr<const Executor> exec, size_type size1, size_type size2,
         size_type size3, bool inverse = false);

    void apply_impl(const LinOp* b, LinOp* x) const override;

    void apply_impl(const LinOp* alpha, const LinOp* b, const LinOp* beta,
                    LinOp* x) const override;

private:
    mutable array<char> buffer_;
    dim<3> fft_size_;
    bool inverse_;
};


}  // namespace matrix
}  // namespace gko


#endif  // GKO_PUBLIC_CORE_MATRIX_FFT_HPP_
