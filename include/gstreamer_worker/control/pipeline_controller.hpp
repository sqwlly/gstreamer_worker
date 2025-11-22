#pragma once

#include <functional>
#include <string>

#include <gst/gst.h>

namespace gstreamer_worker::control {

class PipelineController {
  public:
    PipelineController();
    ~PipelineController();

    void set_pipeline(GstElement* pipeline);
    bool set_state(GstState state);
    bool play();
    void stop();

    void run();
    void request_stop();

    using BusHandler = std::function<void(const GstMessage&)>;
    void set_bus_handler(BusHandler handler);

  private:
    static gboolean bus_callback(GstBus* bus, GstMessage* message, gpointer user_data);

    void ensure_watch();

    GstElement* pipeline_{nullptr};
    GMainLoop* loop_{nullptr};
    guint bus_watch_id_{0};
    BusHandler bus_handler_{};
};

}  // namespace gstreamer_worker::control
