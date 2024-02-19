cmake_minimum_required(VERSION 3.24)


project(messagecache)
project(messagecache LANGUAGES CXX)
project(messagecache VERSION "0.4.0")

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE
    $<INSTALL_INTERFACE:include/coromess>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)


option(BUILD_EXAMPLES "build examples" OFF)
option(BUILD_TESTS "build tests" OFF)

if(${BUILD_EXAMPLES} OR ${BUILD_TESTS})
    # asio will only come into effect when building tests or examples
    # as such, we do not recommend building examples or tests when using
    # the library as a dependency
    include(cmake/asio.cmake)
endif(${BUILD_EXAMPLES} OR ${BUILD_TESTS})

# sanitizers used in tests and examples
include(cmake/sanitizers.cmake)


if(${BUILD_EXAMPLES})
    add_subdirectory(examples)
endif()

if(${BUILD_TESTS})
    enable_testing()
    add_subdirectory(test)
endif()

