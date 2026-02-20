project(DatabaseServer)

include_directories(${CMAKE_SOURCE_DIR}/DatabaseServer)

aux_source_directory(. DATABASE_SRC)

add_executable(
    DatabaseServer
    ${DATABASE_SRC}
)

target_link_libraries(
    DatabaseServer
    mysqlclient
)