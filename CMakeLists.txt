cmake_minimum_required(VERSION 3.20.0)

project(MyProject LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 输出路径
# 静态库的输出路径
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
# 动态库的输出路径
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
# 可执行文件的输出路径
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# （可选）保留你原来的变量，但实际上 CMAKE_RUNTIME_OUTPUT_DIRECTORY 已经生效
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# ===== ASan 开关（默认关闭）=====
option(ENABLE_ASAN "Enable AddressSanitizer (ASan)" ON)

if(ENABLE_ASAN)
    message(STATUS "AddressSanitizer ENABLED")

    # 没指定构建类型时，默认给 Debug（避免 Release 下优化影响定位）
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    endif()

    # 仅对 GCC/Clang 生效
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # 编译参数（对所有子目录生效）
        add_compile_options(
            -fsanitize=address
            -fno-omit-frame-pointer
            -g
            -O0
        )

        # 链接参数（对所有子目录生效）
        add_link_options(
            -fsanitize=address
        )
    else()
        message(WARNING "ENABLE_ASAN is ON but compiler is not GCC/Clang; ASan flags not applied.")
    endif()
endif()

# ===== 子工程 =====
add_subdirectory(${PROJECT_SOURCE_DIR}/EdoyunNet)
add_subdirectory(${PROJECT_SOURCE_DIR}/RtmpServer)
add_subdirectory(${PROJECT_SOURCE_DIR}/SigServer)
add_subdirectory(${PROJECT_SOURCE_DIR}/LoadBanceServer)
add_subdirectory(${PROJECT_SOURCE_DIR}/LoginServer)


