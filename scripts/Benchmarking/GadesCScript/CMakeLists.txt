cmake_minimum_required(VERSION 3.30)

project(GadesCScript LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ArrayFire REQUIRED)
find_package(Armadillo REQUIRED)
include(FetchContent)
FetchContent_Declare(
  GADES
  GIT_REPOSITORY https://github.com/Sazan1209/GADES-main.git
  GIT_TAG dev
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(GADES)


add_library(deps INTERFACE)
target_link_libraries(deps INTERFACE ${ARMADILLO_LIBRARIES} ArrayFire::af GADES2)
target_include_directories(deps INTERFACE include ${ARMADILLO_INCLUDE_DIRS})

add_executable(GadesCScript main.cpp)
target_link_libraries(GadesCScript deps)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message("Running a Debug build")

  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.16.0
    FIND_PACKAGE_ARGS NAMES GTest
  )
  FetchContent_MakeAvailable(googletest)

  add_executable(article_test tests/test.cpp)
  target_link_libraries(
    article_test
    GTest::gtest_main
    deps
  )
  include(GoogleTest)
  gtest_discover_tests(article_test)
endif()

include(GNUInstallDirs)
install(TARGETS GadesCScript)
