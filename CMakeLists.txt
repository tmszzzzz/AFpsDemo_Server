cmake_minimum_required(VERSION 3.28)
project(Demo0_Server)

set(CMAKE_CXX_STANDARD 17)

set(ASIO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/asio")

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fexec-charset=GBK")

add_library(asio INTERFACE
        src/server/gameplay/ability/task/SeqTask.h
        src/server/gameplay/ability/task/TaskAbility.h)
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
        src/server/gameplay/hero/HeroDefines.h
        src/server/gameplay/hero/HeroCore.cpp
        src/server/gameplay/hero/HeroCore.h
        src/server/kcc/KCC.h
        src/server/kcc/KCC.cpp
        src/server/gameplay/movement/sources/NetworkInputMovementSource.cpp
        src/server/gameplay/movement/sources/NetworkInputMovementSource.h
        src/server/gameplay/ability/AbilitySlot.h
        src/server/gameplay/ability/AbilityContext.h
        src/server/gameplay/ability/AbilityBase.h
        src/server/gameplay/ability/task/TaskStatus.h
        src/server/gameplay/ability/task/AbilityTask.h
        src/server/gameplay/ability/task/WaitTask.h
        src/server/gameplay/ability/task/InputGateTask.h
        src/server/gameplay/ability/task/ChargeHoldTask.h
        src/server/gameplay/ability/task/DashTask.h
        src/server/gameplay/ability/task/EmitEventTask.h
        src/server/gameplay/ability/AbilityArbiter.h
        src/server/gameplay/ability/AbilityArbiter.cpp
        src/server/gameplay/ability/AbilitySet.cpp
        src/server/gameplay/ability/AbilitySet.h
        src/server/gameplay/ability/abilities/DashAbility.cpp
        src/server/gameplay/ability/abilities/DashAbility.h
        src/server/gameplay/hero/HeroEntity.cpp
        src/server/gameplay/hero/HeroEntity.h
        src/server/gameplay/movement/sources/DashMovementSource.cpp
        src/server/gameplay/movement/sources/DashMovementSource.h
        src/server/gameplay/ability/AbilityResources.h
)

target_link_libraries(Demo0_Server
        PRIVATE
        asio
)