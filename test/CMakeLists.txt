

enable_testing()



add_executable(ds_test ds.test.cpp)
target_link_libraries(
  ds_test
  PRIVATE
    GTest::gtest_main
    pds
)
target_include_directories(ds_test PUBLIC
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
gtest_discover_tests(ds_test DISCOVERY_MODE PRE_TEST)
gtest_discover_tests(bloom_filter_test DISCOVERY_MODE PRE_TEST)


target_code_coverage(ds_test AUTO ALL EXTERNAL)
target_code_coverage(bloom_filter_test AUTO ALL EXTERNAL)
add_subdirectory(benchmark)
