#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
PLUGIN_IDENTITY_FILE="${ROOT_DIR}/cmake/PluginIdentity.cmake"
DIST_DIR="${ROOT_DIR}/dist"
STAGING_DIR="${ROOT_DIR}/build/release-staging"

usage() {
  cat <<USAGE
Usage: $(basename "$0") --platform <windows|macos> [options]

Creates beta release archives with semantic-version artifact names.

Options:
  --platform <name>       Target platform: windows or macos (required)
  --installer <path>      Path to pre-built installer (msi/pkg)
  --vst3 <path>           Path to SecretSynth.vst3 bundle/directory
  --au <path>             Path to SecretSynth.component (macOS optional)
  --out-dir <path>        Output directory (default: ${DIST_DIR})
  --build-meta <value>    Append +<value> build metadata to version tag
  -h, --help              Show this help message

Examples:
  $(basename "$0") --platform windows --installer build/SecretSynth.msi --vst3 build/SecretSynth.vst3
  $(basename "$0") --platform macos --installer build/SecretSynth.pkg --vst3 build/SecretSynth.vst3 --au build/SecretSynth.component
USAGE
}

require_file() {
  local path="$1"
  local label="$2"
  if [[ ! -e "$path" ]]; then
    echo "Error: ${label} not found at ${path}" >&2
    exit 1
  fi
}

platform=""
installer_path=""
vst3_path=""
au_path=""
out_dir="$DIST_DIR"
build_meta=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --platform)
      platform="${2:-}"
      shift 2
      ;;
    --installer)
      installer_path="${2:-}"
      shift 2
      ;;
    --vst3)
      vst3_path="${2:-}"
      shift 2
      ;;
    --au)
      au_path="${2:-}"
      shift 2
      ;;
    --out-dir)
      out_dir="${2:-}"
      shift 2
      ;;
    --build-meta)
      build_meta="${2:-}"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage
      exit 1
      ;;
  esac
done

if [[ -z "$platform" ]]; then
  echo "Error: --platform is required" >&2
  usage
  exit 1
fi

if [[ "$platform" != "windows" && "$platform" != "macos" ]]; then
  echo "Error: --platform must be either windows or macos" >&2
  exit 1
fi

require_file "$PLUGIN_IDENTITY_FILE" "Plugin identity file"

version="$(sed -n 's/^set(SECRET_SYNTH_VERSION "\([0-9]\+\.[0-9]\+\.[0-9]\+\)")/\1/p' "$PLUGIN_IDENTITY_FILE")"
if [[ -z "$version" ]]; then
  echo "Error: could not parse SECRET_SYNTH_VERSION from ${PLUGIN_IDENTITY_FILE}" >&2
  exit 1
fi

version_tag="v${version}"
if [[ -n "$build_meta" ]]; then
  version_tag="${version_tag}+${build_meta}"
fi

case "$platform" in
  windows)
    installer_ext="msi"
    ;;
  macos)
    installer_ext="pkg"
    ;;
esac

if [[ -z "$installer_path" ]]; then
  echo "Error: --installer is required" >&2
  exit 1
fi

if [[ -z "$vst3_path" ]]; then
  echo "Error: --vst3 is required" >&2
  exit 1
fi

require_file "$installer_path" "Installer"
require_file "$vst3_path" "VST3 bundle"
if [[ -n "$au_path" ]]; then
  require_file "$au_path" "AU component"
fi

archive_basename="SecretSynth-beta-${version_tag}-${platform}"
archive_name="${archive_basename}.zip"
mkdir -p "$STAGING_DIR" "$out_dir"

platform_stage="${STAGING_DIR}/${archive_basename}"
rm -rf "$platform_stage"
mkdir -p "$platform_stage"

cp -R "$installer_path" "${platform_stage}/SecretSynth-installer.${installer_ext}"
cp -R "$vst3_path" "${platform_stage}/SecretSynth.vst3"
if [[ -n "$au_path" ]]; then
  cp -R "$au_path" "${platform_stage}/SecretSynth.component"
fi

(
  cd "$STAGING_DIR"
  rm -f "${out_dir}/${archive_name}"
  zip -qr "${out_dir}/${archive_name}" "${archive_basename}"
)

echo "Created ${out_dir}/${archive_name}"
