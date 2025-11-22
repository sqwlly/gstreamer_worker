# Architecture

## Capture node (edge)

```
[v4l2src io-mode=dmabuf]
    ¡ú queue leaky=downstream, max-buffers=N
    ¡ú video/x-raw (NV12, width¡Áheight, framerate)
    ¡ú nvvidconv (NVMM)
    ¡ú nvv4l2h264enc / x264enc (fallback)
    ¡ú h264parse
    ¡ú rtph264pay (optional rtpulpfecenc)
    ¡ú udpsink host=HQ port=5000
```

- **DMA-BUF ingest**: `v4l2src` keeps frames in kernel dma-bufs so they can be exported to NVMM.
- **Metadata probe**: `apps/capture_server` adds `FrameMeta` to every `GstBuffer` on the source pad, capturing frame counters, timestamps, and placeholder exposure/gain values.
- **Encoder abstraction**: `libs/pipeline/capture_pipeline.cpp` toggles between NVENC and software encoders. NVENC paths request NVMM memory (`nvbuf-memory-type=3`) and keep the pipeline zero-copy.
- **Transport**: the pipeline exposes RTP over UDP with optional ULP FEC. Latency knobs (queue size, iframeinterval, jitter buffer) are controlled through CLI flags.

## Viewer node (core)

```
udpsrc ¡ú queue ¡ú rtpjitterbuffer
    ¡ú rtph264depay ¡ú h264parse
    ¡ú decodebin | nvv4l2decoder | avdec_h264
    ¡ú queue (leaky)
    ¡ú video/x-raw(memory:NVMM) or CPU fallback
    ¡ú appsink name=display_sink
```

- `apps/viewer_client` obtains the configured `appsink` and registers a `SampleConsumer` that leverages `BufferExporter`.
- `BufferExporter` duplicates DMA-BUF file descriptors, forwards frame metadata, and emits them to a callback that can hand the descriptors to CUDA, CUDA-GL interop, Vulkan, etc.
- The viewer CLI supports switching decoders (`--backend auto|nvidia|software`), toggling zero-copy, and adjusting jitter latency.

## Control loop & lifecycle

- `libs/control/pipeline_controller` wraps `GMainLoop`, bus watching, and graceful shutdown. It integrates UNIX signal handlers (`SIGINT`, `SIGTERM`) for unattended operation.
- Bus callbacks log state transitions and stop loops on EOS/ERROR.
- Additional management interfaces (REST/gRPC) can reuse `PipelineController` by embedding it into async runtimes.

## Extending zero-copy consumers

1. Listen to the `BufferExporter` callback (`apps/viewer_client/main.cpp`).
2. Import the duplicated DMA-BUF descriptor:
   - CUDA: `cudaImportExternalMemory` with `cudaExternalMemoryHandleTypeOpaqueFd`.
   - EGL/OpenGL: `eglCreateImageKHR` + `glEGLImageTargetTexture2DOES`.
   - Vulkan: `vkImportMemoryFdKHR`.
3. Use the attached `FrameMeta` struct to correlate IMU/time-of-flight sensors with each frame.

## Observability hooks

- Extend pad probes (`install_metadata_probe`) to push StatsD/Prometheus counters.
- Add `rtpbin` or `rtpjitterbuffer` stats (packet loss, RTT) through `GstStructure` queries and feed them to the control plane.
- Combine with `gst-shark`/`gst-tracer` for latency instrumentation.

This document mirrors the choices codified in `libs/` and `apps/`. Modify the pipeline builders or metadata utilities to target different accelerators (RK3588, Intel iGPU) without changing the application entry points.
