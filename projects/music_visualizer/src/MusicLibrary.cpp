#include "MusicLibrary.h"
#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <algorithm>

static const QStringList AUDIO_EXTS = {"ncm","mp3","flac","wav","m4a","ogg","aac","opus"};
static const QStringList IMG_EXTS   = {"jpg","jpeg","png","webp"};

MusicLibrary::MusicLibrary(QObject* parent) : QAbstractListModel(parent) {}

void MusicLibrary::scanFolder(const QUrl& folderUrl) {
    QString dir = folderUrl.toLocalFile();
    if (dir.isEmpty()) return;

    QDir d(dir);
    if (!d.exists()) return;

    beginResetModel();
    m_tracks.clear();

    // Recurse one level: root + immediate subdirs (skip "meta" dirs)
    QStringList searchDirs = { dir };
    for (auto& sub : d.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
        if (sub.toLower() != "meta")
            searchDirs << dir + "/" + sub;

    QStringList audioFilter;
    for (auto& ext : AUDIO_EXTS) audioFilter << "*." + ext;

    for (auto& searchDir : searchDirs) {
        QDir sd(searchDir);
        for (auto& entry : sd.entryList(audioFilter, QDir::Files, QDir::Name)) {
            QFileInfo fi(searchDir + "/" + entry);
            Track t;
            t.audioPath = fi.absoluteFilePath();
            t.name      = fi.completeBaseName();

            // Paired .lrc
            QString lrc = fi.absolutePath() + "/" + fi.completeBaseName() + ".lrc";
            if (QFile::exists(lrc)) t.lrcPath = lrc;

            // Cover: look in same dir and in a "meta" sibling dir
            t.coverPath = findCover(fi.absolutePath(), fi.completeBaseName());
            if (t.coverPath.isEmpty())
                t.coverPath = findCover(fi.absolutePath() + "/meta", fi.completeBaseName());

            m_tracks.append(t);
        }
    }

    endResetModel();
    emit countChanged();
}

QString MusicLibrary::findCover(const QString& dir, const QString& baseName) {
    QDir d(dir);
    if (!d.exists()) return {};

    // Try exact name match first
    for (auto& ext : IMG_EXTS) {
        QString p = dir + "/" + baseName + "." + ext;
        if (QFile::exists(p)) return p;
    }
    // Then any image in that directory (for cover-only dirs like meta/)
    QStringList imgFilter;
    for (auto& ext : IMG_EXTS) imgFilter << "*." + ext;
    auto imgs = d.entryList(imgFilter, QDir::Files);
    if (!imgs.isEmpty()) return dir + "/" + imgs.first();
    return {};
}

void MusicLibrary::clear() {
    beginResetModel();
    m_tracks.clear();
    endResetModel();
    emit countChanged();
}

int MusicLibrary::rowCount(const QModelIndex&) const { return m_tracks.size(); }

QVariant MusicLibrary::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_tracks.size()) return {};
    const Track& t = m_tracks[index.row()];
    switch (role) {
        case NameRole:     return t.name;
        case AudioUrlRole: return QUrl::fromLocalFile(t.audioPath);
        case LrcPathRole:  return t.lrcPath;
        case CoverUrlRole: return t.coverPath.isEmpty() ? QVariant{} : QUrl::fromLocalFile(t.coverPath);
    }
    return {};
}

QHash<int,QByteArray> MusicLibrary::roleNames() const {
    return {
        { NameRole,     "name"     },
        { AudioUrlRole, "audioUrl" },
        { LrcPathRole,  "lrcPath"  },
        { CoverUrlRole, "coverUrl" },
    };
}
