add_library(justdid_common INTERFACE)
target_link_libraries(justdid_common INTERFACE Qt6::Core)
target_include_directories(justdid_common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
