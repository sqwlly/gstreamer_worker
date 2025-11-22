add_executable(capture_server
    main.cpp
)

target_link_libraries(capture_server
    PRIVATE
        gstreamer_worker::project_options
        gstreamer_worker::gst
        gstreamer_worker::pipeline
        gstreamer_worker::control
)

install(TARGETS capture_server RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
