// SPDX-FileCopyrightText: 2017 - 2024 The Ginkgo authors
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef GKO_PUBLIC_CORE_LOG_PAPI_HPP_
#define GKO_PUBLIC_CORE_LOG_PAPI_HPP_


#include <ginkgo/config.hpp>


#if GKO_HAVE_PAPI_SDE


#include <cstddef>
#include <iostream>
#include <map>
#include <mutex>

#include <sde_lib.h>

#include <ginkgo/core/base/polymorphic_object.hpp>
#include <ginkgo/core/log/logger.hpp>


namespace gko {
namespace log {


static size_type papi_logger_count = 0;
static std::mutex papi_count_mutex;


/**
 * Papi is a Logger which logs every event to the PAPI software. Thanks to this
 * logger, applications which interface with PAPI can access Ginkgo internal
 * data through PAPI.
 * For an example of usage, see examples/papi_logging/papi_logging.cpp
 *
 * The logged values for each event are the following:
 * + all allocation events: number of bytes per executor
 * + all free events: number of calls per executor
 * + copy_started: number of bytes per executor from (to), in
 *   copy_started_from (respectively copy_started_to).
 * + copy_completed: number of bytes per executor from (to), in
 *   copy_completed_from (respectively copy_completed_to).
 * + all polymorphic objects and operation events: number of calls per executor
 * + all apply events: number of calls per LinOp (argument "A").
 * + all factory events: number of calls per factory
 * + criterion_check_completed event: the residual norm is stored in a record
 *   (per criterion)
 * + iteration_complete event: the number of iteration is counted (per solver)
 *
 * @tparam ValueType  the type of values stored in the class (e.g. residuals)
 *
 * @ingroup log
 */
template <typename ValueType = default_precision>
class Papi : public Logger {
public:
    /* Executor events */
    void on_allocation_started(const Executor* exec,
                               const size_type& num_bytes) const override;

    void on_allocation_completed(const Executor* exec,
                                 const size_type& num_bytes,
                                 const uintptr& location) const override;

    void on_free_started(const Executor* exec,
                         const uintptr& location) const override;

    void on_free_completed(const Executor* exec,
                           const uintptr& location) const override;

    void on_copy_started(const Executor* from, const Executor* to,
                         const uintptr& location_from,
                         const uintptr& location_to,
                         const size_type& num_bytes) const override;

    void on_copy_completed(const Executor* from, const Executor* to,
                           const uintptr& location_from,
                           const uintptr& location_to,
                           const size_type& num_bytes) const override;

    /* Operation events */
    void on_operation_launched(const Executor* exec,
                               const Operation* operation) const override;

    void on_operation_completed(const Executor* exec,
                                const Operation* operation) const override;

    /* PolymorphicObject events */
    void on_polymorphic_object_create_started(
        const Executor*, const PolymorphicObject* po) const override;

    void on_polymorphic_object_create_completed(
        const Executor* exec, const PolymorphicObject* input,
        const PolymorphicObject* output) const override;

    void on_polymorphic_object_copy_started(
        const Executor* exec, const PolymorphicObject* from,
        const PolymorphicObject* to) const override;

    void on_polymorphic_object_copy_completed(
        const Executor* exec, const PolymorphicObject* from,
        const PolymorphicObject* to) const override;

    void on_polymorphic_object_move_started(
        const Executor* exec, const PolymorphicObject* from,
        const PolymorphicObject* to) const override;

    void on_polymorphic_object_move_completed(
        const Executor* exec, const PolymorphicObject* from,
        const PolymorphicObject* to) const override;

    void on_polymorphic_object_deleted(
        const Executor* exec, const PolymorphicObject* po) const override;

    /* LinOp events */
    void on_linop_apply_started(const LinOp* A, const LinOp* b,
                                const LinOp* x) const override;

    void on_linop_apply_completed(const LinOp* A, const LinOp* b,
                                  const LinOp* x) const override;

