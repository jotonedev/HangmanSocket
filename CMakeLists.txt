cmake_minimum_required(VERSION 3.23)
set(CMAKE_CXX_COMPILER "g++.exe")
set(CMAKE_C_COMPILER "gcc.exe")

project(hangman_game CXX)

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)

set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/lib)
set(HANGMAN_LIB ${INCLUDE_DIR}/Hangman)

include_directories(${INCLUDE_DIR})

add_subdirectory(client)
add_subdirectory(server)
