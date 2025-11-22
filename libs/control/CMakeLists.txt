add_library(control
    pipeline_controller.cpp
)

target_include_directories(control
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(control
    PUBLIC
        gstreamer_worker::project_options
        gstreamer_worker::gst
)

add_library(gstreamer_worker::control ALIAS control)

install(TARGETS control
    EXPORT gstreamer_workerTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
