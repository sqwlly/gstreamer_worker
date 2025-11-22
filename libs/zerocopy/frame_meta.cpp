#include "gstreamer_worker/zerocopy/frame_meta.hpp"

#include <algorithm>
#include <cstring>

namespace gstreamer_worker::zerocopy {
namespace {

gboolean frame_meta_init(GstMeta* meta, gpointer /*params*/, GstBuffer* /*buffer*/) {
    auto* frame_meta = reinterpret_cast<FrameMeta*>(meta);
    frame_meta->payload = {};
    return TRUE;
}

void frame_meta_free(GstMeta* /*meta*/, GstBuffer* /*buffer*/) {}

gboolean frame_meta_transform(GstBuffer* dest,
                              GstMeta* meta,
                              GstBuffer* /*src*/,
                              GQuark /*type*/,
                              gpointer /*data*/) {
    auto* frame_meta = reinterpret_cast<FrameMeta*>(meta);
    add_frame_meta(dest, frame_meta->payload);
    return TRUE;
}

}  // namespace

FrameMetadata make_frame_metadata(guint64 frame_id,
                                  GstClockTime capture_ts,
                                  std::string_view sensor_id) {
    FrameMetadata metadata;
    metadata.frame_id = frame_id;
    metadata.capture_ts = capture_ts;
    set_sensor_id(metadata, sensor_id);
    return metadata;
}

void set_sensor_id(FrameMetadata& metadata, std::string_view sensor_id) {
    metadata.sensor_id.fill('\0');
    const auto max_copy = metadata.sensor_id.size() - 1;
    const auto len = std::min<std::size_t>(sensor_id.size(), max_copy);
    if (len > 0) {
        std::memcpy(metadata.sensor_id.data(), sensor_id.data(), len);
    }
}

GType frame_meta_api_type() {
    static volatile gsize g_type = 0;
    if (g_once_init_enter(&g_type)) {
        const GType type = gst_meta_api_type_register("GStreamerWorkerFrameMeta", nullptr);
        g_once_init_leave(&g_type, type);
    }
    return g_type;
}

const GstMetaInfo* frame_meta_get_info() {
    static const GstMetaInfo* info = nullptr;
    if (g_once_init_enter(&info)) {
        const GstMetaInfo* registered_info = gst_meta_register(frame_meta_api_type(),
                                                               "GStreamerWorkerFrameMeta",
                                                               sizeof(FrameMeta),
                                                               frame_meta_init,
                                                               frame_meta_free,
                                                               frame_meta_transform);
        g_once_init_leave(&info, registered_info);
    }
    return info;
}

FrameMeta* add_frame_meta(GstBuffer* buffer, const FrameMetadata& metadata) {
    if (!buffer) {
        return nullptr;
    }
    auto* meta = reinterpret_cast<FrameMeta*>(gst_buffer_add_meta(buffer, frame_meta_get_info(), nullptr));
    if (meta) {
        meta->payload = metadata;
    }
    return meta;
}

const FrameMeta* get_frame_meta(const GstBuffer* buffer) {
    if (!buffer) {
        return nullptr;
    }
    auto* meta = reinterpret_cast<FrameMeta*>(
        gst_buffer_get_meta(const_cast<GstBuffer*>(buffer), frame_meta_api_type()));
    return meta;
}

}  // namespace gstreamer_worker::zerocopy
