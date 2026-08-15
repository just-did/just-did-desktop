include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_subdirectory(common)
add_subdirectory(core)
add_subdirectory(service)
add_subdirectory(ui)

add_executable(JustDid WIN32
    main.cpp
    app.rc
    resources.qrc
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

# Copy QML files to build output after each build
add_custom_command(TARGET JustDid POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/src/ui/qml"
        "$<TARGET_FILE_DIR:JustDid>/qml"
    COMMENT "Copying QML files to output directory"
)
