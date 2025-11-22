#include <algorithm>
#include <cctype>
#include <csignal>
#include <cstdlib>
#include <cstdint>
#include <exception>
#include <iostream>
#include <string>

#include <gst/app/gstappsink.h>
#include <gst/gst.h>
#if defined(G_OS_UNIX)
#include <glib-unix.h>
#include <unistd.h>
#endif

#include "gstreamer_worker/control/pipeline_controller.hpp"
#include "gstreamer_worker/pipeline/config.hpp"
#include "gstreamer_worker/pipeline/viewer_pipeline.hpp"
#include "gstreamer_worker/zerocopy/buffer_exporter.hpp"

using gstreamer_worker::control::PipelineController;
using gstreamer_worker::pipeline::DecoderBackend;
using gstreamer_worker::pipeline::ViewerPipelineConfig;
using gstreamer_worker::pipeline::make_viewer_pipeline;
using gstreamer_worker::zerocopy::BufferExporter;
using gstreamer_worker::zerocopy::ExportPacket;

namespace {

struct Options {
    ViewerPipelineConfig config{};
    bool verbose{true};
};

void print_usage(const char* program) {
    std::cout << "Usage: " << program
              << " [--listen 0.0.0.0] [--port 5000] [--latency 32]\n"
              << "             [--backend auto|nvidia|software] [--appsink-name display_sink]\n"
              << "             [--no-zero-copy]\n";
}

DecoderBackend to_backend(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (value == "nvidia" || value == "nv" || value == "cuda") {
        return DecoderBackend::Nvidia;
    }
    if (value == "software" || value == "sw") {
        return DecoderBackend::Software;
    }
    return DecoderBackend::Auto;
}

Options parse_args(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto require_value = [&](const char* flag) -> std::string {
            if (i + 1 >= argc) {
                throw std::invalid_argument(std::string(flag) + " requires a value");
            }
            return std::string(argv[++i]);
        };
        if (arg == "--listen") {
            options.config.listen.host = require_value("--listen");
        } else if (arg == "--port") {
            options.config.listen.port = static_cast<std::uint16_t>(std::stoul(require_value("--port")));
        } else if (arg == "--latency") {
            options.config.latency_ms = static_cast<std::uint32_t>(std::stoul(require_value("--latency")));
        } else if (arg == "--backend") {
            options.config.backend = to_backend(require_value("--backend"));
        } else if (arg == "--appsink-name") {
            options.config.appsink_name = require_value("--appsink-name");
        } else if (arg == "--no-zero-copy") {
            options.config.request_zero_copy = false;
        } else if (arg == "--quiet") {
            options.verbose = false;
        } else if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            std::exit(0);
        } else {
            throw std::invalid_argument("Unknown argument: " + arg);
        }
    }
    return options;
}

class SampleConsumer {
  public:
    explicit SampleConsumer(bool verbose)
        : exporter_([verbose](const ExportPacket& packet) {
              if (verbose) {
                  gchar* caps_str = packet.caps ? gst_caps_to_string(packet.caps) : nullptr;
                  g_print("Frame %llu fd=%d caps=%s\n", static_cast<unsigned long long>(packet.metadata.frame_id),
                          packet.dma_fd, caps_str ? caps_str : "<unknown>");
                  g_free(caps_str);
              }
#if defined(G_OS_UNIX)
              if (packet.dma_fd >= 0) {
                  close(packet.dma_fd);
              }
#endif
          }) {}

    GstFlowReturn on_new_sample(GstAppSink* sink) {
        GstSample* sample = gst_app_sink_pull_sample(sink);
        if (!sample) {
            return GST_FLOW_ERROR;
        }
        const bool ok = exporter_.export_sample(sample);
        gst_sample_unref(sample);
        return ok ? GST_FLOW_OK : GST_FLOW_ERROR;
    }

  private:
    BufferExporter exporter_;
};

GstFlowReturn on_new_sample_proxy(GstAppSink* sink, gpointer user_data) {
    auto* consumer = static_cast<SampleConsumer*>(user_data);
    if (!consumer) {
        return GST_FLOW_ERROR;
    }
    return consumer->on_new_sample(sink);
}

#if defined(G_OS_UNIX)
gboolean handle_signal(gpointer controller_ptr) {
    auto* controller = static_cast<PipelineController*>(controller_ptr);
    if (controller) {
        controller->request_stop();
    }
    return G_SOURCE_REMOVE;
}
#endif

}  // namespace

int main(int argc, char** argv) {
    gst_init(&argc, &argv);

    Options options;
    try {
        options = parse_args(argc, argv);
    } catch (const std::exception& ex) {
        std::cerr << "Argument error: " << ex.what() << "\n";
        print_usage(argv[0]);
        return 1;
    }

    GError* error = nullptr;
    GstElement* pipeline = nullptr;

    try {
        pipeline = make_viewer_pipeline(options.config, &error);
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << "\n";
        return 1;
    }

    if (!pipeline) {
        std::cerr << "Failed to build pipeline: " << (error ? error->message : "unknown error") << "\n";
        if (error) {
            g_error_free(error);
        }
        return 1;
    }

    GstElement* sink = gst_bin_get_by_name(GST_BIN(pipeline), options.config.appsink_name.c_str());
    if (!sink) {
        std::cerr << "Unable to find appsink named " << options.config.appsink_name << "\n";
        gst_object_unref(pipeline);
        return 1;
    }

    SampleConsumer consumer(options.verbose);
    GstAppSink* app_sink = GST_APP_SINK(sink);
    gst_app_sink_set_emit_signals(app_sink, TRUE);
    gst_app_sink_set_max_buffers(app_sink, 1);
    gst_app_sink_set_drop(app_sink, TRUE);
    g_signal_connect(app_sink, "new-sample", G_CALLBACK(on_new_sample_proxy), &consumer);

    PipelineController controller;
    controller.set_pipeline(pipeline);

#if defined(G_OS_UNIX)
    g_unix_signal_add(SIGINT, handle_signal, &controller);
    g_unix_signal_add(SIGTERM, handle_signal, &controller);
#endif

    if (!controller.play()) {
        std::cerr << "Unable to transition pipeline to PLAYING." << std::endl;
        gst_object_unref(sink);
        gst_object_unref(pipeline);
        return 1;
    }

    if (options.verbose) {
        std::cout << "Viewer listening on " << options.config.listen.host << ":" << options.config.listen.port
                  << std::endl;
    }

    controller.run();

    controller.stop();
    gst_object_unref(sink);
    gst_object_unref(pipeline);
    return 0;
}
