cmake_minimum_required(VERSION 3.24)


project(messagecache)
project(messagecache LANGUAGES CXX)
project(project_name VERSION "0.4.0")


option(BUILD_EXAMPLES "build examples" OFF)

if(${BUILD_EXAMPLES})
    add_subdirectory(examples)
endif()

