#include "gstreamer_worker/pipeline/viewer_pipeline.hpp"

#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "gstreamer_worker/pipeline/config.hpp"

namespace gstreamer_worker::pipeline {
namespace {

std::string quote(std::string_view text) {
    if (text.find_first_of(" \"") == std::string_view::npos) {
        return std::string{text};
    }
    std::string result = "\"";
    for (char ch : text) {
        if (ch == '\"' || ch == '\\') {
            result.push_back('\\');
        }
        result.push_back(ch);
    }
    result.push_back('\"');
    return result;
}

std::string decoder_branch(const ViewerPipelineConfig& config) {
    switch (config.backend) {
        case DecoderBackend::Nvidia:
            return "nvv4l2decoder enable-max-performance=1 enable-low-latency=1 ! nvvidconv";
        case DecoderBackend::Software:
            return "avdec_h264 skip-frame=0 ! videoconvert";
        case DecoderBackend::Auto:
        default:
            return "decodebin name=decoder";
    }
}

}  // namespace

std::string build_viewer_launch(const ViewerPipelineConfig& config) {
    std::ostringstream stream;
    stream << "udpsrc address=" << quote(config.listen.host)
           << " port=" << config.listen.port
           << " caps=\"application/x-rtp,media=video,encoding-name=H264,payload=96\"";

    stream << " ! queue max-size-buffers=32";
    stream << " ! rtpjitterbuffer latency=" << config.latency_ms << " do-lost=true";
    stream << " ! rtph264depay";
    stream << " ! h264parse";
    stream << " ! " << decoder_branch(config);
    stream << " ! queue max-size-buffers=4 leaky=downstream";
    if (config.request_zero_copy && config.backend == DecoderBackend::Nvidia) {
        stream << " ! video/x-raw(memory:NVMM),format=NV12";
    } else {
        stream << " ! video/x-raw,format=NV12";
    }
    stream << " ! appsink name=" << config.appsink_name
           << " drop=true max-buffers=1 emit-signals=true sync=false";
    return stream.str();
}

GstElement* make_viewer_pipeline(const ViewerPipelineConfig& config, GError** error) {
    const auto description = build_viewer_launch(config);
    GError* local_error = nullptr;
    GstElement* pipeline = gst_parse_launch(description.c_str(), &local_error);
    if (!pipeline) {
        if (error) {
            *error = local_error;
            return nullptr;
        }
        std::string reason = "unknown";
        if (local_error) {
            reason = local_error->message;
            g_error_free(local_error);
        }
        throw std::runtime_error("Failed to create viewer pipeline: " + reason);
    }
    return pipeline;
}

}  // namespace gstreamer_worker::pipeline
