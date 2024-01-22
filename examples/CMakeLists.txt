cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(main main.cpp)

add_dependencies(main asio-project)
target_include_directories(main PRIVATE ${CMAKE_SOURCE_DIR}/include
                                        ${ASIO_INCLUDE_DIR})

target_compile_options(
  main
  PUBLIC
    -Wall
    -Werror=format-security
    -pipe
    -march=native
    $<$<CONFIG:DEBUG>:-O0>
    $<$<CONFIG:DEBUG>:-g3>
    $<$<CXX_COMPILER_ID:Clang>:-ferror-limit=2>
    # $<$<CXX_COMPILER_ID:GNU>:-fanalyzer>
    $<$<CONFIG:DEBUG>:-fno-optimize-sibling-calls>
    $<$<CONFIG:DEBUG>:-fno-omit-frame-pointer>
    $<$<CONFIG:DEBUG>:-Wfloat-equal>
    $<$<CONFIG:DEBUG>:-Wall>
    $<$<CONFIG:DEBUG>:-Wpointer-arith>
    $<$<CONFIG:DEBUG>:-Wformat-nonliteral>
    $<$<CONFIG:DEBUG>:-Winit-self>
    $<$<CONFIG:DEBUG>:-ggdb>
    $<$<CONFIG:RELEASE>:-O3>
    # $<$<CONFIG:RELEASE>:-march=native>
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:RELEASE>>:-fipa-pta>
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:RELEASE>>:-flto=auto>
    $<$<AND:$<NOT:$<CXX_COMPILER_ID:GNU>>,$<CONFIG:RELEASE>>:-flto>
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:RELEASE>>:-fdevirtualize-at-ltrans>
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:RELEASE>>:-fdevirtualize-speculatively>
)

set_sanitizers(main)
