project(LoginServer)

include_directories(${CMAKE_SOURCE_DIR}/LoginServer)

aux_source_directory(. LOGIN_SRC)

add_library(
    LoginServer
    ${LOGIN_SRC}
)

target_link_libraries(
    LoginServer
    mysqlclient
    EdoyunNet
)