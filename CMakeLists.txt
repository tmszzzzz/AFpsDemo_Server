cmake_minimum_required(VERSION 3.28)
project(Demo0_Server)

set(CMAKE_CXX_STANDARD 17)

set(ASIO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/asio")

add_library(asio INTERFACE)
target_include_directories(asio INTERFACE
        "${ASIO_ROOT}/include"
)

target_compile_definitions(asio INTERFACE
        ASIO_STANDALONE         # 告诉 asio 使用独立模式（不是 boost::asio）
)

if (WIN32)
    # MinGW/VS 下都用这两个
    target_link_libraries(asio INTERFACE ws2_32 mswsock)
elseif (UNIX AND NOT APPLE)
    target_link_libraries(asio INTERFACE pthread)
endif()

add_executable(Demo0_Server src/main.cpp
        src/protocol/Messages.h
        src/protocol/Serializer.h
        src/protocol/Serializer.cpp
        src/server/GameServer.h
        src/server/GameServer.cpp
        src/net/NetTransport.h
        src/net/NetTransport.cpp)

target_link_libraries(Demo0_Server
        PRIVATE
        asio
)