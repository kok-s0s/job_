import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtMultimedia
import MusicVisualizer

ApplicationWindow {
    id: root
    width: 900; height: 600
    minimumWidth: 640; minimumHeight: 420
    visible: true
    title: "Music Visualizer"
    color: "#080810"

    // ── Audio backend ─────────────────────────────────────────────────────

    AudioAnalyzer {
        id: analyzer
        onSpectrumChanged: canvas.requestPaint()
        onAnalysisComplete: {
            statusText.text = ""
            player.source = fileDialog.selectedFile
            player.play()
        }
        onBusyChanged: {
            if (busy) statusText.text = "Analyzing…"
        }
    }

    MediaPlayer {
        id: player
        audioOutput: AudioOutput { id: audioOut; volume: volumeSlider.value }
        onPositionChanged: {
            if (!seeker.pressed)
                seeker.value = duration > 0 ? position / duration : 0
            analyzer.seekTo(position)
        }
        onPlaybackStateChanged: {
            if (playbackState === MediaPlayer.StoppedState)
                seeker.value = 0
        }
    }

    // Drive the visualizer at ~60 fps even when position events are sparse
    Timer {
        interval: 16
        running: player.playbackState === MediaPlayer.PlayingState
        repeat: true
        onTriggered: analyzer.seekTo(player.position)
    }

    FileDialog {
        id: fileDialog
        title: "Open audio file"
        nameFilters: ["Audio (*.mp3 *.flac *.wav *.aac *.m4a *.ogg *.opus)"]
        onAccepted: analyzer.analyzeFile(selectedFile)
    }

    // ── Layout ────────────────────────────────────────────────────────────

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        // Song title + artist
        RowLayout {
            Layout.fillWidth: true
            Text {
                id: titleText
                text: player.metaData.title || (player.source.toString().split("/").pop().replace(/%20/g," ")) || "No file loaded"
                color: "#e2e8f0"
                font { pixelSize: 17; bold: true }
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            Text {
                text: player.metaData.albumArtist || player.metaData.author || ""
                color: "#64748b"
                font.pixelSize: 13
            }
        }

        // ── Spectrum canvas ───────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#0d0d1f"
            radius: 12

            Canvas {
                id: canvas
                anchors.fill: parent
                anchors.margins: 8

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)

                    var bars = analyzer.spectrum
                    if (!bars || bars.length === 0) return

                    var n   = bars.length
                    var gap = Math.max(1, Math.floor(width / n * 0.15))
                    var bw  = (width - gap * (n - 1)) / n

                    for (var i = 0; i < n; i++) {
                        var v  = Math.max(0, Math.min(1, bars[i]))
                        var bh = Math.max(2, v * height * 0.92)
                        var x  = i * (bw + gap)
                        var y  = height - bh

                        // Color: bass = purple, mid = blue, treble = cyan
                        var t  = i / (n - 1)
                        var r  = Math.round(124 * (1 - t) + 6  * t)
                        var g  = Math.round(58  * (1 - t) + 182 * t)
                        var bv = Math.round(237 * (1 - t) + 212 * t)

                        var grad = ctx.createLinearGradient(x, y, x, height)
                        grad.addColorStop(0.0, "rgba(" + r + "," + g + "," + bv + ",1.0)")
                        grad.addColorStop(0.6, "rgba(" + r + "," + g + "," + bv + ",0.6)")
                        grad.addColorStop(1.0, "rgba(" + r + "," + g + "," + bv + ",0.15)")
                        ctx.fillStyle = grad
                        ctx.fillRect(x, y, bw, bh)

                        // Bright cap at top of each bar
                        ctx.fillStyle = "rgba(255,255,255,0.55)"
                        ctx.fillRect(x, y, bw, Math.min(3, bh))
                    }
                }
            }

            // Idle / status overlay
            Column {
                anchors.centerIn: parent
                spacing: 10
                visible: player.playbackState !== MediaPlayer.PlayingState && !analyzer.busy

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "♫"
                    color: "#334155"
                    font.pixelSize: 56
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Open an audio file to start"
                    color: "#475569"
                    font.pixelSize: 15
                }
            }

            Text {
                id: statusText
                anchors.centerIn: parent
                color: "#94a3b8"
                font.pixelSize: 15
            }
        }

        // ── Progress bar ──────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: fmtTime(player.position)
                color: "#64748b"; font { pixelSize: 12; family: "Menlo, monospace" }
                Layout.minimumWidth: 38
            }

            Slider {
                id: seeker
                Layout.fillWidth: true
                from: 0; to: 1; value: 0
                onMoved: player.position = value * player.duration

                background: Rectangle {
                    x: seeker.leftPadding
                    y: seeker.topPadding + seeker.availableHeight / 2 - height / 2
                    width: seeker.availableWidth; height: 3; radius: 2
                    color: "#1e293b"
                    Rectangle {
                        width: seeker.visualPosition * parent.width
                        height: parent.height; radius: 2
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0.0; color: "#7c3aed" }
                            GradientStop { position: 1.0; color: "#06b6d4" }
                        }
                    }
                }
                handle: Rectangle {
                    x: seeker.leftPadding + seeker.visualPosition * seeker.availableWidth - width / 2
                    y: seeker.topPadding  + seeker.availableHeight / 2 - height / 2
                    width: 12; height: 12; radius: 6
                    color: "#7c3aed"
                    border { color: "#c4b5fd"; width: 1 }
                }
            }

            Text {
                text: fmtTime(player.duration)
                color: "#64748b"; font { pixelSize: 12; family: "Menlo, monospace" }
                Layout.minimumWidth: 38
            }
        }

        // ── Controls ──────────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            // Transport
            Repeater {
                model: [
                    { label: "⏮", action: function(){ player.position = 0 } },
                    { label: player.playbackState === MediaPlayer.PlayingState ? "⏸" : "▶", action: function(){
                        if (player.playbackState === MediaPlayer.PlayingState) player.pause()
                        else player.play()
                    }},
                    { label: "⏹", action: function(){ player.stop(); seeker.value = 0 } }
                ]
                delegate: RoundButton {
                    text: modelData.label
                    font.pixelSize: 18
                    width: 44; height: 44
                    background: Rectangle {
                        color: parent.pressed ? "#1e293b" : (parent.hovered ? "#161625" : "transparent")
                        radius: 22
                    }
                    contentItem: Text {
                        text: parent.text
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: "#e2e8f0"
                        font.pixelSize: 18
                    }
                    onClicked: modelData.action()
                }
            }

            Item { Layout.fillWidth: true }

            // Volume
            Text { text: "🔊"; color: "#64748b"; font.pixelSize: 14 }
            Slider {
                id: volumeSlider
                from: 0; to: 1; value: 0.8; width: 90
                background: Rectangle {
                    x: volumeSlider.leftPadding
                    y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2
                    width: volumeSlider.availableWidth; height: 3; radius: 2
                    color: "#1e293b"
                    Rectangle {
                        width: volumeSlider.visualPosition * parent.width
                        height: parent.height; radius: 2; color: "#475569"
                    }
                }
                handle: Rectangle {
                    x: volumeSlider.leftPadding + volumeSlider.visualPosition * volumeSlider.availableWidth - width / 2
                    y: volumeSlider.topPadding  + volumeSlider.availableHeight / 2 - height / 2
                    width: 10; height: 10; radius: 5; color: "#94a3b8"
                }
            }

            Item { width: 8 }

            // Open file
            Button {
                text: "Open File"
                onClicked: fileDialog.open()
                background: Rectangle {
                    color: parent.pressed ? "#5b21b6" : (parent.hovered ? "#6d28d9" : "#7c3aed")
                    radius: 6
                    Behavior on color { ColorAnimation { duration: 120 } }
                }
                contentItem: Text {
                    text: parent.text; color: "white"
                    font { pixelSize: 13; bold: true }
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                leftPadding: 16; rightPadding: 16
            }
        }
    }

    function fmtTime(ms) {
        var s = Math.floor(ms / 1000)
        var m = Math.floor(s / 60)
        s = s % 60
        return m + ":" + (s < 10 ? "0" : "") + s
    }
}
