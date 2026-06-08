cmake_minimum_required(VERSION 3.10)
project(banking)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(banking STATIC Account.cpp Transaction.cpp)
target_include_directories(banking PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(COLLECT_COVERAGE)
  target_compile_options(banking PRIVATE -O0 -g --coverage)
  target_link_options(banking PRIVATE --coverage)
endif()
