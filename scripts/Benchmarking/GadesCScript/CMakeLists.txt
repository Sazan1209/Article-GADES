cmake_minimum_required(VERSION 3.30)

project(GadesCScript LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ArrayFire REQUIRED)
find_package(Armadillo REQUIRED)

add_library(deps INTERFACE)
target_link_libraries(deps INTERFACE ${ARMADILLO_LIBRARIES} ArrayFire::af)
target_include_directories(deps INTERFACE include ${ARMADILLO_INCLUDE_DIRS})

add_executable(GadesCScript main.cpp)
target_link_libraries(GadesCScript deps)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message("Running a Debug build")
  include(FetchContent)
  FetchContent_Declare(
    GTest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.16.0
  )
  FetchContent_MakeAvailable(GTest)

  add_executable(test tests/test.cpp)
  target_link_libraries(
    test
    GTest::gtest_main
    deps
  )
  include(GoogleTest)
  gtest_discover_tests(test)
endif()

include(GNUInstallDirs)
install(TARGETS GadesCScript)
