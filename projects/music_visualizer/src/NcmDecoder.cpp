#include "NcmDecoder.h"

#include <QFile>
#include <QDir>
#include <QUuid>
#include <QDebug>
#include <array>
#include <utility>
#include <CommonCrypto/CommonCrypto.h>

static constexpr uint8_t KEY_CORE[16] = {
    'h','z','H','R','A','m','s','o','5','k','I','n','b','a','x','W'
};

static QByteArray aesDecryptECB(const QByteArray& data, const uint8_t* key) {
    // Use raw ECB without automatic PKCS7 so we can inspect the full output.
    // NCM's encrypted key block length is always a multiple of 16.
    QByteArray out(data.size(), '\0');
    size_t outLen = 0;
    CCCryptorStatus st = CCCrypt(
        kCCDecrypt, kCCAlgorithmAES128,
        kCCOptionECBMode,           // no PKCS7 – handle manually
        key, kCCKeySizeAES128, nullptr,
        data.constData(), data.size(),
        out.data(), out.size(), &outLen);
    if (st != kCCSuccess) { qWarning() << "NCM: AES failed" << st; return {}; }
    out.resize(static_cast<qsizetype>(outLen));
    // Strip trailing padding bytes (value == padding length, PKCS7 style)
    if (!out.isEmpty()) {
        uint8_t pad = static_cast<uint8_t>(out.back());
        if (pad <= 16) {
            bool validPad = true;
            for (int i = out.size()-pad; i < out.size(); i++)
                if (static_cast<uint8_t>(out[i]) != pad) { validPad = false; break; }
            if (validPad) out.chop(pad);
        }
    }
    return out;
}

// KSA (RC4 key scheduling algorithm)
static std::array<uint8_t,256> ksa(const QByteArray& key) {
    std::array<uint8_t,256> box;
    for (int i = 0; i < 256; i++) box[i] = static_cast<uint8_t>(i);
    int kLen = key.size();
    for (int i = 0, j = 0; i < 256; i++) {
        j = (j + box[i] + static_cast<uint8_t>(key[i % kLen])) & 0xFF;
        std::swap(box[i], box[j]);
    }
    return box;
}

static std::array<uint8_t,256> buildKeyBox(const QByteArray& key) {
    const auto box = ksa(key);
    std::array<uint8_t,256> ks;
    for (int i = 0; i < 256; i++) {
        const int j = (i + 1) & 0xFF;
        ks[i] = box[(box[j] + box[(box[j] + j) & 0xFF]) & 0xFF];
    }
    return ks;
}

static bool looksLikeAudio(const QByteArray& d) {
    if (d.size() < 4) return false;
    uint8_t a = d[0], b = d[1], c = d[2], e = d[3];
    // MP3 sync
    if (a == 0xFF && (b & 0xE0) == 0xE0) return true;
    // ID3
    if (a == 0x49 && b == 0x44 && c == 0x33) return true;
    // FLAC
    if (a == 0x66 && b == 0x4C && c == 0x61 && e == 0x43) return true;
    return false;
}

static QByteArray decryptAudioPayload(const QByteArray& payload,
                                      const std::array<uint8_t,256>& keyBox,
                                      int skipBytes) {
    if (payload.size() <= skipBytes) return {};

    QByteArray audio = payload.mid(skipBytes);
    for (qsizetype k = 0; k < audio.size(); k++)
        audio[k] ^= static_cast<char>(keyBox[k & 0xFF]);
    return audio;
}

QString NcmDecoder::decode(const QString& ncmPath) {
    QFile f(ncmPath);
    if (!f.open(QIODevice::ReadOnly)) { qWarning() << "NCM: open failed"; return {}; }

    if (f.read(8) != QByteArray("CTENFDAM", 8)) { qWarning() << "NCM: bad magic"; return {}; }
    f.read(2);

    uint32_t keyLen = 0;
    f.read(reinterpret_cast<char*>(&keyLen), 4);
    QByteArray keyData = f.read(keyLen);
    for (auto& b : keyData) b ^= 0x64;

    QByteArray decKey = aesDecryptECB(keyData, KEY_CORE);
    if (decKey.isEmpty()) return {};

    static constexpr int PREFIX = 17; // "neteasecloudmusic"
    if (decKey.size() <= PREFIX) { qWarning() << "NCM: key too short"; return {}; }
    const QByteArray rc4key = decKey.mid(PREFIX);

    qDebug() << "NCM: rc4key len=" << rc4key.size()
             << "first8=" << rc4key.left(8).toHex();

    // Skip meta block
    uint32_t metaLen = 0;
    f.read(reinterpret_cast<char*>(&metaLen), 4);
    f.read(metaLen);

    f.read(4);  // gap (4 bytes)
    f.read(1);  // gap (1 byte)

    uint32_t imgLen = 0;
    f.read(reinterpret_cast<char*>(&imgLen), 4);
    f.read(imgLen);

    qDebug() << "NCM: audio offset=" << f.pos()
             << "keyLen=" << keyLen
             << "metaLen=" << metaLen
             << "imgLen=" << imgLen;

    const QByteArray payload = f.readAll();
    const auto keyBox = buildKeyBox(rc4key);

    QByteArray audio;
    int payloadSkip = -1;
    for (int skip = 0; skip <= 16; skip++) {
        QByteArray candidate = decryptAudioPayload(payload, keyBox, skip);
        if (looksLikeAudio(candidate)) {
            audio = std::move(candidate);
            payloadSkip = skip;
            break;
        }
    }

    if (audio.isEmpty()) {
        QByteArray candidate = decryptAudioPayload(payload, keyBox, 0);
        qWarning() << "NCM: decrypted audio has unknown header" << candidate.left(8).toHex();
        return {};
    }

    if (payloadSkip > 0)
        qDebug() << "NCM: skipped" << payloadSkip << "payload bytes before audio";

    QString suffix = looksLikeAudio(audio) && audio[0] == 'f' ? ".flac" : ".mp3";
    QString tmp = QDir::tempPath() + "/ncm_" + QUuid::createUuid().toString(QUuid::Id128) + suffix;
    QFile out(tmp);
    if (!out.open(QIODevice::WriteOnly)) { qWarning() << "NCM: write temp failed"; return {}; }
    out.write(audio);
    qDebug() << "NCM: wrote" << tmp;
    return tmp;
}
