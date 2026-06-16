#include "AudioAnalyzer.h"
#include <QAudioFormat>
#include <cmath>
#include <algorithm>

static constexpr int   SAMPLE_RATE = 44100;
static constexpr float LOG2N       = 11.0f;   // log2(FFT_N=2048)

AudioAnalyzer::AudioAnalyzer(QObject* parent)
    : QObject(parent)
    , m_decoder(new QAudioDecoder(this))
{
    // Request mono float32 PCM so FFT input is always consistent
    QAudioFormat fmt;
    fmt.setSampleRate(SAMPLE_RATE);
    fmt.setChannelCount(1);
    fmt.setSampleFormat(QAudioFormat::Float);
    m_decoder->setAudioFormat(fmt);

    connect(m_decoder, &QAudioDecoder::bufferReady, this, &AudioAnalyzer::onBufferReady);
    connect(m_decoder, &QAudioDecoder::finished,    this, &AudioAnalyzer::onDecoderFinished);

    // vDSP FFT setup (reused across all frames — expensive to create)
    m_fftSetup = vDSP_create_fftsetup(static_cast<vDSP_Length>(LOG2N), FFT_RADIX2);

    m_window.resize(FFT_N);
    vDSP_hann_window(m_window.data(), FFT_N, vDSP_HANN_NORM);

    m_re.resize(FFT_N / 2);
    m_im.resize(FFT_N / 2);

    m_spectrum.resize(BINS);
    m_spectrum.fill(0.0);
}

AudioAnalyzer::~AudioAnalyzer() {
    vDSP_destroy_fftsetup(m_fftSetup);
}

void AudioAnalyzer::analyzeFile(const QUrl& url) {
    m_frames.clear();
    m_pcm.clear();
    m_samplesIn = 0;

    m_busy = true;
    emit busyChanged();
    m_spectrum.fill(0.0);
    emit spectrumChanged();

    m_decoder->setSource(url);
    m_decoder->start();
}

void AudioAnalyzer::onBufferReady() {
    while (m_decoder->bufferAvailable())
        processBuffer(m_decoder->read());
}

void AudioAnalyzer::processBuffer(const QAudioBuffer& buf) {
    const float* src = buf.constData<float>();
    int n = buf.frameCount();   // mono → frameCount == sample count
    m_pcm.append(QVector<float>(src, src + n));

    // Emit one FFT frame per HOP samples
    while (m_pcm.size() >= FFT_N) {
        qint64 tMs = (m_samplesIn * 1000LL) / SAMPLE_RATE;
        buildFrame(tMs);
        m_samplesIn += HOP;
        m_pcm.remove(0, HOP);
    }
}

void AudioAnalyzer::buildFrame(qint64 frameMs) {
    // 1. Apply Hann window to the first FFT_N samples in m_pcm
    QVector<float> windowed(FFT_N);
    vDSP_vmul(m_pcm.constData(), 1, m_window.constData(), 1, windowed.data(), 1, FFT_N);

    // 2. Pack real signal into split-complex (vDSP_fft_zrip convention)
    //    Treat pairs (windowed[0],windowed[1]), (windowed[2],windowed[3]),...
    //    as complex numbers → FFT_N/2 complex inputs
    DSPSplitComplex split{ m_re.data(), m_im.data() };
    vDSP_ctoz(reinterpret_cast<const DSPComplex*>(windowed.constData()),
               2, &split, 1, static_cast<vDSP_Length>(FFT_N / 2));

    // 3. Real-signal FFT
    vDSP_fft_zrip(m_fftSetup, &split, 1,
                   static_cast<vDSP_Length>(LOG2N), FFT_FORWARD);

    // 4. Magnitude squared
    QVector<float> mag2(FFT_N / 2);
    vDSP_zvmags(&split, 1, mag2.data(), 1, static_cast<vDSP_Length>(FFT_N / 2));

    // 5. Map FFT bins → 64 display bars on a log-frequency scale
    //    Human hearing: 20 Hz – 20 kHz, log-spaced
    const float logMin = std::log10(20.0f);
    const float logMax = std::log10(20000.0f);

    QVector<float> bins(BINS, 0.0f);
    for (int b = 0; b < BINS; b++) {
        float fLo = std::pow(10.0f, logMin + (logMax - logMin) *  b      / BINS);
        float fHi = std::pow(10.0f, logMin + (logMax - logMin) * (b + 1) / BINS);
        int   iLo = std::clamp(static_cast<int>(fLo * FFT_N / SAMPLE_RATE), 1, FFT_N/2 - 1);
        int   iHi = std::clamp(static_cast<int>(fHi * FFT_N / SAMPLE_RATE), 1, FFT_N/2 - 1);

        float sum = 0;
        for (int i = iLo; i <= iHi; i++) sum += mag2[i];
        bins[b] = (iHi >= iLo) ? sum / (iHi - iLo + 1) : 0.0f;

        // Convert to dB, normalize -80..0 dB → 0..1
        float db = 10.0f * std::log10(bins[b] + 1e-10f);
        bins[b]  = std::clamp((db + 80.0f) / 80.0f, 0.0f, 1.0f);
    }

    m_frames.append({ frameMs, bins });
}

void AudioAnalyzer::onDecoderFinished() {
    m_busy = false;
    emit busyChanged();
    emit analysisComplete();
}

void AudioAnalyzer::seekTo(qint64 positionMs) {
    if (m_frames.isEmpty()) return;

    // Binary search for the closest pre-computed frame
    int lo = 0, hi = m_frames.size() - 1;
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        if (m_frames[mid].first < positionMs) lo = mid + 1;
        else hi = mid;
    }

    const QVector<float>& target = m_frames[lo].second;
    bool changed = false;

    for (int i = 0; i < BINS; i++) {
        float t = (i < target.size()) ? target[i] : 0.0f;
        float c = static_cast<float>(m_spectrum[i]);

        // Fast attack (0.5), slow decay (0.2) — classic visualizer feel
        float rate = (t > c) ? 0.5f : 0.2f;
        float next = c + (t - c) * rate;

        if (std::abs(next - c) > 0.001f) {
            m_spectrum[i] = static_cast<qreal>(next);
            changed = true;
        }
    }

    if (changed) emit spectrumChanged();
}
