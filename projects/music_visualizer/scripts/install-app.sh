#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build-launcher"
DIST_DIR="$ROOT_DIR/dist"
RUNTIME_DIR="$DIST_DIR/runtime"
LAUNCHER_APP="$DIST_DIR/Music Visualizer.app"
LAUNCHER_EXE="$LAUNCHER_APP/Contents/MacOS/MusicVisualizerLauncher"
RUNTIME_APP="$RUNTIME_DIR/music-visualizer.app"
QT_PREFIX="${QT_PREFIX:-/opt/homebrew/opt/qt6}"

package_runtime_app() {
    local built_app="$1"

    mkdir -p "$RUNTIME_DIR"
    rm -rf "$RUNTIME_APP"
    ditto "$built_app" "$RUNTIME_APP"

    codesign --remove-signature "$RUNTIME_APP" >/dev/null 2>&1 || true
    codesign --force --deep --sign - "$RUNTIME_APP"
}

find_built_app() {
    for candidate in "$BUILD_DIR"/*.app "$BUILD_DIR"/MusicVisualizer.app "$BUILD_DIR"/music-visualizer.app; do
        if [[ -d "$candidate" ]]; then
            printf '%s\n' "$candidate"
            return 0
        fi
    done
    return 1
}

write_launcher() {
    mkdir -p "$LAUNCHER_APP/Contents/MacOS"

    cat > "$LAUNCHER_APP/Contents/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
  "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleDevelopmentRegion</key>
  <string>en</string>
  <key>CFBundleExecutable</key>
  <string>MusicVisualizerLauncher</string>
  <key>CFBundleIdentifier</key>
  <string>dev.local.musicvisualizer.launcher</string>
  <key>CFBundleName</key>
  <string>Music Visualizer</string>
  <key>CFBundlePackageType</key>
  <string>APPL</string>
  <key>CFBundleShortVersionString</key>
  <string>1.0</string>
  <key>CFBundleVersion</key>
  <string>1.0</string>
  <key>LSMinimumSystemVersion</key>
  <string>12.0</string>
</dict>
</plist>
PLIST

    cat > "$LAUNCHER_EXE" <<LAUNCHER
#!/usr/bin/env bash
set -euo pipefail

export PATH="/opt/homebrew/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin"

ROOT_DIR="$ROOT_DIR"
BUILD_DIR="\$ROOT_DIR/build-launcher"
DIST_DIR="\$ROOT_DIR/dist"
RUNTIME_DIR="\$DIST_DIR/runtime"
RUNTIME_APP="\$RUNTIME_DIR/music-visualizer.app"
QT_PREFIX="\${QT_PREFIX:-$QT_PREFIX}"
LOG_FILE="\$BUILD_DIR/launcher.log"

package_runtime_app() {
    local built_app="\$1"
    mkdir -p "\$RUNTIME_DIR"
    rm -rf "\$RUNTIME_APP"
    ditto "\$built_app" "\$RUNTIME_APP"
    codesign --remove-signature "\$RUNTIME_APP" >/dev/null 2>&1 || true
    codesign --force --deep --sign - "\$RUNTIME_APP"
}

find_built_app() {
    for candidate in "\$BUILD_DIR"/*.app "\$BUILD_DIR"/MusicVisualizer.app "\$BUILD_DIR"/music-visualizer.app; do
        if [[ -d "\$candidate" ]]; then
            printf '%s\n' "\$candidate"
            return 0
        fi
    done
    return 1
}

mkdir -p "\$BUILD_DIR"
{
    echo "Building Music Visualizer at \$(date)"
    cmake -S "\$ROOT_DIR" -B "\$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
    cmake --build "\$BUILD_DIR" --config Release
    built_app="\$(find_built_app)"
    package_runtime_app "\$built_app"
} > "\$LOG_FILE" 2>&1 || {
    osascript -e "display dialog \\"Music Visualizer build/package failed. See: \$LOG_FILE\\" buttons {\\"OK\\"} default button \\"OK\\" with icon caution" >/dev/null 2>&1 || true
    exit 1
}

open -n "\$RUNTIME_APP"
LAUNCHER

    chmod +x "$LAUNCHER_EXE"
    codesign --remove-signature "$LAUNCHER_APP" >/dev/null 2>&1 || true
    codesign --force --deep --sign - "$LAUNCHER_APP"
}

write_launcher
echo "Launcher app written to $LAUNCHER_APP"

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --config Release

BUILT_APP="$(find_built_app)"
package_runtime_app "$BUILT_APP"

echo "Runtime app written to $RUNTIME_APP"
echo "Keep this launcher in the Dock: $LAUNCHER_APP"
