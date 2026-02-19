project(SigServer)

include_directories(${CMAKE_SOURCE_DIR}/SigServer)

aux_source_directory(. SIG_SRC)

add_executable(
    SigServer
    ${SIG_SRC}
)

target_link_libraries(
    SigServer
    EdoyunNet
)