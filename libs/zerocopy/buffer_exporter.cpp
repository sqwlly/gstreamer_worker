#include "gstreamer_worker/zerocopy/buffer_exporter.hpp"

#include <stdexcept>

#include <gst/allocators/gstdmabuf.h>

#if defined(G_OS_UNIX)
#include <unistd.h>
#endif

#include "gstreamer_worker/zerocopy/frame_meta.hpp"

namespace gstreamer_worker::zerocopy {
namespace {
GstMemory* find_dmabuf_memory(GstBuffer* buffer) {
    if (!buffer) {
        return nullptr;
    }

    guint memories = gst_buffer_n_memory(buffer);
    for (guint index = 0; index < memories; ++index) {
        GstMemory* memory = gst_buffer_peek_memory(buffer, index);
        if (memory && gst_is_dmabuf_memory(memory)) {
            return memory;
        }
    }
    return nullptr;
}

}  // namespace

BufferExporter::BufferExporter(Callback callback) : callback_(std::move(callback)) {
    if (!callback_) {
        throw std::invalid_argument("BufferExporter requires a callback");
    }
}

bool BufferExporter::export_sample(GstSample* sample) const {
    if (!sample) {
        return false;
    }

    GstBuffer* buffer = gst_sample_get_buffer(sample);
    if (!buffer) {
        return false;
    }

    ExportPacket packet;

    if (const auto* meta = get_frame_meta(buffer)) {
        packet.metadata = meta->payload;
    }

    GstCaps* caps = gst_sample_get_caps(sample);
    if (caps) {
        packet.caps = gst_caps_ref(caps);
    }

    packet.dma_fd = acquire_dmabuf_fd(buffer);

    callback_(packet);

    if (packet.caps) {
        gst_caps_unref(packet.caps);
    }

    return true;
}

bool BufferExporter::has_dmabuf(const GstBuffer* buffer) {
    if (!buffer) {
        return false;
    }
    GstBuffer* mutable_buffer = const_cast<GstBuffer*>(buffer);
    guint memories = gst_buffer_n_memory(mutable_buffer);
    for (guint index = 0; index < memories; ++index) {
        GstMemory* memory = gst_buffer_peek_memory(mutable_buffer, index);
        if (memory && gst_is_dmabuf_memory(memory)) {
            return true;
        }
    }
    return false;
}

int BufferExporter::acquire_dmabuf_fd(GstBuffer* buffer) {
#if defined(G_OS_UNIX)
    GstMemory* memory = find_dmabuf_memory(buffer);
    if (!memory) {
        return -1;
    }
    int fd = gst_dmabuf_memory_get_fd(memory);
    if (fd < 0) {
        return -1;
    }
    return dup(fd);
#else
    (void)buffer;
    return -1;
#endif
}

}  // namespace gstreamer_worker::zerocopy
