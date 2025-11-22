# GStreamer Worker

Zero-copy GStreamer video transport pipeline featuring a capture server and a viewer client. The capture app ingests CSI/USB cameras via `v4l2src` with DMA-BUF buffers, encodes with NVENC or software fallback, adds metadata, and streams RTP over UDP. The viewer app receives, decodes, exports DMA-BUF file descriptors, and exposes GPU-ready frames through an `appsink` callback.

## Repository layout

```
apps/              # Capture server & viewer entry points
libs/              # Reusable zerocopy, pipeline, and control libraries
include/           # Public headers consumed by applications
scripts/           # Helper scripts for fetching GStreamer source releases
third_party/       # Populated by setup scripts
tests/             # Lightweight config/unit checks
```

## Requirements

- CMake 3.21+
- A C++20 compiler (GCC 11+, Clang 13+, or MSVC 2022)
- GStreamer 1.20+ development packages with plugins: base, app, video, rtp, rtsp, pbutils, allocators, nvcodec (optional)
- pkg-config
- For zero-copy NVIDIA path: Jetson Linux 35.x+ with `nvvidconv`/`nvv4l2h264enc` plugins

Use the convenience scripts to download upstream GStreamer sources if you build the framework locally:

```powershell
# Windows PowerShell
scripts\setup_gstreamer.ps1 -Version 1.24.4
```

```bash
# Linux/macOS
FORCE=1 scripts/setup_gstreamer.sh 1.24.4 ~/workspace/gstreamer_deps
```

> The scripts only mirror the sources under `third_party/`. Link or build the SDK using your preferred workflow (Meson, Cerbero, distro packages, etc.).

## Build

```bash
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

To run the config snapshot check:

```bash
ctest --output-on-failure --test-dir build
```

## Usage

1. **Capture server** (Jetson/edge node):
   ```bash
   ./build/apps/capture_server/capture_server \
       --device /dev/video0 --host 192.168.10.2 --port 5000 \
       --width 3840 --height 2160 --fps 30 --bitrate 12000000 \
       --sensor-id cam_front
   ```
   Flags such as `--no-nvenc`, `--no-zero-copy`, `--fec 20`, or `--queue-size 8` let you switch encoders, disable DMA-BUF, add FEC, or adjust buffering. Without a physical camera, add `--use-test-pattern` (optional `--test-pattern smpte|snow|ball`) to source frames from `videotestsrc` instead.

2. **Viewer client** (central server/workstation):
   ```bash
   ./build/apps/viewer_client/viewer_client \
       --listen 0.0.0.0 --port 5000 --backend nvidia --latency 20
   ```
   Use `--backend software` on x86 machines without NVDEC and `--no-zero-copy` to fall back to CPU buffers.

Both binaries install metadata probes/buffer exporters automatically. The viewer prints frame IDs, DMA-BUF file descriptors, and caps when `--quiet` is not supplied.

## Zero-copy path

- `v4l2src io-mode=dmabuf` maps capture buffers to DMA-BUF handles.
- A pad probe injects `FrameMeta` (frame ID, timestamps, IMU placeholders) into `GstBuffer` instances without extra copies.
- NVENC builds keep frames inside NVMM memory through `nvvidconv` and `nvv4l2h264enc`.
- On the viewer side, DMA-BUF FDs are duplicated and passed to the callback supplied to `BufferExporter`, enabling CUDA, EGL, or Vulkan consumers to import GPU memory directly.

## Testing & validation

- `tests/config_snapshot` validates capture/viewer pipeline descriptions and doubles as documentation (`ctest -R config_snapshot --output-on-failure`).
- Integrate `gst-validate-1.0` or system tests by extending `tests/` with custom executables; the infrastructure is ready for additional suites.

## Next steps

- Hook the exported DMA-BUF descriptors into CUDA/OpenGL consumers (see `BufferExporter` usage in `viewer_client`).
- Extend `libs/control` with gRPC/REST endpoints for runtime parameter tuning.
- Add CI recipes (GitHub Actions, GitLab) that cross-build for aarch64/x86_64 and run the snapshot test under QEMU or containers.
