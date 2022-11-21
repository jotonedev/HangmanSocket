set(SERVER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(MARKOV_LIB_DIR ${CMAKE_SOURCE_DIR}/lib/Markov/src)

add_executable(server ${SERVER_SOURCE_DIR}/main.cpp ${HANGMAN_LIB}/server.cpp ${MARKOV_LIB_DIR}/markov.cpp)
install(TARGETS server RUNTIME DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/bin)
