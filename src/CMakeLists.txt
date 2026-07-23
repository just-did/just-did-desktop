include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_subdirectory(common)
add_subdirectory(core)
add_subdirectory(service)
add_subdirectory(ui)

add_executable(JustDid
    main.cpp
)

target_link_libraries(JustDid PRIVATE
    justdid_ui
    justdid_service
    justdid_core
    justdid_common
    Qt6::Core
    Qt6::Quick
    Qt6::Network
    Qt6::Sql
    Qt6::HttpServer
    yaml-cpp::yaml-cpp
)
