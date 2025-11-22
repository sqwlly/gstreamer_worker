#include <iostream>
#include <string>

#include "gstreamer_worker/pipeline/capture_pipeline.hpp"
#include "gstreamer_worker/pipeline/viewer_pipeline.hpp"

int main(int argc, char** argv) {
    gstreamer_worker::pipeline::CapturePipelineConfig capture;
    capture.device = "/dev/video-test";
    capture.use_nvenc = false;
    capture.enable_fec = true;
    capture.fec_percentage = 10;

    gstreamer_worker::pipeline::ViewerPipelineConfig viewer;
    viewer.backend = gstreamer_worker::pipeline::DecoderBackend::Software;
    viewer.request_zero_copy = false;
    viewer.appsink_name = "test_sink";

    const auto capture_line = gstreamer_worker::pipeline::build_capture_launch(capture);
    const auto viewer_line = gstreamer_worker::pipeline::build_viewer_launch(viewer);

    if (argc > 1 && std::string(argv[1]) == "--print") {
        std::cout << "Capture: " << capture_line << "\nViewer:  " << viewer_line << "\n";
    }

    return (capture_line.empty() || viewer_line.empty()) ? 1 : 0;
}
