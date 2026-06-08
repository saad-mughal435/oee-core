cmake_minimum_required(VERSION 3.16)
project(oee_core VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# ---- library --------------------------------------------------------------
add_library(oee_core src/oee.cpp)
target_include_directories(oee_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(MSVC)
  target_compile_options(oee_core PRIVATE /W4)
else()
  target_compile_options(oee_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ---- CLI ------------------------------------------------------------------
add_executable(oee apps/oee_cli.cpp)
target_link_libraries(oee PRIVATE oee_core)

# ---- tests ----------------------------------------------------------------
option(OEE_BUILD_TESTS "Build unit tests" ON)
if(OEE_BUILD_TESTS)
  include(FetchContent)
  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.2
  )
  FetchContent_MakeAvailable(Catch2)

  add_executable(oee_tests tests/oee_tests.cpp)
  target_link_libraries(oee_tests PRIVATE oee_core Catch2::Catch2WithMain)

  enable_testing()
  add_test(NAME unit COMMAND oee_tests)
endif()
