cmake_minimum_required(VERSION 3.24)


project(messagecache)
project(messagecache LANGUAGES CXX)
project(project_name VERSION "0.4.0")

# sanitizers used in tests and examples
include(cmake/sanitizers.cmake)

# Note: asio will only come into effect when building tests or examples
include(cmake/asio.cmake)

option(BUILD_EXAMPLES "build examples" OFF)
option(BUILD_TESTS "build tests" OFF)

if(${BUILD_EXAMPLES})
    add_subdirectory(examples)
endif()

if(${BUILD_TESTS})
    enable_testing()
    add_subdirectory(test)
endif()

