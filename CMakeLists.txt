cmake_minimum_required(VERSION 3.10)

project(LDS_APP)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(lds_core
    design_patterns/reactor/src/reactor.cpp
    design_patterns/command/src/ICommand.cpp
    design_patterns/watchdog_thread/src/WatchdogThread.cpp

    services/mediator/src/InputMediator.cpp
    services/watchdog_monitor/WatchdogMonitor.cpp

    services/communication_protocols/tcp/src/TCPDriverComm.cpp
    services/communication_protocols/nbd/src/DriverData.cpp
    services/communication_protocols/nbd/src/NBDDriverComm.cpp

    services/local_storage/src/IStorage.cpp
    services/local_storage/src/LocalStorage.cpp
    services/local_storage/src/StorageMinionMemory.cpp
    services/local_storage/src/StorageMinionFile.cpp
    services/local_storage/src/MemoryMetadataStore.cpp
    services/local_storage/src/FileMetadataStore.cpp
    services/local_storage/src/StorageFactory.cpp
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
    design_patterns/watchdog_thread/include
    design_patterns/singleton/include
    design_patterns/observer/include
    design_patterns/factory/include

    services/mediator/include
    services/watchdog_monitor
    services/communication_protocols/tcp/include
    services/communication_protocols/nbd/include
    services/local_storage/include

    utilities/threading/thread_pool/include
    utilities/thread_safe_data_structures/priority_queue/include
    utilities/logger/include

    plugins/include
    external/Inotify_cpp
    external/Inotify_cpp/inotify
    watchdog/include
    scheduler/include
)
add_executable(lds_app
    app/LDS.cpp
)

target_link_libraries(lds_app PRIVATE lds_core dl pthread)

# Build watchdog executable for process monitoring
add_executable(lds_watchdog
    scheduler/src/scheduler.c
    scheduler/src/uid.c
    scheduler/src/utils.c
    scheduler/src/task.c
    scheduler/src/heap_pq.c
    scheduler/src/vector.c
    watchdog/src/watchdog.c
)

target_compile_options(lds_watchdog PRIVATE -D_POSIX_C_SOURCE=200809L)
target_include_directories(lds_watchdog PRIVATE watchdog/include scheduler/include)
target_link_libraries(lds_watchdog PRIVATE pthread)
target_include_directories(lds_app PRIVATE /home/daniel/git/watchdog/include /home/daniel/git/scheduler/include)

add_library(watchdog_lib OBJECT
    scheduler/src/scheduler.c
    scheduler/src/uid.c
    scheduler/src/utils.c
    scheduler/src/task.c
    scheduler/src/heap_pq.c
    scheduler/src/vector.c
    watchdog/src/wd.c
)
target_compile_options(watchdog_lib PRIVATE -D_POSIX_C_SOURCE=200809L)
target_include_directories(watchdog_lib PRIVATE watchdog/include scheduler/include)

target_link_libraries(lds_app PRIVATE watchdog_lib pthread)

add_executable(test_reactor
    test/unit/test_reactor.cpp
)

target_link_libraries(test_reactor PRIVATE lds_core)