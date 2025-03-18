add_library(MurmurHash3 STATIC MurmurHash3.cpp)
target_code_coverage(MurmurHash3 AUTO)
target_include_directories(MurmurHash3 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/>)
target_include_directories(MurmurHash3 SYSTEM PUBLIC $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/thirdparty>)
