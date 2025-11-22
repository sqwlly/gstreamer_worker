#include "gstreamer_worker/control/pipeline_controller.hpp"

#include <iostream>
#include <stdexcept>
#include <utility>

namespace gstreamer_worker::control {

PipelineController::PipelineController() {
    loop_ = g_main_loop_new(nullptr, FALSE);
}

PipelineController::~PipelineController() {
    stop();
    if (pipeline_) {
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
    }
    if (loop_) {
        g_main_loop_unref(loop_);
        loop_ = nullptr;
    }
}

void PipelineController::set_pipeline(GstElement* pipeline) {
    if (!pipeline) {
        throw std::invalid_argument("PipelineController requires a valid pipeline");
    }
    if (pipeline_) {
        gst_object_unref(pipeline_);
    }
    pipeline_ = GST_ELEMENT(gst_object_ref(pipeline));
    ensure_watch();
}

bool PipelineController::set_state(GstState state) {
    if (!pipeline_) {
        return false;
    }
    const GstStateChangeReturn result = gst_element_set_state(pipeline_, state);
    return result != GST_STATE_CHANGE_FAILURE;
}

bool PipelineController::play() {
    ensure_watch();
    return set_state(GST_STATE_PLAYING);
}

void PipelineController::stop() {
    if (bus_watch_id_ != 0) {
        g_source_remove(bus_watch_id_);
        bus_watch_id_ = 0;
    }
    if (pipeline_) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
    }
}

void PipelineController::run() {
    if (!loop_) {
        loop_ = g_main_loop_new(nullptr, FALSE);
    }
    g_main_loop_run(loop_);
}

void PipelineController::request_stop() {
    if (loop_ && g_main_loop_is_running(loop_)) {
        g_main_loop_quit(loop_);
    }
}

void PipelineController::set_bus_handler(BusHandler handler) {
    bus_handler_ = std::move(handler);
    ensure_watch();
}

void PipelineController::ensure_watch() {
    if (!pipeline_ || bus_watch_id_ != 0) {
        return;
    }
    GstBus* bus = gst_element_get_bus(pipeline_);
    bus_watch_id_ = gst_bus_add_watch(bus, &PipelineController::bus_callback, this);
    gst_object_unref(bus);
}

gboolean PipelineController::bus_callback(GstBus* /*bus*/, GstMessage* message, gpointer user_data) {
    auto* self = static_cast<PipelineController*>(user_data);
    if (!self || !message) {
        return G_SOURCE_CONTINUE;
    }

    if (self->bus_handler_) {
        self->bus_handler_(*message);
    }

    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_ERROR: {
            GError* err = nullptr;
            gchar* debug = nullptr;
            gst_message_parse_error(message, &err, &debug);
            std::cerr << "Pipeline error: " << (err ? err->message : "unknown") << '\n';
            if (debug) {
                std::cerr << "Debug: " << debug << '\n';
            }
            if (err) {
                g_error_free(err);
            }
            g_free(debug);
            self->request_stop();
            break;
        }
        case GST_MESSAGE_EOS:
            self->request_stop();
            break;
        default:
            break;
    }

    return G_SOURCE_CONTINUE;
}

}  // namespace gstreamer_worker::control
