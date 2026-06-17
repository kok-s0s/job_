#pragma once
#include <QObject>
#include <QVector>
#include <QString>

class LyricsParser : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString  currentLine READ currentLine NOTIFY lyricsChanged)
    Q_PROPERTY(QString  prevLine    READ prevLine    NOTIFY lyricsChanged)
    Q_PROPERTY(QString  nextLine    READ nextLine    NOTIFY lyricsChanged)
    Q_PROPERTY(bool     loaded      READ loaded      NOTIFY loadedChanged)

public:
    explicit LyricsParser(QObject* parent = nullptr);

    // Try loading <audioFilePath> with .lrc extension.  Returns true on success.
    Q_INVOKABLE bool loadFor(const QString& audioFilePath);
    Q_INVOKABLE void seekTo(qint64 positionMs);
    Q_INVOKABLE void reset();

    QString currentLine() const;
    QString prevLine()    const;
    QString nextLine()    const;
    bool    loaded()      const { return m_loaded; }

signals:
    void lyricsChanged();
    void loadedChanged();

private:
    struct Line { qint64 timeMs; QString text; };
    QVector<Line> m_lines;
    int           m_index{-1};
    bool          m_loaded{false};
};
