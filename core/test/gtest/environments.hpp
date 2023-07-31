#ifndef GINKGO_ENVIRONMENTS_HPP
#define GINKGO_ENVIRONMENTS_HPP

#include <algorithm>
#include <regex>


#ifdef GKO_COMPILING_OMP
#include <omp.h>
#endif


#ifdef GKO_COMPILING_CUDA
#include "cuda/base/device.hpp"
#endif


#ifdef GKO_COMPILING_HIP
#include "hip/base/device.hpp"
#endif


#include <ginkgo/core/base/exception_helpers.hpp>
#include <ginkgo/core/base/executor.hpp>
#include <ginkgo/core/base/mpi.hpp>


struct ctest_resource {
    int id;
    int slots;
};


inline char* get_ctest_group(std::string resource_type, int group_id)
{
    std::transform(resource_type.begin(), resource_type.end(),
                   resource_type.begin(),
                   [](auto c) { return std::toupper(c); });
    std::string rs_group_env = "CTEST_RESOURCE_GROUP_" +
                               std::to_string(group_id) + "_" + resource_type;
    return std::getenv(rs_group_env.c_str());
}


inline ctest_resource parse_ctest_resources(std::string resource)
{
    std::regex re(R"(id\:(\d+),slots\:(\d+))");
    std::smatch match;

    if (!std::regex_match(resource, match, re)) {
        GKO_INVALID_STATE("Can't parse ctest_resource string: " + resource);
    }

    return ctest_resource{std::stoi(match[1]), std::stoi(match[2])};
}


class ResourceEnvironment : public ::testing::Environment {
public:
    explicit ResourceEnvironment(int rank = 0, int size = 1)
    {
#if GINKGO_BUILD_MPI
        if (size > 1) {
            cuda_device_id = gko::experimental::mpi::map_rank_to_device_id(
                MPI_COMM_WORLD,
                std::max(gko::CudaExecutor::get_num_devices(), 1));
            hip_device_id = gko::experimental::mpi::map_rank_to_device_id(
                MPI_COMM_WORLD,
                std::max(gko::HipExecutor::get_num_devices(), 1));
            sycl_device_id = gko::experimental::mpi::map_rank_to_device_id(
                MPI_COMM_WORLD,
                std::max(gko::DpcppExecutor::get_num_devices("gpu"), 1));
        }
#endif

        auto rs_count_env = std::getenv("CTEST_RESOURCE_GROUP_COUNT");
        auto rs_count = rs_count_env ? std::stoi(rs_count_env) : 0;
        if (rs_count == 0) {
            std::cerr << "Running without CTest ctest_resource configuration"
                      << std::endl;
            return;
        }
        if (rs_count != size) {
            GKO_INVALID_STATE("Invalid resource group count: " +
                              std::to_string(rs_count));
        }

        // parse CTest ctest_resource group descriptions
        if (rank == 0) {
            std::cerr << "Running with CTest ctest_resource configuration:"
                      << std::endl;
        }
        // OpenMP CPU threads
        if (auto rs_omp_env = get_ctest_group("cpu", rank)) {
            auto resource = parse_ctest_resources(rs_omp_env);
            omp_threads = resource.slots;
            if (rank == 0) {
                std::cerr << omp_threads << " CPU threads" << std::endl;
            }
        }
        // CUDA GPUs
        if (auto rs_cuda_env = get_ctest_group("cudagpu", rank)) {
            auto resource = parse_ctest_resources(rs_cuda_env);
            cuda_device_id = resource.id;
            if (rank == 0) {
                std::cerr << "CUDA device " << cuda_device_id << std::endl;
            }
        }
        // HIP GPUs
        if (auto rs_hip_env = get_ctest_group("hipgpu", rank)) {
            auto resource = parse_ctest_resources(rs_hip_env);
            hip_device_id = resource.id;
            if (rank == 0) {
                std::cerr << "HIP device " << hip_device_id << std::endl;
            }
        }
        // SYCL GPUs (no other devices!)
        if (auto rs_sycl_env = get_ctest_group("syclgpu", rank)) {
            auto resource = parse_ctest_resources(rs_sycl_env);
            sycl_device_id = resource.id;
            if (rank == 0) {
                std::cerr << "SYCL device " << sycl_device_id << std::endl;
            }
        }
    }

    static int omp_threads;
    static int cuda_device_id;
    static int hip_device_id;
    static int sycl_device_id;
};


#ifdef GKO_COMPILING_OMP

class OmpEnvironment : public ::testing::Environment {
public:
    void SetUp() override
    {
        if (ResourceEnvironment::omp_threads > 0) {
            omp_set_num_threads(ResourceEnvironment::omp_threads);
        }
    }
};

#else


class OmpEnvironment : public ::testing::Environment {};

#endif


#ifdef GKO_COMPILING_CUDA

class CudaEnvironment : public ::testing::Environment {
public:
    void TearDown() override
    {
        gko::kernels::cuda::reset_device(ResourceEnvironment::cuda_device_id);
    }
};

#else

class CudaEnvironment : public ::testing::Environment {};

#endif


#ifdef GKO_COMPILING_HIP

class HipEnvironment : public ::testing::Environment {
public:
    void TearDown() override
    {
        gko::kernels::hip::reset_device(ResourceEnvironment::hip_device_id);
    }
};

#else

class HipEnvironment : public ::testing::Environment {};

#endif


#endif  // GINKGO_ENVIRONMENTS_HPP
