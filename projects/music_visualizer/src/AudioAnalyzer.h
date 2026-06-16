#pragma once

#include <QObject>
#include <QList>
#include <QVector>
#include <QPair>
#include <QAudioDecoder>
#include <Accelerate/Accelerate.h>

// Decodes an audio file to mono PCM, computes real-time FFT via macOS vDSP,
// and exposes a 64-bin spectrum to QML via Q_PROPERTY.
//
// Usage:
//   analyzer.analyzeFile(url)    → triggers decode + FFT of whole file
//   analyzer.seekTo(positionMs)  → updates spectrum[] to match playback position
class AudioAnalyzer : public QObject {
    Q_OBJECT
    Q_PROPERTY(QList<qreal> spectrum READ spectrum NOTIFY spectrumChanged)
    Q_PROPERTY(bool         busy     READ busy     NOTIFY busyChanged)

public:
    static constexpr int BINS  = 64;    // display bars
    static constexpr int FFT_N = 2048;  // FFT window size
    static constexpr int HOP   = 512;   // hop between frames (~11.6ms at 44100Hz)

    explicit AudioAnalyzer(QObject* parent = nullptr);
    ~AudioAnalyzer() override;

    QList<qreal> spectrum() const { return m_spectrum; }
    bool         busy()     const { return m_busy; }

    Q_INVOKABLE void analyzeFile(const QUrl& url);
    Q_INVOKABLE void seekTo(qint64 positionMs);

signals:
    void spectrumChanged();
    void busyChanged();
    void analysisComplete();

private slots:
    void onBufferReady();
    void onDecoderFinished();

private:
    void processBuffer(const QAudioBuffer& buf);
    void buildFrame(qint64 frameMs);

    QAudioDecoder*   m_decoder;
    QList<qreal>     m_spectrum;
    QVector<float>   m_pcm;            // rolling PCM accumulator
    qint64           m_samplesIn{0};   // total samples consumed so far

    // Pre-computed spectrum timeline: (timeMs, 64 bins)
    QVector<QPair<qint64, QVector<float>>> m_frames;

    // vDSP FFT state
    FFTSetup       m_fftSetup;
    QVector<float> m_window;   // Hann window, length FFT_N
    QVector<float> m_re;       // split-complex real,  FFT_N/2
    QVector<float> m_im;       // split-complex imag,  FFT_N/2

    bool m_busy{false};
};
