cmake_minimum_required(VERSION 3.25)
project(pds VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

cmake_dependent_option(PDS_EXAMPLES "Build PDS examples." ON PROJECT_IS_TOP_LEVEL OFF)
cmake_dependent_option(PDS_TESTS "Build PDS test suite." ON PROJECT_IS_TOP_LEVEL OFF)
cmake_dependent_option(PDS_BENCHMARKS "Build PDS benchmarks." ON PROJECT_IS_TOP_LEVEL OFF)


include(FetchContent)

FetchContent_Declare(
  googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.16.0
)
FetchContent_Declare(
  googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.9.1
)
set(BENCHMARK_ENABLE_TESTING OFF)

FetchContent_MakeAvailable(
  googletest
  googlebenchmark
)



list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake/modules)

include(code-coverage)
add_code_coverage_all_targets()


add_library(pds INTERFACE)
target_include_directories(pds INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>)
target_include_directories(pds SYSTEM INTERFACE $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>)

add_subdirectory(thirdparty)

if (PDS_EXAMPLES)
	add_subdirectory(example)
endif()

if (PDS_TESTS)
	add_subdirectory(test)
endif()

if (PDS_BENCHMARKS)
	add_subdirectory(benchmark)
endif()


