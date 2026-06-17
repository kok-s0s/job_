#pragma once
#include <QAbstractListModel>
#include <QUrl>
#include <QString>
#include <QVector>

struct Track {
    QString name;       // display name (without extension)
    QString audioPath;  // .ncm or .mp3/.flac path
    QString lrcPath;    // .lrc path (may be empty)
    QString coverPath;  // cover image path (may be empty)
};

// ListModel that QML ListView can bind to directly.
// Roles: name, audioUrl, lrcPath, coverUrl
class MusicLibrary : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles { NameRole = Qt::UserRole+1, AudioUrlRole, LrcPathRole, CoverUrlRole };

    explicit MusicLibrary(QObject* parent = nullptr);

    // Scan a folder for audio files (.ncm .mp3 .flac .wav .m4a .ogg)
    // and their paired .lrc and cover images.
    Q_INVOKABLE void scanFolder(const QUrl& folderUrl);
    Q_INVOKABLE void clear();

    int      rowCount(const QModelIndex& = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int,QByteArray> roleNames() const override;

signals:
    void countChanged();

private:
    QVector<Track> m_tracks;

    static QString findCover(const QString& dir, const QString& baseName);
};
