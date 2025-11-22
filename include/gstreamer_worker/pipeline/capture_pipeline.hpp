#pragma once

#include <string>

#include <gst/gst.h>

#include "gstreamer_worker/pipeline/config.hpp"

namespace gstreamer_worker::pipeline {

std::string build_capture_launch(const CapturePipelineConfig& config);
GstElement* make_capture_pipeline(const CapturePipelineConfig& config, GError** error = nullptr);

}  // namespace gstreamer_worker::pipeline
