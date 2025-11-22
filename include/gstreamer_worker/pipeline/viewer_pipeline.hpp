#pragma once

#include <string>

#include <gst/gst.h>

#include "gstreamer_worker/pipeline/config.hpp"

namespace gstreamer_worker::pipeline {

std::string build_viewer_launch(const ViewerPipelineConfig& config);
GstElement* make_viewer_pipeline(const ViewerPipelineConfig& config, GError** error = nullptr);

}  // namespace gstreamer_worker::pipeline
