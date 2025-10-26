cmake_minimum_required(VERSION 3.20)
project(alder LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Your library / sources
add_library(alder STATIC
    src/frontend/lexer.cpp
    # add other sources…
)
target_include_directories(alder PUBLIC include)

# ---- Catch2 (v3) via FetchContent ----
include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.6.0
)
FetchContent_MakeAvailable(Catch2)

# ---- Tests ----
add_executable(alder_tests
    tests/test_lexer.cpp
)
target_link_libraries(alder_tests PRIVATE alder Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(alder_tests)
