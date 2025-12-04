cmake_minimum_required(VERSION 3.28)
project(Demo0_Server)

set(CMAKE_CXX_STANDARD 17)

set(ASIO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/asio")

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fexec-charset=GBK")

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
        src/net/NetTransport.cpp
        src/server/collision/CollisionWorld.h
        src/server/collision/CollisionWorld.cpp
        src/server/gameplay/movement/sources/IMovementSource.h
        src/utils/Utils.h
        src/server/gameplay/movement/core/PlayerState.h
        src/server/gameplay/movement/core/MovementCommand.h
        src/server/gameplay/movement/core/MovementSourceCollection.cpp
        src/server/gameplay/movement/core/MovementSourceCollection.h
        src/server/gameplay/movement/core/CharacterMotor.cpp
        src/server/gameplay/movement/core/CharacterMotor.h
        src/server/gameplay/heroes/HeroDefines.h
        src/server/gameplay/heroes/HeroCore.cpp
        src/server/gameplay/heroes/HeroCore.h
        src/server/kcc/KCC.h
        src/server/kcc/KCC.cpp
)

target_link_libraries(Demo0_Server
        PRIVATE
        asio
)