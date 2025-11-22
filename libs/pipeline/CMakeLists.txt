add_library(pipeline
    capture_pipeline.cpp
    viewer_pipeline.cpp
)

target_include_directories(pipeline
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(pipeline
    PUBLIC
        gstreamer_worker::project_options
        gstreamer_worker::gst
        gstreamer_worker::zerocopy
)

add_library(gstreamer_worker::pipeline ALIAS pipeline)

install(TARGETS pipeline
    EXPORT gstreamer_workerTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
