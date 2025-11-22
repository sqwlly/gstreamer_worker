add_library(zerocopy
    buffer_exporter.cpp
    frame_meta.cpp
)

target_include_directories(zerocopy
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(zerocopy
    PUBLIC
        gstreamer_worker::project_options
        gstreamer_worker::gst
)

add_library(gstreamer_worker::zerocopy ALIAS zerocopy)

install(TARGETS zerocopy
    EXPORT gstreamer_workerTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
