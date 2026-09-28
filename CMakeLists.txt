cmake_minimum_required(VERSION 3.16)
project(compiler CXX)

set(CMAKE_CXX_STANDARD 17)       
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Compiler
        src/lexer/lexer.cpp
        src/main.cpp
        src/error/errorController.cpp
        src/manager/manager.cpp
)
target_compile_options(Compiler PRIVATE -Wall -Wextra)