    void on_linop_advanced_apply_started(const LinOp* A, const LinOp* alpha,
                                         const LinOp* b, const LinOp* beta,
                                         const LinOp* x) const override;

    void on_linop_advanced_apply_completed(const LinOp* A, const LinOp* alpha,
                                           const LinOp* b, const LinOp* beta,
                                           const LinOp* x) const override;

    /* LinOpFactory events */
    void on_linop_factory_generate_started(const LinOpFactory* factory,
                                           const LinOp* input) const override;

    void on_linop_factory_generate_completed(
        const LinOpFactory* factory, const LinOp* input,
        const LinOp* output) const override;

    void on_criterion_check_completed(
        const stop::Criterion* criterion, const size_type& num_iterations,
        const LinOp* residual, const LinOp* residual_norm,
        const LinOp* solution, const uint8& stopping_id,
        const bool& set_finalized, const array<stopping_status>* status,
        const bool& one_changed, const bool& all_converged) const override;

    /* Internal solver events */
    void on_iteration_complete(const LinOp* solver, const LinOp* b,
                               const LinOp* x, const size_type& num_iterations,
                               const LinOp* residual,
                               const LinOp* residual_norm,
                               const LinOp* implicit_resnorm_sq,
                               const array<stopping_status>* status,
                               bool stopped) const override;

    GKO_DEPRECATED(
        "Please use the version with the additional stopping "
        "information.")
    void on_iteration_complete(const LinOp* solver,
                               const size_type& num_iterations,
                               const LinOp* residual, const LinOp* solution,
                               const LinOp* residual_norm) const override;

    GKO_DEPRECATED(
        "Please use the version with the additional stopping "
        "information.")
    void on_iteration_complete(
        const LinOp* solver, const size_type& num_iterations,
        const LinOp* residual, const LinOp* solution,
        const LinOp* residual_norm,
        const LinOp* implicit_sq_residual_norm) const override;

    /**
     * Creates a Papi Logger.
     *
     * @param enabled_events  the events enabled for this Logger
     */
    GKO_DEPRECATED("use single-parameter create")
    static std::shared_ptr<Papi> create(
        std::shared_ptr<const gko::Executor>,
        const Logger::mask_type& enabled_events = Logger::all_events_mask)
    {
        return Papi::create(enabled_events);
    }

    /**
     * Creates a Papi Logger.
     *
     * @param enabled_events  the events enabled for this Logger
     */
    static std::shared_ptr<Papi> create(
        const Logger::mask_type& enabled_events = Logger::all_events_mask)
    {
        return std::shared_ptr<Papi>(new Papi(enabled_events), [](auto logger) {
            auto handle = logger->get_handle();
            delete logger;
            papi_sde_shutdown(handle);
        });
    }

    /**
     * Returns the unique name of this logger, which can be used in the
     * PAPI_read() call.
     *
     * @return the unique name of this logger
     */
    const std::string get_handle_name() const { return name; }

    /**
     * Returns the corresponding papi_handle_t for this logger
     *
     * @return the corresponding papi_handle_t for this logger
     */
    const papi_handle_t get_handle() const { return papi_handle; }

protected:
    GKO_DEPRECATED("use single-parameter constructor")
    explicit Papi(
        std::shared_ptr<const gko::Executor> exec,
        const Logger::mask_type& enabled_events = Logger::all_events_mask)
        : Papi(enabled_events)
    {}

    explicit Papi(
        const Logger::mask_type& enabled_events = Logger::all_events_mask)
        : Logger(enabled_events)
    {
        std::ostringstream os;

        std::lock_guard<std::mutex> guard(papi_count_mutex);
        os << "ginkgo" << papi_logger_count;
        name = os.str();
        papi_handle = papi_sde_init(name.c_str());
        papi_logger_count++;
    }

private:
    template <typename PointerType>
    class papi_queue {
    public:
        papi_queue(papi_handle_t* handle, const char* counter_name)
            : handle{handle}, counter_name{counter_name}
        {}

