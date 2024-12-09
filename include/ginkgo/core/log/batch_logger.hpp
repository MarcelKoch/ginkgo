// SPDX-FileCopyrightText: 2017 - 2024 The Ginkgo authors
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef GKO_PUBLIC_CORE_LOG_BATCH_LOGGER_HPP_
#define GKO_PUBLIC_CORE_LOG_BATCH_LOGGER_HPP_


#include <memory>

#include <ginkgo/core/base/batch_multi_vector.hpp>
#include <ginkgo/core/base/types.hpp>
#include <ginkgo/core/log/logger.hpp>


namespace gko {
namespace batch {
/**
 * @brief The logger namespace .
 * @ref log
 * @ingroup log
 */
namespace log {
namespace detail {


/**
 * Stores logging data for batch solver kernels.
 *
 * @note Supports only single rhs
 */
template <typename ValueType>
struct log_data final {
    using real_type = remove_complex<ValueType>;
    using index_type = int;

    log_data(std::shared_ptr<const Executor> exec, size_type num_batch_items);

    log_data(std::shared_ptr<const Executor> exec, size_type num_batch_items,
             array<unsigned char>& workspace);

    /**
     * Stores residual norm values for every linear system in the batch.
     */
    array<real_type> res_norms;

    /**
     * Stores convergence iteration counts for every matrix in the batch
     */
    array<index_type> iter_counts;
};


}  // namespace detail


/**
 * Logs the final residuals and iteration counts for a batch solver.
 *
 * The purpose of this logger is to give simple access to standard data
 * generated by the solver once it has converged.
 *
 * @note The final logged residuals are the implicit residuals that have been
 * computed within the solver process. Depending on the solver algorithm, this
 * may be significantly different from the true residual (||b - Ax||).
 *
 * @ingroup log
 */
template <typename ValueType = default_precision>
class BatchConvergence final : public gko::log::Logger {
public:
    using real_type = remove_complex<ValueType>;
    using index_type = int;
    using mask_type = gko::log::Logger::mask_type;

    void on_batch_solver_completed(
        const array<index_type>& iteration_count,
        const array<real_type>& residual_norm) const override;

    /**
     * Creates a convergence logger. This dynamically allocates the memory,
     * constructs the object and returns an std::unique_ptr to this object.
     * TODO: See if the objects can be pre-allocated beforehand instead of being
     * copied in the `on_<>` event
     *
     * @param exec  the executor
     * @param enabled_events  the events enabled for this logger. By default all
     *                        events.
     *
     * @return an std::unique_ptr to the the constructed object
     */
    static std::unique_ptr<BatchConvergence> create(
        const mask_type& enabled_events =
            gko::log::Logger::batch_solver_completed_mask)
    {
        return std::unique_ptr<BatchConvergence>(
            new BatchConvergence(enabled_events));
    }

    /**
     * @return  The number of iterations for entire batch
     */
    const array<index_type>& get_num_iterations() const noexcept
    {
        return iteration_count_;
    }

    /**
     * @return  The residual norms for the entire batch.
     */
    const array<real_type>& get_residual_norm() const noexcept
    {
        return residual_norm_;
    }

protected:
    explicit BatchConvergence(const mask_type& enabled_events =
                                  gko::log::Logger::batch_solver_completed_mask)
        : gko::log::Logger(enabled_events)
    {}

private:
    mutable array<index_type> iteration_count_{};
    mutable array<real_type> residual_norm_{};
};


}  // namespace log
}  // namespace batch
}  // namespace gko


#endif  // GKO_PUBLIC_CORE_LOG_BATCH_LOGGER_HPP_
