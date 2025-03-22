

enable_testing()



add_executable(hash_test hash.test.cpp)
target_link_libraries(
  hash_test
  PRIVATE
    GTest::gtest_main
    pds
    MurmurHash3
)
target_include_directories(hash_test PUBLIC
                          "${PROJECT_SOURCE_DIR}/thirdparty"
                          )
add_executable(bloom_filter_test bloom_filter.test.cpp)
target_link_libraries(
  bloom_filter_test
  PRIVATE
    GTest::gtest_main
    pds
    MurmurHash3
)
target_include_directories(bloom_filter_test PUBLIC
                          "${PROJECT_SOURCE_DIR}/thirdparty"
                          )
include(GoogleTest)
gtest_discover_tests(hash_test DISCOVERY_MODE PRE_TEST)
gtest_discover_tests(bloom_filter_test DISCOVERY_MODE PRE_TEST)


target_code_coverage(hash_test AUTO ALL EXTERNAL)
target_code_coverage(bloom_filter_test AUTO ALL EXTERNAL)
add_subdirectory(benchmark)
