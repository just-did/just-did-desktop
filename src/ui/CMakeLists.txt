add_library(justdid_ui STATIC
    viewmodels/CalendarViewModel.cpp
    viewmodels/TimelineViewModel.cpp
    viewmodels/FloatingInputViewModel.cpp
    viewmodels/StorageViewModel.cpp
    viewmodels/ConnectionViewModel.cpp
    models/CalendarModel.cpp
    models/RecordListModel.cpp
    QRCodeProvider.cpp
    qrcodegen.cpp
)

target_link_libraries(justdid_ui PUBLIC
    justdid_service
    Qt6::Core
    Qt6::Quick
    Qt6::Gui
)

target_include_directories(justdid_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
