cmake_minimum_required(VERSION 3.10)
project(banking_project)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Build tests" OFF)
option(COLLECT_COVERAGE "Collect coverage" OFF)

add_subdirectory(banking)

if(BUILD_TESTS)
  enable_testing()
  
  # Скачиваем Google Test
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
  )
  FetchContent_MakeAvailable(googletest)

  file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")
  add_executable(check ${TEST_SOURCES})
  target_link_libraries(check banking gtest_main gmock_main)

  if(COLLECT_COVERAGE)
    target_compile_options(check PRIVATE -O0 -g --coverage)
    target_link_options(check PRIVATE --coverage)
  endif()

  add_test(NAME check COMMAND check)
endif()
