#pragma once
#include <QString>

// Decrypts a NetEase Cloud Music .ncm file to a temporary file.
// Returns the temp file path on success, or empty string on failure.
// The caller is responsible for deleting the temp file when done.
class NcmDecoder {
public:
    static QString decode(const QString& ncmPath);
};
