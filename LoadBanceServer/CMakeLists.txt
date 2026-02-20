project(LoadBanceServer)

include_directories(${CMAKE_SOURCE_DIR}/LoadBanceServer)

aux_source_directory(. LOADBANCE_SRC)

add_executable(
    LoadBanceServer
    ${LOADBANCE_SRC}
)

target_link_libraries(
    LoadBanceServer
    EdoyunNet
)