project(RtmpServer)

include_directories(${CMAKE_SOURCE_DIR}/RtmpServer)

aux_source_directory(. RTMP_SRC)

add_executable(
    RtmpServer
    ${RTMP_SRC}
)

target_link_libraries(
    RtmpServer
    EdoyunNet
)