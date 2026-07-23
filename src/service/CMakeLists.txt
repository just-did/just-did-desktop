add_library(justdid_service STATIC
    ReportService.cpp
    SyncService.cpp
    HttpServer.cpp
)

target_link_libraries(justdid_service PUBLIC
    justdid_core
    Qt6::Core
    Qt6::Network
    Qt6::HttpServer
)

target_include_directories(justdid_service PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
