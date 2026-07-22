#!/bin/bash
# Install a modern standalone protoc, exposed as `protoc-python`, used ONLY to
# generate the Python protobuf bindings (navitiacommon / monitor / jormungandr
# tests). The C++ bindings keep using the distribution protoc/libprotobuf so
# they stay ABI-compatible with kraken, hence a dedicated binary name that does
# not shadow /usr/bin/protoc.
set -euo pipefail

PROTOC_VERSION="${PROTOC_VERSION:-29.5}"
DEST="${PROTOC_PYTHON_BIN:-/usr/local/bin/protoc-python}"

case "$(uname -m)" in
    x86_64 | amd64) arch="x86_64" ;;
    aarch64 | arm64) arch="aarch_64" ;;
    *)
        echo "install_protoc.sh: unsupported architecture $(uname -m)" >&2
        exit 1
        ;;
esac

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

url="https://github.com/protocolbuffers/protobuf/releases/download/v${PROTOC_VERSION}/protoc-${PROTOC_VERSION}-linux-${arch}.zip"
echo "install_protoc.sh: downloading ${url}"
curl -fsSL -o "$tmp/protoc.zip" "$url"
python3 -m zipfile -e "$tmp/protoc.zip" "$tmp/protoc"
install -m 0755 "$tmp/protoc/bin/protoc" "$DEST"
"$DEST" --version
