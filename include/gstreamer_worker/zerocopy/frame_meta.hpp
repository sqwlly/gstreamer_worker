#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include <gst/gst.h>

namespace gstreamer_worker::zerocopy {

struct FrameMetadata {
    guint64 frame_id{0};
    GstClockTime capture_ts{GST_CLOCK_TIME_NONE};
    GstClockTime encode_ts{GST_CLOCK_TIME_NONE};
    double exposure_ms{0.0};
    double gain{0.0};
    std::array<float, 3> imu_rpy{0.0F, 0.0F, 0.0F};
    std::array<char, 32> sensor_id{};
};

struct FrameMeta {
    GstMeta meta;
    FrameMetadata payload;
};

FrameMetadata make_frame_metadata(guint64 frame_id,
                                  GstClockTime capture_ts,
                                  std::string_view sensor_id = {});
void set_sensor_id(FrameMetadata& metadata, std::string_view sensor_id);

GType frame_meta_api_type();
const GstMetaInfo* frame_meta_get_info();

FrameMeta* add_frame_meta(GstBuffer* buffer, const FrameMetadata& metadata);
const FrameMeta* get_frame_meta(const GstBuffer* buffer);

}  // namespace gstreamer_worker::zerocopy
