#include "NcmDecoder.h"

#include <QFile>
#include <QDir>
#include <QUuid>
#include <QDebug>
#include <array>
#include <CommonCrypto/CommonCrypto.h>

// AES-128 key for NCM key block (public knowledge, in every open-source ncmdump)
static constexpr uint8_t KEY_CORE[16] = {
    'h','z','H','R','A','m','s','o','5','B','k','w','7','V','N','2'
};

static QByteArray aesDecryptECB(const QByteArray& data, const uint8_t* key) {
    QByteArray out(data.size() + kCCBlockSizeAES128, '\0');
    size_t outLen = 0;
    CCCryptorStatus st = CCCrypt(
        kCCDecrypt, kCCAlgorithmAES128,
        kCCOptionECBMode | kCCOptionPKCS7Padding,
        key, kCCKeySizeAES128, nullptr,
        data.constData(), data.size(),
        out.data(), out.size(), &outLen);
    if (st != kCCSuccess) { qWarning() << "NCM: AES decrypt failed" << st; return {}; }
    out.resize(static_cast<qsizetype>(outLen));
    return out;
}

QString NcmDecoder::decode(const QString& ncmPath) {
    QFile f(ncmPath);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "NCM: cannot open" << ncmPath;
        return {};
    }

    // ── Magic ─────────────────────────────────────────────────────────────
    QByteArray magic = f.read(8);
    if (magic != QByteArray("CTENFDAM", 8)) {
        qWarning() << "NCM: wrong magic" << magic.toHex();
        return {};
    }
    f.read(2);  // 2-byte gap

    // ── Key block: XOR 0x64, then AES-128-ECB ────────────────────────────
    uint32_t keyLen = 0;
    f.read(reinterpret_cast<char*>(&keyLen), 4);
    QByteArray keyData = f.read(keyLen);
    for (auto& b : keyData) b ^= 0x64;

    QByteArray decKey = aesDecryptECB(keyData, KEY_CORE);
    if (decKey.isEmpty()) return {};

    // Strip "neteasecloudmusic" prefix (17 bytes)
    static constexpr int PREFIX = 17;
    if (decKey.size() <= PREFIX) {
        qWarning() << "NCM: decrypted key too short, got" << decKey.size() << "bytes, prefix:" << decKey.left(20);
        return {};
    }
    decKey = decKey.mid(PREFIX);
    const int kLen = decKey.size();

    // ── KSA (RC4 key scheduling) ──────────────────────────────────────────
    std::array<uint8_t, 256> box;
    for (int i = 0; i < 256; i++) box[i] = static_cast<uint8_t>(i);
    for (int i = 0, j = 0; i < 256; i++) {
        j = (j + box[i] + static_cast<uint8_t>(decKey[i % kLen])) & 0xFF;
        std::swap(box[i], box[j]);
    }

    // ── Generate 256-byte keystream (NCM custom PRGA, matches ncmdump) ────
    // For each index i, j = (i+1), swap box[j] and box[(j+box[j]) & 0xFF],
    // then take box[(old_box_j + new_box_j) & 0xFF] as keystream byte.
    std::array<uint8_t, 256> ks;
    for (int i = 0; i < 256; i++) {
        uint8_t j  = static_cast<uint8_t>(i + 1);
        uint8_t a  = box[j];                        // box[j] before swap
        uint8_t d  = static_cast<uint8_t>(j + a);  // target index
        std::swap(box[j], box[d]);
        ks[i] = box[static_cast<uint8_t>(a + box[j])];  // box[j] after swap
    }

    // ── Skip meta block ───────────────────────────────────────────────────
    uint32_t metaLen = 0;
    f.read(reinterpret_cast<char*>(&metaLen), 4);
    f.read(metaLen);
    f.read(4);   // crc32
    f.read(1);   // gap
    uint32_t imgLen = 0;
    f.read(reinterpret_cast<char*>(&imgLen), 4);
    f.read(imgLen);

    // ── Decrypt audio stream ──────────────────────────────────────────────
    QByteArray audio = f.readAll();
    for (qsizetype k = 0; k < audio.size(); k++)
        audio[k] ^= static_cast<char>(ks[k & 0xFF]);

    // ── Detect format from audio header ──────────────────────────────────
    QString suffix = ".mp3";
    if (audio.size() >= 4
        && static_cast<uint8_t>(audio[0]) == 0x66  // 'f'
        && static_cast<uint8_t>(audio[1]) == 0x4C  // 'L'
        && static_cast<uint8_t>(audio[2]) == 0x61  // 'a'
        && static_cast<uint8_t>(audio[3]) == 0x43) // 'C'
        suffix = ".flac";

    qDebug() << "NCM: decoded" << audio.size() << "bytes, format" << suffix
             << "first bytes:" << audio.left(4).toHex();

    // ── Write temp file ───────────────────────────────────────────────────
    QString tmpPath = QDir::tempPath() + "/ncm_"
                    + QUuid::createUuid().toString(QUuid::Id128) + suffix;
    QFile tmp(tmpPath);
    if (!tmp.open(QIODevice::WriteOnly)) {
        qWarning() << "NCM: cannot write temp file" << tmpPath;
        return {};
    }
    tmp.write(audio);
    qDebug() << "NCM: wrote temp file" << tmpPath;
    return tmpPath;
}
