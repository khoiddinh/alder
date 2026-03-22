cmake_minimum_required(VERSION 3.20)
project(alder LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Core compiler library
add_library(alder_core STATIC
    src/frontend/lexer/lexer.cpp
    src/frontend/parser/parser.cpp
    src/frontend/parser/operator_precedence.cpp
    src/frontend/semantic/semantic.cpp
    src/frontend/codegen/transpiler.cpp
    src/frontend/diagnostic/diagnostic.cpp
)
target_include_directories(alder_core PUBLIC
    src/frontend/token
    src/frontend/lexer
    src/frontend/ast
    src/frontend/parser
    src/frontend/semantic
    src/frontend/codegen
    src/frontend/diagnostic
)

# Compiler executable
add_executable(alder src/main.cpp)
target_link_libraries(alder PRIVATE alder_core)

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
    tests/test_parser.cpp
)
target_link_libraries(alder_tests PRIVATE alder_core Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(alder_tests)
