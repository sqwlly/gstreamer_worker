#include <csignal>
#include <cstdlib>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

#include <gst/gst.h>
#if defined(G_OS_UNIX)
#include <glib-unix.h>
#endif

#include "gstreamer_worker/control/pipeline_controller.hpp"
#include "gstreamer_worker/pipeline/capture_pipeline.hpp"
#include "gstreamer_worker/pipeline/config.hpp"
#include "gstreamer_worker/zerocopy/frame_meta.hpp"

using gstreamer_worker::control::PipelineController;
using gstreamer_worker::pipeline::CapturePipelineConfig;
using gstreamer_worker::pipeline::make_capture_pipeline;
using gstreamer_worker::zerocopy::FrameMetadata;
using gstreamer_worker::zerocopy::add_frame_meta;
using gstreamer_worker::zerocopy::make_frame_metadata;

namespace {

struct Options {
    CapturePipelineConfig config{};
    std::string sensor_id{"cam0"};
};

void print_usage(const char* program) {
    std::cout << "Usage: " << program
              << " [--device /dev/video0] [--host 192.168.1.10] [--port 5000]\n"
              << "             [--width 1920] [--height 1080] [--fps 60] [--bitrate 8000000]\n"
              << "             [--no-nvenc] [--no-zero-copy] [--fec <percentage>] [--sensor-id cam0]\n"
              << "             [--use-test-pattern] [--test-pattern smpte]\n";
}

std::uint32_t parse_u32(const std::string& value) {
    return static_cast<std::uint32_t>(std::stoul(value));
}

Options parse_args(int argc, char** argv) {
    Options options;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto require_value = [&](const char* flag) -> std::string {
            if (i + 1 >= argc) {
                throw std::invalid_argument(std::string(flag) + " requires a value");
            }
            return std::string(argv[++i]);
        };

        if (arg == "--device") {
            options.config.device = require_value("--device");
        } else if (arg == "--host") {
            options.config.network.host = require_value("--host");
        } else if (arg == "--port") {
            options.config.network.port = static_cast<std::uint16_t>(std::stoul(require_value("--port")));
        } else if (arg == "--width") {
            options.config.width = parse_u32(require_value("--width"));
        } else if (arg == "--height") {
            options.config.height = parse_u32(require_value("--height"));
        } else if (arg == "--fps") {
            options.config.framerate = parse_u32(require_value("--fps"));
        } else if (arg == "--bitrate") {
            options.config.bitrate = parse_u32(require_value("--bitrate"));
        } else if (arg == "--sensor-id") {
            options.sensor_id = require_value("--sensor-id");
        } else if (arg == "--no-nvenc") {
            options.config.use_nvenc = false;
        } else if (arg == "--no-zero-copy") {
            options.config.use_zero_copy = false;
        } else if (arg == "--fec") {
            options.config.enable_fec = true;
            options.config.fec_percentage = parse_u32(require_value("--fec"));
        } else if (arg == "--queue-size") {
            options.config.queue_size = parse_u32(require_value("--queue-size"));
        } else if (arg == "--use-test-pattern") {
            options.config.use_test_pattern = true;
        } else if (arg == "--test-pattern") {
            options.config.use_test_pattern = true;
            options.config.test_pattern = require_value("--test-pattern");
        } else if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            std::exit(0);
        } else {
            throw std::invalid_argument("Unknown argument: " + arg);
        }
    }

    return options;
}

struct MetadataProbeData {
    std::string sensor_id;
    guint64 frame_counter{0};
};

GstPadProbeReturn metadata_probe(GstPad* /*pad*/, GstPadProbeInfo* info, gpointer user_data) {
    auto* data = static_cast<MetadataProbeData*>(user_data);
    if (!data || !(info->type & GST_PAD_PROBE_TYPE_BUFFER)) {
        return GST_PAD_PROBE_OK;
    }

    GstBuffer* buffer = gst_pad_probe_info_get_buffer(info);
    if (!buffer) {
        return GST_PAD_PROBE_OK;
    }

    GstBuffer* writable = gst_buffer_make_writable(buffer);
    if (!writable) {
        return GST_PAD_PROBE_OK;
    }

    FrameMetadata metadata = make_frame_metadata(++data->frame_counter, gst_util_get_timestamp(), data->sensor_id);
    metadata.encode_ts = gst_util_get_timestamp();
    metadata.gain = 1.0;
    add_frame_meta(writable, metadata);
    auto** info_data = reinterpret_cast<GstMiniObject**>(&GST_PAD_PROBE_INFO_DATA(info));
    gst_mini_object_replace(info_data, GST_MINI_OBJECT_CAST(writable));
    return GST_PAD_PROBE_OK;
}

void install_metadata_probe(GstElement* pipeline, const std::string& sensor_id) {
    if (!pipeline) {
        return;
    }

    GstElement* source = gst_bin_get_by_name(GST_BIN(pipeline), "source");
    if (!source) {
        return;
    }

    GstPad* pad = gst_element_get_static_pad(source, "src");
    if (!pad) {
        gst_object_unref(source);
        return;
    }

    auto* data = new MetadataProbeData{sensor_id, 0};
    gst_pad_add_probe(pad,
                      GST_PAD_PROBE_TYPE_BUFFER,
                      metadata_probe,
                      data,
                      [](gpointer ptr) {
                          delete static_cast<MetadataProbeData*>(ptr);
                      });

    gst_object_unref(pad);
    gst_object_unref(source);
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
        pipeline = make_capture_pipeline(options.config, &error);
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

    install_metadata_probe(pipeline, options.sensor_id);

    PipelineController controller;
    controller.set_pipeline(pipeline);

    controller.set_bus_handler([pipeline](const GstMessage& message) {
        if (GST_MESSAGE_TYPE(&message) != GST_MESSAGE_STATE_CHANGED) {
            return;
        }
        const GstMessage* msg = &message;
        if (GST_MESSAGE_SRC(msg) != GST_OBJECT(pipeline)) {
            return;
        }
        GstState old_state, new_state;
        gst_message_parse_state_changed(const_cast<GstMessage*>(msg), &old_state, &new_state, nullptr);
        g_print("Pipeline state changed from %s to %s\n", gst_element_state_get_name(old_state),
                gst_element_state_get_name(new_state));
    });

#if defined(G_OS_UNIX)
    g_unix_signal_add(SIGINT, handle_signal, &controller);
    g_unix_signal_add(SIGTERM, handle_signal, &controller);
#endif

    if (!controller.play()) {
        std::cerr << "Unable to transition pipeline to PLAYING." << std::endl;
        gst_object_unref(pipeline);
        return 1;
    }

    std::cout << "Capture pipeline running. Press Ctrl+C to stop." << std::endl;
    controller.run();

    controller.stop();
    if (pipeline) {
        gst_object_unref(pipeline);
    }
    return 0;
}
