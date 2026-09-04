add_executable(justdid_core_tests
    test_core.cpp
)

target_link_libraries(justdid_core_tests PRIVATE
    justdid_core
    Qt6::Core
)

target_include_directories(justdid_core_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

add_test(NAME justdid_core_tests COMMAND justdid_core_tests)
