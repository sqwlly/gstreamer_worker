#pragma once

#include <cstdint>
#include <string>

namespace gstreamer_worker::pipeline {

struct NetworkTarget {
    std::string host{"127.0.0.1"};
    std::uint16_t port{5000};
};

struct CapturePipelineConfig {
    std::string name{"capture-pipeline"};
    std::string device{"/dev/video0"};
    bool use_test_pattern{false};
    std::string test_pattern{"smpte"};
    std::uint32_t width{1920};
    std::uint32_t height{1080};
    std::uint32_t framerate{60};
    std::uint32_t bitrate{8'000'000};
    bool use_nvenc{true};
    bool use_zero_copy{true};
    bool enable_fec{false};
    std::uint32_t fec_percentage{5};
    std::uint32_t queue_size{4};
    NetworkTarget network{};
};

enum class DecoderBackend { Auto, Nvidia, Software };

struct ViewerPipelineConfig {
    std::string name{"viewer-pipeline"};
    DecoderBackend backend{DecoderBackend::Auto};
    NetworkTarget listen{"0.0.0.0", 5000};
    std::uint32_t latency_ms{32};
    std::string appsink_name{"display_sink"};
    bool request_zero_copy{true};
};

}  // namespace gstreamer_worker::pipeline
