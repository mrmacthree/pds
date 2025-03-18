add_executable(demo demo.cpp)
target_link_libraries(demo PRIVATE pds MurmurHash3)
target_include_directories(demo PUBLIC
                          "${PROJECT_SOURCE_DIR}/thirdparty"
                          )
target_code_coverage(demo AUTO ALL EXTERNAL)
