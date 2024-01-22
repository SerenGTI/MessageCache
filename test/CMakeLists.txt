cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE ON)

include(cmake/gtest.cmake)

set(test_cases_o_files)

function (new_test source name)
  add_executable(${name} ${source})
  target_link_libraries(${name} LINK_PUBLIC
    ${CMAKE_THREAD_LIBS_INIT}
    gtest
    gtest_main)

  target_include_directories(${name} PUBLIC
    gtest
    gtest_main
    spdlog
    ${ASIO_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
  )

  set_sanitizers(${name})

  include(GoogleTest)
  gtest_discover_tests(${name})

  target_compile_options(${name} PRIVATE --coverage -g -O0)
  target_link_options(${name} PRIVATE --coverage)


  add_dependencies(${name} gtest-project)
  add_dependencies(${name} asio-project)
endfunction()


new_test(ring_buffer_test.cpp ring_buffer_test)
list(APPEND test_cases_o_files ${CMAKE_BINARY_DIR}/test/CMakeFiles/ring_buffer_test.dir/*.o) # why cant this happen in the new_test function?






# create coverage folder, execute tests
add_custom_target(gcov
  COMMAND mkdir -p coverage
  COMMAND ${CMAKE_MAKE_PROGRAM} test
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
# mark coverage folder to be cleaned
set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_BINARY_DIR}/coverage)
# invoke gcov
add_custom_command(TARGET gcov
  # COMMAND echo "list: ${test_cases_o_files}"
  COMMAND gcov ${test_cases_o_files} > /dev/null
  COMMAND gcovr --root ${CMAKE_SOURCE_DIR} --filter ${CMAKE_SOURCE_DIR}/include/*
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/coverage
)

