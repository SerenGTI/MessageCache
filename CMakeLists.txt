cmake_minimum_required(VERSION 3.24)


project(messagecache)
project(messagecache LANGUAGES CXX)
project(messagecache VERSION "0.4.0")

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE
    $<INSTALL_INTERFACE:include/coromess>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)

option(MESSAGECACHE_BUILD_EXAMPLES "build examples" OFF)
option(MESSAGECACHE_BUILD_TESTS "build tests" OFF)

if(${MESSAGECACHE_BUILD_EXAMPLES} OR ${MESSAGECACHE_BUILD_TESTS})
    # asio will only come into effect when building tests or examples
    # as such, we do not recommend building examples or tests when using
    # the library as a dependency
    include(cmake/asio.cmake)
endif(${MESSAGECACHE_BUILD_EXAMPLES} OR ${MESSAGECACHE_BUILD_TESTS})

# sanitizers used in tests and examples
include(cmake/sanitizers.cmake)


if(${MESSAGECACHE_BUILD_EXAMPLES})
    add_subdirectory(examples)
endif()

if(${MESSAGECACHE_BUILD_TESTS})
    enable_testing()
    add_subdirectory(test)
endif()

