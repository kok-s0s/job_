#include "NcmDecoder.h"

#include <QFile>
#include <QDir>
#include <QUuid>
#include <QByteArray>
#include <array>
#include <CommonCrypto/CommonCrypto.h>

// NCM format uses two hard-coded AES-128 keys (public knowledge, in every
// open-source ncmdump implementation):
static constexpr uint8_t KEY_CORE[16] = {
    'h','z','H','R','A','m','s','o','5','B','k','w','7','V','N','2'
};
// (meta key not needed for audio extraction)

static QByteArray aesDecryptECB(const QByteArray& data, const uint8_t* key) {
    QByteArray out(data.size() + kCCBlockSizeAES128, '\0');
    size_t outLen = 0;
    CCCrypt(kCCDecrypt, kCCAlgorithmAES128,
            kCCOptionECBMode | kCCOptionPKCS7Padding,
            key, kCCKeySizeAES128, nullptr,
            data.constData(), data.size(),
            out.data(), out.size(), &outLen);
    out.resize(static_cast<qsizetype>(outLen));
    return out;
}

QString NcmDecoder::decode(const QString& ncmPath) {
    QFile f(ncmPath);
    if (!f.open(QIODevice::ReadOnly)) return {};

    // Magic check
    if (f.read(8) != QByteArray("CTENFDAM", 8)) return {};
    f.read(2);  // padding

    // ── Key ──────────────────────────────────────────────────────────────
    uint32_t keyLen = 0;
    f.read(reinterpret_cast<char*>(&keyLen), 4);
    QByteArray keyData = f.read(keyLen);
    for (auto& b : keyData) b ^= 0x64;

    QByteArray decKey = aesDecryptECB(keyData, KEY_CORE);
    // Decrypted key starts with literal "neteasecloudmusic" (17 bytes)
    static const int PREFIX = 17;
    if (decKey.size() <= PREFIX) return {};
    decKey = decKey.mid(PREFIX);

    // ── Build RC4 S-box ───────────────────────────────────────────────────
    std::array<uint8_t, 256> box;
    for (int i = 0; i < 256; i++) box[i] = static_cast<uint8_t>(i);
    const int kLen = decKey.size();
    for (int i = 0, j = 0; i < 256; i++) {
        j = (j + box[i] + static_cast<uint8_t>(decKey[i % kLen])) & 0xFF;
        std::swap(box[i], box[j]);
    }

    // Key stream (256-byte period)
    std::array<uint8_t, 256> ks;
    for (int i = 1; i <= 256; i++) {
        uint8_t a = box[i & 0xFF];
        uint8_t b = box[(i + a) & 0xFF];
        ks[i - 1]  = box[(a + b) & 0xFF];
    }

    // ── Skip meta + image ─────────────────────────────────────────────────
    uint32_t metaLen = 0;
    f.read(reinterpret_cast<char*>(&metaLen), 4);
    f.read(metaLen);
    f.read(4);  // crc32
    f.read(1);  // gap
    uint32_t imgLen = 0;
    f.read(reinterpret_cast<char*>(&imgLen), 4);
    f.read(imgLen);

    // ── Decrypt audio stream ──────────────────────────────────────────────
    QByteArray audio = f.readAll();
    for (qsizetype i = 0; i < audio.size(); i++)
        audio[i] ^= static_cast<char>(ks[i % 256]);

    // Detect inner format from header bytes
    QString suffix = ".mp3";
    if (audio.size() >= 4
        && audio[0] == 'f' && audio[1] == 'L'
        && audio[2] == 'a' && audio[3] == 'C')
        suffix = ".flac";

    // Write to temp file
    QString tmpPath = QDir::tempPath() + "/ncm_"
                    + QUuid::createUuid().toString(QUuid::Id128) + suffix;
    QFile tmp(tmpPath);
    if (!tmp.open(QIODevice::WriteOnly)) return {};
    tmp.write(audio);
    return tmpPath;
}
