set(CLIENT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(client ${CLIENT_SOURCE_DIR}/main.cpp ${HANGMAN_LIB}/client.h)
install(TARGETS client RUNTIME DESTINATION bin)
