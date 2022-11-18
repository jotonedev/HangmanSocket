cmake_minimum_required(VERSION 3.16)
project(hangman_game CXX)

enable_language(C CXX)

set(CMAKE_C_STANDARD 20)
set(CMAKE_CXX_STANDARD 20)
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/lib)
set(HANGMAN_LIB ${INCLUDE_DIR}/Hangman)

include_directories(${INCLUDE_DIR})

add_subdirectory(client)
add_subdirectory(server)
