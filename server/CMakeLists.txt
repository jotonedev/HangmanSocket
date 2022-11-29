set(SERVER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

IF (WIN32)
    set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/data/res.rc")
    add_executable(server WIN32 ${SERVER_SOURCE_DIR}/main.cpp ${HANGMAN_LIB}/server.cpp ${APP_ICON_RESOURCE_WINDOWS})
else()
    add_executable(server ${SERVER_SOURCE_DIR}/main.cpp ${HANGMAN_LIB}/server.cpp)
endif()

install(TARGETS server RUNTIME DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/bin)
