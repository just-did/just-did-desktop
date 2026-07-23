add_library(justdid_core STATIC
    AppCore.cpp
    ConfigManager.cpp
    LogManager.cpp
    DatabaseManager.cpp
    FileManager.cpp
    DataManager.cpp
    ConnectionStateMachine.cpp
)

target_link_libraries(justdid_core PUBLIC
    justdid_common
    Qt6::Core
    Qt6::Sql
    yaml-cpp::yaml-cpp
)

target_include_directories(justdid_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
