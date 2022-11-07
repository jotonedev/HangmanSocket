set (SERVER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(server ${SERVER_SOURCE_DIR}/main.cpp ${HANGMAN_LIB}/server.h)
install(TARGETS server RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/bin)
