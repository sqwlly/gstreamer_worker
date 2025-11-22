#include "gstreamer_worker/pipeline/capture_pipeline.hpp"

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

void append_encoder(std::ostringstream& stream, const CapturePipelineConfig& config) {
    if (config.use_nvenc) {
        stream << " ! nvvidconv name=nvconv";
        if (config.use_zero_copy) {
            stream << " nvbuf-memory-type=3";
        }
        stream << " ! video/x-raw(memory:NVMM),format=NV12";
        stream << " ! nvv4l2h264enc control-rate=1 bitrate=" << config.bitrate
               << " iframeinterval=15 insert-sps-pps=true EnableTwopassCBR=false "
                  "OutputIniCtrl=0";
    } else {
        stream << " ! x264enc tune=zerolatency speed-preset=superfast bitrate=" << config.bitrate / 1000
               << " key-int-max=30 sliced-threads=true bframes=0";
    }
}

void append_transport(std::ostringstream& stream, const CapturePipelineConfig& config) {
    stream << " ! h264parse disable-passthrough=true config-interval=1";
    stream << " ! rtph264pay pt=96 config-interval=1";
    if (config.enable_fec) {
        stream << " ! rtpulpfecenc percentage=" << config.fec_percentage;
    }
    stream << " ! queue max-size-time=0 max-size-buffers=4";
    stream << " ! udpsink host=" << quote(config.network.host)
           << " port=" << config.network.port << " sync=false async=false";
}

}  // namespace

std::string build_capture_launch(const CapturePipelineConfig& config) {
    std::ostringstream stream;
    const char* desired_format = config.use_nvenc ? "NV12" : "I420";

    if (config.use_test_pattern) {
        stream << "videotestsrc name=source pattern=" << quote(config.test_pattern)
               << " is-live=true";
        stream << " ! videoconvert";
    } else {
        stream << "v4l2src name=source device=" << quote(config.device);
        if (config.use_zero_copy) {
            stream << " io-mode=dmabuf";
        }
        if (!config.use_nvenc) {
            stream << " ! videoconvert";
        }
    }
    stream << " ! video/x-raw,format=" << desired_format << ",width=" << config.width
           << ",height=" << config.height
           << ",framerate=" << config.framerate << "/1";
    stream << " ! queue max-size-buffers=" << config.queue_size << " leaky=downstream";

    append_encoder(stream, config);
    append_transport(stream, config);
    return stream.str();
}

GstElement* make_capture_pipeline(const CapturePipelineConfig& config, GError** error) {
    const auto description = build_capture_launch(config);
    g_print("Pipeline description: %s\n", description.c_str());
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
        throw std::runtime_error("Failed to create capture pipeline: " + reason);
    }
    return pipeline;
}

}  // namespace gstreamer_worker::pipeline
