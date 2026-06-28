cmake_minimum_required(VERSION 3.10)

project(LDS_APP)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(lds_core
    design_patterns/reactor/src/reactor.cpp
    design_patterns/command/src/ICommand.cpp

    services/mediator/src/InputMediator.cpp

    services/communication_protocols/tcp/src/TCPDriverComm.cpp
    services/communication_protocols/nbd/src/DriverData.cpp
    services/communication_protocols/nbd/src/NBDDriverComm.cpp

    services/local_storage/src/IStorage.cpp
    services/local_storage/src/LocalStorage.cpp
    services/local_storage/src/StorageMinion.cpp
    services/local_storage/src/RAIDManager.cpp

    utilities/threading/thread_pool/src/thread_pool.cpp
    utilities/logger/src/logger.cpp

    plugins/src/dir_monitor.cpp
    plugins/src/pnp.cpp
    plugins/src/soLoader.cpp

    external/Inotify_cpp/inotify/InotifyManager.cpp
    external/Inotify_cpp/inotify/InotifyWatch.cpp
    external/Inotify_cpp/inotify/InotifyEvent.cpp
)

target_include_directories(lds_core PUBLIC
    design_patterns/reactor/include
    design_patterns/command/include
    design_patterns/singleton/include
    design_patterns/observer/include
    design_patterns/factory/include

    services/mediator/include
    services/communication_protocols/tcp/include
    services/communication_protocols/nbd/include
    services/local_storage/include

    utilities/threading/thread_pool/include
    utilities/thread_safe_data_structures/priority_queue/include
    utilities/logger/include

    plugins/include
    external/Inotify_cpp
    external/Inotify_cpp/inotify
)
add_executable(lds_app
    app/LDS.cpp
)

target_link_libraries(lds_app PRIVATE lds_core dl)

add_executable(test_reactor
    test/unit/test_reactor.cpp
)

target_link_libraries(test_reactor PRIVATE lds_core)