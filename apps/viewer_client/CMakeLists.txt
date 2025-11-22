add_executable(viewer_client
    main.cpp
)

target_link_libraries(viewer_client
    PRIVATE
        gstreamer_worker::project_options
        gstreamer_worker::gst
        gstreamer_worker::pipeline
        gstreamer_worker::control
        gstreamer_worker::zerocopy
)

install(TARGETS viewer_client RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
