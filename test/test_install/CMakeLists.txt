cmake_minimum_required(VERSION 3.16)

project(TestInstall LANGUAGES CXX)

find_package(Ginkgo REQUIRED
    PATHS # The Path where ginkgo was installed
    # Alternatively, use `cmake -DCMAKE_PREFIX_PATH=<ginkgo_install_dir>` to specify the install directory
    )


# Manage RPATH. This needs to be before the targets.
set(CMAKE_SKIP_BUILD_RPATH OFF)
set(CMAKE_SKIP_INSTALL_RPATH OFF)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)

# Copy the data to the build directory
configure_file(data/A.mtx data/A.mtx COPYONLY)
configure_file(data/b.mtx data/b.mtx COPYONLY)
configure_file(data/x0.mtx data/x0.mtx COPYONLY)

include(CheckLanguage)
check_language(CUDA)

set(HAS_REFERENCE 0)
if (GINKGO_BUILD_REFERENCE)
    set(HAS_REFERENCE 1)
endif()
add_executable(test_install test_install.cpp)
target_compile_definitions(test_install PRIVATE HAS_REFERENCE=${HAS_REFERENCE})
target_link_libraries(test_install PRIVATE Ginkgo::ginkgo)
if(GINKGO_BUILD_MPI)
    find_package(MPI 3.1 COMPONENTS CXX REQUIRED)
    target_link_libraries(test_install PRIVATE MPI::MPI_CXX)
endif()

if(GINKGO_BUILD_CUDA)
    enable_language(CUDA)
    configure_file(test_install.cpp test_install.cu COPYONLY)
    add_executable(test_install_cuda ${CMAKE_CURRENT_BINARY_DIR}/test_install.cu)
    set_target_properties(test_install_cuda PROPERTIES CUDA_STANDARD 17)
    target_compile_definitions(test_install_cuda PRIVATE HAS_CUDA=1)
    target_compile_definitions(test_install_cuda PRIVATE HAS_REFERENCE=${HAS_REFERENCE})
    target_link_libraries(test_install_cuda PRIVATE Ginkgo::ginkgo)
endif()

if(GINKGO_BUILD_HIP)
    enable_language(HIP)
    configure_file(test_install.cpp test_install.hip.cpp COPYONLY)
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/test_install.hip.cpp PROPERTIES LANGUAGE HIP)
    add_executable(test_install_hip ${CMAKE_CURRENT_BINARY_DIR}/test_install.hip.cpp)
    set_target_properties(test_install_hip PROPERTIES HIP_STANDARD 17)

    target_link_libraries(test_install_hip PRIVATE Ginkgo::ginkgo)
    target_compile_definitions(test_install_hip PRIVATE HAS_HIP=1)
    target_compile_definitions(test_install_hip PRIVATE HAS_REFERENCE=${HAS_REFERENCE})
endif()

# Installation step
set(TESTINSTALL_INSTALL_DIR "${GINKGO_INSTALL_PREFIX}/smoke_tests")
set(TESTINSTALL_INSTALL_DATADIR "${TESTINSTALL_INSTALL_DIR}/data")

# Install the binaries and data
install(TARGETS test_install RUNTIME DESTINATION ${TESTINSTALL_INSTALL_DIR})
if(GINKGO_BUILD_CUDA)
    install(TARGETS test_install_cuda RUNTIME DESTINATION ${TESTINSTALL_INSTALL_DIR})
endif()
if(GINKGO_BUILD_HIP)
    install(TARGETS test_install_hip RUNTIME DESTINATION ${TESTINSTALL_INSTALL_DIR})
endif()
install(DIRECTORY "${TestInstall_BINARY_DIR}/data/"
    DESTINATION "${TESTINSTALL_INSTALL_DATADIR}")