        ~papi_queue()
        {
            for (auto e : data) {
                std::ostringstream oss;
                oss << counter_name << "::" << e.first;
                papi_sde_unregister_counter(*handle, oss.str().c_str());
            }
            data.clear();
        }

        size_type& get_counter(const PointerType* ptr)
        {
            const auto tmp = reinterpret_cast<uintptr>(ptr);
            if (data.find(tmp) == data.end()) {
                data[tmp] = 0;
            }
            auto& value = data[tmp];
            if (!value) {
                std::ostringstream oss;
                oss << counter_name << "::" << tmp;
                papi_sde_register_counter(*handle, oss.str().c_str(),
                                          PAPI_SDE_RO | PAPI_SDE_INSTANT,
                                          PAPI_SDE_long_long, &value);
            }
            return data[tmp];
        }

    private:
        papi_handle_t* handle;
        const char* counter_name;
        std::map<std::uintptr_t, size_type> data;
    };


    mutable papi_queue<Executor> allocation_started{&papi_handle,
                                                    "allocation_started"};
    mutable papi_queue<Executor> allocation_completed{&papi_handle,
                                                      "allocation_completed"};
    mutable papi_queue<Executor> free_started{&papi_handle, "free_started"};
    mutable papi_queue<Executor> free_completed{&papi_handle, "free_completed"};
    mutable papi_queue<Executor> copy_started_from{&papi_handle,
                                                   "copy_started_from"};
    mutable papi_queue<Executor> copy_started_to{&papi_handle,
                                                 "copy_started_to"};
    mutable papi_queue<Executor> copy_completed_from{&papi_handle,
                                                     "copy_completed_from"};
    mutable papi_queue<Executor> copy_completed_to{&papi_handle,
                                                   "copy_completed_to"};

    mutable papi_queue<Executor> operation_launched{&papi_handle,
                                                    "operation_launched"};
    mutable papi_queue<Executor> operation_completed{&papi_handle,
                                                     "operation_completed"};

    mutable papi_queue<Executor> polymorphic_object_create_started{
        &papi_handle, "polymorphic_object_create_started"};
    mutable papi_queue<Executor> polymorphic_object_create_completed{
        &papi_handle, "polymorphic_object_create_completed"};
    mutable papi_queue<Executor> polymorphic_object_copy_started{
        &papi_handle, "polymorphic_object_copy_started"};
    mutable papi_queue<Executor> polymorphic_object_copy_completed{
        &papi_handle, "polymorphic_object_copy_completed"};
    mutable papi_queue<Executor> polymorphic_object_move_started{
        &papi_handle, "polymorphic_object_move_started"};
    mutable papi_queue<Executor> polymorphic_object_move_completed{
        &papi_handle, "polymorphic_object_move_completed"};
    mutable papi_queue<Executor> polymorphic_object_deleted{
        &papi_handle, "polymorphic_object_deleted"};

    mutable papi_queue<LinOpFactory> linop_factory_generate_started{
        &papi_handle, "linop_factory_generate_started"};
    mutable papi_queue<LinOpFactory> linop_factory_generate_completed{
        &papi_handle, "linop_factory_generate_completed"};

    mutable papi_queue<LinOp> linop_apply_started{&papi_handle,
                                                  "linop_apply_started"};
    mutable papi_queue<LinOp> linop_apply_completed{&papi_handle,
                                                    "linop_apply_completed"};
    mutable papi_queue<LinOp> linop_advanced_apply_started{
        &papi_handle, "linop_advanced_apply_started"};
    mutable papi_queue<LinOp> linop_advanced_apply_completed{
        &papi_handle, "linop_advanced_apply_completed"};

    mutable std::map<std::uintptr_t, void*> criterion_check_completed;

    mutable papi_queue<LinOp> iteration_complete{&papi_handle,
                                                 "iteration_complete"};


    std::string name{"ginkgo"};
    papi_handle_t papi_handle;
};


}  // namespace log
}  // namespace gko


#endif  // GKO_HAVE_PAPI_SDE
#endif  // GKO_PUBLIC_CORE_LOG_PAPI_HPP_
