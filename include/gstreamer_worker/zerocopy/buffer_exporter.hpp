#pragma once

#include <functional>
#include <optional>

#include <gst/app/gstappsink.h>

#include "gstreamer_worker/zerocopy/frame_meta.hpp"

namespace gstreamer_worker::zerocopy {

struct ExportPacket {
    // File descriptor duplicated from the underlying DMA-BUF memory. The callback
    // becomes the owner and must close it when done.
    int dma_fd{-1};
    GstCaps* caps{nullptr};
    FrameMetadata metadata{};
};

class BufferExporter {
  public:
    using Callback = std::function<void(const ExportPacket&)>;

    explicit BufferExporter(Callback callback);

    bool export_sample(GstSample* sample) const;

    static bool has_dmabuf(const GstBuffer* buffer);
    static int acquire_dmabuf_fd(GstBuffer* buffer);

  private:
    Callback callback_;
};

}  // namespace gstreamer_worker::zerocopy
