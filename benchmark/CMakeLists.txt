cmake_minimum_required(VERSION 3.24)

include(cmake/benchmark.cmake)

# needed for multithreading
find_package(Threads REQUIRED)

add_executable(benchmarks bench.cpp)
add_executable(benchmarks.tsan bench.cpp)

#set the c++ version
target_compile_features(benchmarks PRIVATE cxx_std_20)
target_compile_features(benchmarks.tsan PRIVATE cxx_std_20)


target_link_libraries(benchmarks LINK_PRIVATE
  ${CMAKE_THREAD_LIBS_INIT}
  benchmark::benchmark
)
target_link_libraries(benchmarks.tsan LINK_PRIVATE
  ${CMAKE_THREAD_LIBS_INIT}
  benchmark::benchmark
  tsan
)

target_include_directories(
  benchmarks PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
target_include_directories(
  benchmarks.tsan PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# custom target `make bench`
add_custom_target(bench
  COMMAND  "${CMAKE_BINARY_DIR}/benchmark/benchmarks"
  DEPENDS  benchmarks
)
# bench with sanitizers
add_custom_target(bench.tsan
  COMMAND  "${CMAKE_BINARY_DIR}/benchmark/benchmarks"
  DEPENDS  benchmarks
)
# set_sanitizers(benchmarks.tsan)
# enable thread sanitizers
target_compile_options(benchmarks.tsan PRIVATE -fsanitize=thread)


