# Music Visualizer

## Developer launcher

From this directory:

```bash
./scripts/install-app.sh
```

The script writes a stable developer launcher to:

```text
dist/Music Visualizer.app
```

Drag that app bundle to the Dock once. Clicking the Dock icon runs CMake,
builds the latest local code, and then opens the freshly built app. Build logs
are written to:

```text
build-launcher/launcher.log
```

The script also tries to build immediately. The built app is copied to:

```text
dist/runtime/music-visualizer.app
```

This is a local development bundle that uses your Homebrew Qt installation. It
does not run `macdeployqt`, which avoids partial framework rewriting and invalid
code signatures during iterative local builds.
