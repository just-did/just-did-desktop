cmake_minimum_required(VERSION 3.20)
project(QRCodeTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui)

set(QRCGEN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../src/ui)

add_executable(QRCodeTest
    main.cpp
    ${QRCGEN_DIR}/qrcodegen.cpp
)

target_include_directories(QRCodeTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${QRCGEN_DIR}
)

target_link_libraries(QRCodeTest PRIVATE
    Qt6::Core
    Qt6::Gui
)
