#!/usr/bin/env bash
set -euo pipefail

VERSION=${1:-1.24.4}
DESTINATION=${2:-"$(dirname "${BASH_SOURCE[0]}")/../third_party"}
FORCE=${FORCE:-0}

mkdir -p "${DESTINATION}"
ARCHIVE_NAME="gstreamer-${VERSION}.tar.xz"
ARCHIVE_PATH="${DESTINATION}/${ARCHIVE_NAME}"
SOURCE_URL="https://gstreamer.freedesktop.org/src/gstreamer/${ARCHIVE_NAME}"

if [[ -f "${ARCHIVE_PATH}" && ${FORCE} -eq 0 ]]; then
    echo "Archive already downloaded: ${ARCHIVE_PATH}"
else
    echo "Downloading ${SOURCE_URL}"
    curl -L "${SOURCE_URL}" -o "${ARCHIVE_PATH}"
fi

EXTRACT_DIR="${DESTINATION}/gstreamer-${VERSION}"
if [[ -d "${EXTRACT_DIR}" && ${FORCE} -eq 0 ]]; then
    echo "GStreamer source already extracted: ${EXTRACT_DIR}"
    exit 0
fi

rm -rf "${EXTRACT_DIR}"
tar -xf "${ARCHIVE_PATH}" -C "${DESTINATION}"

echo "Done. Sources located at ${EXTRACT_DIR}"
