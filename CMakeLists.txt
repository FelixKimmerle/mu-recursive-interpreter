cmake_minimum_required(VERSION 3.20)

project(mu-recursive-interpreter
    VERSION 1.0.0
    DESCRIPTION "Interpreter for mu-recursive functions"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_executable(mu-interp
    src/main.cpp
    src/interpreter.cpp
    src/parser.cpp
    src/lexer.cpp
    src/evaluator.cpp
)

if(MSVC)
    target_compile_options(mu-interp PRIVATE /W4)
else()
    target_compile_options(mu-interp PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
endif()

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
