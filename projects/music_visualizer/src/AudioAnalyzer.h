#pragma once

#include <QObject>
#include <QList>
#include <QVector>
#include <QUrl>
#include <QAudioDecoder>
#include <Accelerate/Accelerate.h>

class AudioAnalyzer : public QObject {
    Q_OBJECT
    Q_PROPERTY(QList<qreal> spectrum READ spectrum NOTIFY spectrumChanged)
    Q_PROPERTY(QList<qreal> waveform READ waveform NOTIFY spectrumChanged)
    Q_PROPERTY(bool         busy     READ busy     NOTIFY busyChanged)

public:
    static constexpr int BINS     = 64;
    static constexpr int FFT_N    = 2048;
    static constexpr int HOP      = 512;
    static constexpr int WAVE_PTS = 128;  // waveform points exposed to QML

    explicit AudioAnalyzer(QObject* parent = nullptr);
    ~AudioAnalyzer() override;

    QList<qreal> spectrum() const { return m_spectrum; }
    QList<qreal> waveform() const { return m_waveform; }
    bool         busy()     const { return m_busy; }

    Q_INVOKABLE void analyzeFile(const QUrl& url);
    Q_INVOKABLE void seekTo(qint64 positionMs);

signals:
    void spectrumChanged();
    void busyChanged();
    void analysisComplete(const QUrl& sourceUrl);
    void analysisFailed(const QString& message);

private slots:
    void onBufferReady();
    void onDecoderFinished();

private:
    void processBuffer(const QAudioBuffer& buf);
    void buildFrame(qint64 frameMs);

    struct Frame {
        qint64        timeMs;
        QVector<float> spec;   // BINS values
        QVector<float> wave;   // WAVE_PTS values, normalized -1..1
    };

    QAudioDecoder*  m_decoder;
    QUrl            m_sourceUrl;
    QList<qreal>    m_spectrum;
    QList<qreal>    m_waveform;
    QVector<float>  m_pcm;
    qint64          m_samplesIn{0};
    QVector<Frame>  m_frames;

    FFTSetup        m_fftSetup;
    QVector<float>  m_window;
    QVector<float>  m_re, m_im;
    bool            m_busy{false};
};
