cmake_minimum_required(VERSION 3.28)
project(translator C)

set(CMAKE_C_STANDARD 11)

include_directories(include/)

add_executable(translator
        src/main.c
        src/file_lines.c
        src/translator.c
)
