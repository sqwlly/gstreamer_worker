add_executable(config_snapshot
    config_snapshot.cpp
)

target_link_libraries(config_snapshot
    PRIVATE
        gstreamer_worker::project_options
        gstreamer_worker::pipeline
)

add_test(NAME config_snapshot COMMAND config_snapshot --print)
