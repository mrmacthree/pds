
add_executable(ds_benchmark)
target_sources(
  ds_benchmark
  PRIVATE
    ds.benchmark.cpp
)
target_link_libraries(
  ds_benchmark
  PRIVATE
    pds
    MurmurHash3
    pthread
    benchmark::benchmark 
)
