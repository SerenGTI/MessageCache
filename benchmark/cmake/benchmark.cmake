include(FetchContent)
include(GNUInstallDirs)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark testing" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "Disable building the unit tests which depend on gtest" FORCE)
set(BENCHMARK_ENABLE_EXCEPTIONS OFF CACHE BOOL "Disable exceptions in benchmark" FORCE)
set(BENCHMARK_ENABLE_LTO ON CACHE BOOL "Enable Link Time Optimization in benchmark" FORCE)
set(BENCHMARK_INSTALL_DOCS OFF CACHE BOOL "Disable installation of benchmark docs" FORCE)
set(BENCHMARK_DOWNLOAD_DEPENDENCIES OFF CACHE BOOL "Allow benchmark to download dependencies" FORCE)
set(BENCHMARK_ENABLE_EXCEPTIONS OFF CACHE BOOL "Disable the use of exceptions in the benchmark library" FORCE)


FetchContent_Declare(
  googlebenchmark
  URL https://github.com/google/benchmark/archive/refs/tags/v1.8.3.tar.gz

)
FetchContent_MakeAvailable(googlebenchmark)