import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtMultimedia
import MusicVisualizer

ApplicationWindow {
    id: root
    width: 900; height: 660
    minimumWidth: 640; minimumHeight: 480
    visible: true
    title: "Music Visualizer"
    color: "#080810"

    // ── C++ backends ──────────────────────────────────────────────────────

    AudioAnalyzer {
        id: analyzer
        onSpectrumChanged: canvas.requestPaint()
        onAnalysisComplete: {
            statusText.text = ""
            player.source = currentAudioUrl
            player.play()
        }
        onBusyChanged: { if (busy) statusText.text = "Analyzing…" }
    }

    LyricsParser { id: lyrics }

    property url currentAudioUrl: ""

    MediaPlayer {
        id: player
        audioOutput: AudioOutput { volume: volumeSlider.value }
        onPositionChanged: {
            if (!seeker.pressed)
                seeker.value = duration > 0 ? position / duration : 0
            analyzer.seekTo(position)
            lyrics.seekTo(position)
        }
        onPlaybackStateChanged: {
            if (playbackState === MediaPlayer.StoppedState) seeker.value = 0
        }
    }

    Timer {
        interval: 16
        running: player.playbackState === MediaPlayer.PlayingState
        repeat: true
        onTriggered: {
            analyzer.seekTo(player.position)
            if (visMode === "particles") canvas.requestPaint()
        }
    }

    FileDialog {
        id: fileDialog
        title: "Open audio file"
        nameFilters: ["Audio (*.mp3 *.flac *.wav *.aac *.m4a *.ogg *.ncm)"]
        onAccepted: {
            root.currentAudioUrl = selectedFile
            lyrics.loadFor(selectedFile.toString().replace("file://",""))
            analyzer.analyzeFile(selectedFile)
        }
    }

    // ── Visualization mode ────────────────────────────────────────────────
    property string visMode: "bars"
    property var    particles: []

    function spawnParticles(bars) {
        for (var i = 0; i < bars.length; i += 2) {
            if (bars[i] > 0.35 && Math.random() < bars[i] * 0.7) {
                var t = i / (bars.length - 1)
                var r = Math.round(124*(1-t) + 6*t)
                var g = Math.round(58*(1-t)  + 182*t)
                var b = Math.round(237*(1-t) + 212*t)
                particles.push({
                    x: (i / bars.length) * canvas.width + Math.random()*8-4,
                    y: canvas.height,
                    vy: -(bars[i] * 9 + 2),
                    vx: (Math.random()-0.5) * 1.5,
                    life: 1.0,
                    size: bars[i]*7+1,
                    color: "rgba("+r+","+g+","+b+","
                })
            }
        }
        for (var j = particles.length-1; j >= 0; j--) {
            particles[j].y  += particles[j].vy
            particles[j].x  += particles[j].vx
            particles[j].vy *= 0.97
            particles[j].life -= 0.018
            if (particles[j].life <= 0) particles.splice(j,1)
        }
    }

    // ── Layout ────────────────────────────────────────────────────────────

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 8

        // Title + artist
        RowLayout {
            Layout.fillWidth: true
            Text {
                text: player.metaData.title
                      || currentAudioUrl.toString().split("/").pop().replace(/%20/g," ")
                      || "No file loaded"
                color: "#e2e8f0"; font { pixelSize: 16; bold: true }
                elide: Text.ElideRight; Layout.fillWidth: true
            }
            Text {
                text: player.metaData.albumArtist || player.metaData.author || ""
                color: "#64748b"; font.pixelSize: 12
            }
        }

        // ── Canvas area ───────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#0d0d1f"; radius: 10

            Canvas {
                id: canvas
                anchors.fill: parent
                anchors.margins: 8

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    var bars = analyzer.spectrum
                    if (!bars || bars.length === 0) return

                    switch (visMode) {
                    case "bars":    drawBars(ctx, bars);     break
                    case "circle":  drawCircle(ctx, bars);   break
                    case "wave":    drawWave(ctx);            break
                    case "particles": {
                        spawnParticles(bars)
                        drawParticles(ctx)
                        break
                    }
                    }
                }

                function barColor(i, n) {
                    var t = i / (n-1)
                    return "rgba("+Math.round(124*(1-t)+6*t)+","
                                  +Math.round(58*(1-t)+182*t)+","
                                  +Math.round(237*(1-t)+212*t)+","
                }

                function drawBars(ctx, bars) {
                    var n   = bars.length
                    var gap = Math.max(1, Math.floor(width/n*0.15))
                    var bw  = (width - gap*(n-1)) / n
                    for (var i = 0; i < n; i++) {
                        var v  = Math.max(0, bars[i])
                        var bh = Math.max(2, v * height * 0.92)
                        var x  = i*(bw+gap), y = height-bh
                        var g = ctx.createLinearGradient(x,y,x,height)
                        g.addColorStop(0.0, barColor(i,n)+"1.0)")
                        g.addColorStop(1.0, barColor(i,n)+"0.15)")
                        ctx.fillStyle = g
                        ctx.fillRect(x, y, bw, bh)
                        ctx.fillStyle = "rgba(255,255,255,0.5)"
                        ctx.fillRect(x, y, bw, Math.min(2, bh))
                    }
                }

                function drawCircle(ctx, bars) {
                    var cx = width/2, cy = height/2
                    var minR = Math.min(width,height)*0.18
                    var maxR = Math.min(width,height)*0.46
                    var n = bars.length

                    // Inner glow ring
                    var glow = ctx.createRadialGradient(cx,cy,minR*0.5,cx,cy,minR)
                    glow.addColorStop(0, "rgba(124,58,237,0.3)")
                    glow.addColorStop(1, "transparent")
                    ctx.fillStyle = glow
                    ctx.beginPath(); ctx.arc(cx,cy,minR,0,Math.PI*2); ctx.fill()

                    for (var i = 0; i < n; i++) {
                        var angle = (i/n)*Math.PI*2 - Math.PI/2
                        var v = Math.max(0, bars[i])
                        var r2 = minR + v*(maxR-minR)
                        ctx.strokeStyle = barColor(i,n)+"0.9)"
                        ctx.lineWidth   = Math.max(2, (2*Math.PI*minR/n)*0.65)
                        ctx.beginPath()
                        ctx.moveTo(cx+Math.cos(angle)*minR, cy+Math.sin(angle)*minR)
                        ctx.lineTo(cx+Math.cos(angle)*r2,   cy+Math.sin(angle)*r2)
                        ctx.stroke()
                    }
                }

                function drawWave(ctx) {
                    var wave = analyzer.waveform
                    if (!wave || wave.length === 0) return
                    var n = wave.length

                    ctx.lineWidth   = 2
                    ctx.strokeStyle = "rgba(124,58,237,0.9)"
                    ctx.shadowBlur  = 10
                    ctx.shadowColor = "#7c3aed"
                    ctx.beginPath()
                    for (var i = 0; i < n; i++) {
                        var x = (i/(n-1))*width
                        var y = (1 - wave[i])/2 * height
                        i===0 ? ctx.moveTo(x,y) : ctx.lineTo(x,y)
                    }
                    ctx.stroke()
                    ctx.shadowBlur = 0

                    // Mirror below center with cyan
                    ctx.strokeStyle = "rgba(6,182,212,0.5)"
                    ctx.beginPath()
                    for (var j = 0; j < n; j++) {
                        var xm = (j/(n-1))*width
                        var ym = (1 + wave[j])/2 * height
                        j===0 ? ctx.moveTo(xm,ym) : ctx.lineTo(xm,ym)
                    }
                    ctx.stroke()
                }

                function drawParticles(ctx) {
                    for (var i = 0; i < particles.length; i++) {
                        var p = particles[i]
                        ctx.fillStyle = p.color + p.life + ")"
                        ctx.beginPath()
                        ctx.arc(p.x, p.y, p.size * p.life, 0, Math.PI*2)
                        ctx.fill()
                    }
                }
            }

            // Mode switcher buttons (top-right of canvas)
            Row {
                anchors { top: parent.top; right: parent.right; margins: 10 }
                spacing: 4

                Repeater {
                    model: [
                        { key: "bars",      icon: "▤" },
                        { key: "circle",    icon: "◎" },
                        { key: "wave",      icon: "∿" },
                        { key: "particles", icon: "✦" }
                    ]
                    delegate: Rectangle {
                        width: 30; height: 30; radius: 6
                        color: visMode === modelData.key ? "#7c3aed" : "#1e293b"
                        Text {
                            anchors.centerIn: parent
                            text: modelData.icon; color: "white"; font.pixelSize: 14
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: { visMode = modelData.key; canvas.requestPaint() }
                        }
                    }
                }
            }

            // Idle overlay
            Column {
                anchors.centerIn: parent
                spacing: 8
                visible: player.playbackState !== MediaPlayer.PlayingState && !analyzer.busy
                Text { anchors.horizontalCenter: parent.horizontalCenter; text: "♫"; color: "#334155"; font.pixelSize: 48 }
                Text { anchors.horizontalCenter: parent.horizontalCenter; text: "Open an audio file to start"; color: "#475569"; font.pixelSize: 14 }
            }

            Text { id: statusText; anchors.centerIn: parent; color: "#94a3b8"; font.pixelSize: 14 }
        }

        // ── Lyrics ────────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 68
            color: "#0d0d1f"; radius: 8
            visible: lyrics.loaded

            Column {
                anchors.centerIn: parent
                width: parent.width - 24
                spacing: 2

                Text {
                    width: parent.width
                    text: lyrics.prevLine
                    color: "#475569"; font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                }
                Text {
                    width: parent.width
                    text: lyrics.currentLine
                    color: "#e2e8f0"; font { pixelSize: 15; bold: true }
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight

                    Behavior on text {
                        SequentialAnimation {
                            NumberAnimation { target: currentLyricText; property: "opacity"; to: 0; duration: 80 }
                            PropertyAction  { }
                            NumberAnimation { target: currentLyricText; property: "opacity"; to: 1; duration: 120 }
                        }
                    }
                    id: currentLyricText
                }
                Text {
                    width: parent.width
                    text: lyrics.nextLine
                    color: "#334155"; font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                }
            }
        }

        // ── Progress ──────────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: fmtTime(player.position)
                color: "#64748b"
                font.pixelSize: 12
                Layout.minimumWidth: 36
            }

            Slider {
                id: seeker
                Layout.fillWidth: true
                from: 0; to: 1; value: 0
                onMoved: player.position = value * player.duration
                background: Rectangle {
                    x: seeker.leftPadding
                    y: seeker.topPadding + seeker.availableHeight / 2 - height / 2
                    width: seeker.availableWidth; height: 3; radius: 2; color: "#1e293b"
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
                    y: seeker.topPadding + seeker.availableHeight / 2 - height / 2
                    width: 12; height: 12; radius: 6; color: "#7c3aed"
                }
            }

            Text {
                text: fmtTime(player.duration)
                color: "#64748b"
                font.pixelSize: 12
                Layout.minimumWidth: 36
            }
        }

        // ── Controls ──────────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            RoundButton {
                text: "⏮"; font.pixelSize: 18; width: 42; height: 42
                background: Rectangle { color: parent.pressed ? "#1e293b" : (parent.hovered ? "#161625" : "transparent"); radius: 21 }
                contentItem: Text { text: parent.text; color: "#e2e8f0"; font.pixelSize: 18; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                onClicked: player.position = 0
            }
            RoundButton {
                text: player.playbackState === MediaPlayer.PlayingState ? "⏸" : "▶"
                font.pixelSize: 18; width: 42; height: 42
                background: Rectangle { color: parent.pressed ? "#1e293b" : (parent.hovered ? "#161625" : "transparent"); radius: 21 }
                contentItem: Text { text: parent.text; color: "#e2e8f0"; font.pixelSize: 18; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                onClicked: player.playbackState === MediaPlayer.PlayingState ? player.pause() : player.play()
            }
            RoundButton {
                text: "⏹"; font.pixelSize: 18; width: 42; height: 42
                background: Rectangle { color: parent.pressed ? "#1e293b" : (parent.hovered ? "#161625" : "transparent"); radius: 21 }
                contentItem: Text { text: parent.text; color: "#e2e8f0"; font.pixelSize: 18; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                onClicked: { player.stop(); seeker.value = 0 }
            }

            Item { Layout.fillWidth: true }

            Text { text: "🔊"; color: "#64748b"; font.pixelSize: 13 }
            Slider {
                id: volumeSlider; from: 0; to: 1; value: 0.8; width: 80
                background: Rectangle {
                    x: volumeSlider.leftPadding; y: volumeSlider.topPadding+volumeSlider.availableHeight/2-height/2
                    width: volumeSlider.availableWidth; height: 3; radius: 2; color: "#1e293b"
                    Rectangle { width: volumeSlider.visualPosition*parent.width; height: parent.height; radius: 2; color: "#475569" }
                }
                handle: Rectangle {
                    x: volumeSlider.leftPadding+volumeSlider.visualPosition*volumeSlider.availableWidth-width/2
                    y: volumeSlider.topPadding+volumeSlider.availableHeight/2-height/2
                    width: 10; height: 10; radius: 5; color: "#94a3b8"
                }
            }

            Item { width: 6 }

            Button {
                text: "Open File"; onClicked: fileDialog.open()
                background: Rectangle {
                    color: parent.pressed?"#5b21b6":(parent.hovered?"#6d28d9":"#7c3aed"); radius: 6
                    Behavior on color { ColorAnimation { duration: 100 } }
                }
                contentItem: Text {
                    text: parent.text; color: "white"
                    font.pixelSize: 13; font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                leftPadding: 16; rightPadding: 16
            }
        }
    }

    function fmtTime(ms) {
        var s = Math.floor(ms/1000), m = Math.floor(s/60)
        s = s%60; return m+":"+(s<10?"0":"")+s
    }
}
