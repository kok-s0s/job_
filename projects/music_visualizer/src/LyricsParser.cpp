#include "LyricsParser.h"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>
#include <algorithm>

LyricsParser::LyricsParser(QObject* parent) : QObject(parent) {}

bool LyricsParser::loadFor(const QString& audioFilePath) {
    // Look for a .lrc file with the same base name
    QFileInfo fi(audioFilePath);
    QString lrcPath = fi.absolutePath() + "/" + fi.completeBaseName() + ".lrc";

    QFile f(lrcPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_loaded = false;
        emit loadedChanged();
        return false;
    }

    m_lines.clear();
    m_index = -1;

    QTextStream in(&f);
    in.setEncoding(QStringConverter::Utf8);

    // [mm:ss.xx] or [mm:ss:xx]
    static QRegularExpression re(R"(\[(\d+):(\d+)[.:](\d+)\])");

    while (!in.atEnd()) {
        QString raw = in.readLine().trimmed();
        QRegularExpressionMatchIterator it = re.globalMatch(raw);

        // Strip all timestamps to get the lyric text
        QString text = raw;
        text.remove(re).trimmed();

        while (it.hasNext()) {
            auto m = it.next();
            qint64 ms = m.captured(1).toLongLong() * 60000
                      + m.captured(2).toLongLong() * 1000
                      + m.captured(3).leftJustified(3, '0').left(3).toLongLong();
            if (!text.isEmpty())
                m_lines.append({ ms, text.trimmed() });
        }
    }

    std::sort(m_lines.begin(), m_lines.end(),
              [](const Line& a, const Line& b){ return a.timeMs < b.timeMs; });

    m_loaded = !m_lines.isEmpty();
    emit loadedChanged();
    emit lyricsChanged();
    return m_loaded;
}

void LyricsParser::seekTo(qint64 positionMs) {
    if (m_lines.isEmpty()) return;

    // Find last line whose timestamp ≤ positionMs
    int idx = -1;
    for (int i = 0; i < m_lines.size(); i++) {
        if (m_lines[i].timeMs <= positionMs) idx = i;
        else break;
    }

    if (idx != m_index) {
        m_index = idx;
        emit lyricsChanged();
    }
}

void LyricsParser::reset() {
    m_lines.clear();
    m_index = -1;
    m_loaded = false;
    emit loadedChanged();
    emit lyricsChanged();
}

QString LyricsParser::currentLine() const {
    return (m_index >= 0 && m_index < m_lines.size())
        ? m_lines[m_index].text : "";
}

QString LyricsParser::prevLine() const {
    return (m_index > 0) ? m_lines[m_index - 1].text : "";
}

QString LyricsParser::nextLine() const {
    return (m_index >= 0 && m_index + 1 < m_lines.size())
        ? m_lines[m_index + 1].text : "";
}
