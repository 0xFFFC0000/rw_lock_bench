
################################################
cmake_minimum_required(VERSION 3.19)
project(storage-engine LANGUAGES CXX)

################################################
# CMake utilities
include(cmake/utilities.cmake)

# Universal settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(lock_test LANGUAGES CXX C)

include(FetchContent)

################################################
# Define global variables
set(thirdparty_source ${CMAKE_SOURCE_DIR}/thirdparty CACHE INTERNAL "Thirdparty source location." FORCE)

if(CMAKE_BUILD_TYPE MATCHES Debug)
  add_definitions(-DDEBUGBUILD)
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g3 -ggdb")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3 -ggdb")
endif()

################################################
# Add thirdparty libaries
# Define submodules required commit
set(abseil_dir "abseil-cpp")
set(gflags_dir "gflags")
set(glog_dir "glog")
set(googletest_dir "googletest")
set(benchmark_dir "benchmark")
set(asio_dir "asio")

# lts_2023_08_02
set(abseil_REQUIRED_COMMIT "29bf8085f3bf17b84d30e34b3d7ff8248fda404e")
# v2.2.2
set(gflags_REQUIRED_COMMIT "e171aa2d15ed9eb17054558e0b3a6a413bb01067")
# v0.4.0
set(glog_REQUIRED_COMMIT "96a2f23dca4cc7180821ca5f32e526314395d26a")
# v1.14.x
set(googletest_REQUIRED_COMMIT "f8d7d77c06936315286eb55f8de22cd23c188571")
# v1.8.3
set(benchmark_REQUIRED_CIMMIT "344117638c8ff7e239044fd0fa7085839fc03021")
# boost-1.83.0
set(asio_REQUIRED_CIMMIT "f2fbbd824c1fafa67c1b9c7ee6e4d2649b343a46")

################################################
# Git submodules setup
find_package(Git QUIET)

if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
  set_submodule_with_specific_commit(${thirdparty_source}/${abseil_dir}          ${abseil_REQUIRED_COMMIT})
  set_submodule_with_specific_commit(${thirdparty_source}/${gflags_dir}          ${gflags_REQUIRED_COMMIT})
  set_submodule_with_specific_commit(${thirdparty_source}/${glog_dir}            ${glog_REQUIRED_COMMIT})
  set_submodule_with_specific_commit(${thirdparty_source}/${googletest_dir}      ${googletest_REQUIRED_COMMIT})
  set_submodule_with_specific_commit(${thirdparty_source}/${benchmark_dir}       ${benchmark_REQUIRED_CIMMIT})
  set_submodule_with_specific_commit(${thirdparty_source}/${asio_dir}            ${asio_REQUIRED_CIMMIT})
else()
  message(FATAL_ERROR "Cannot find git executable. Make sure you have installed git.")
endif()

################################################
# Abseil
# include(cmake/abseil.cmake)

################################################
# glog
include(cmake/glog.cmake)

################################################
# gflags
include(cmake/gflags.cmake)

################################################
# googltest only if testing is enabled.
if(BUILD_TESTS)
  include(cmake/googletest.cmake)
endif()

################################################
# googlebenchmark
include(cmake/benchmark.cmake)

################################################
# asio
include(cmake/asio.cmake)


################################################
# Boost
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.45.0 COMPONENTS system filesystem thread date_time chrono regex serialization program_options)

if(NOT Boost_FOUND)
  message(FATAL_ERROR "Cannot find boost libraries")
endif()

add_executable(lock_test lock_test.cpp recursive_shared_mutex.cpp)
target_include_directories(lock_test PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(lock_test PRIVATE gtest glog gflags benchmark ${Boost_LIBRARIES})

