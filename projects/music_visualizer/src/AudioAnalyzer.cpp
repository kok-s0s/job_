#include "AudioAnalyzer.h"
#include "NcmDecoder.h"
#include <QAudioFormat>
#include <QFileInfo>
#include <QUrl>
#include <cmath>
#include <algorithm>
#include <numeric>

static constexpr int   SAMPLE_RATE = 44100;
static constexpr float LOG2N       = 11.0f;

AudioAnalyzer::AudioAnalyzer(QObject* parent)
    : QObject(parent)
    , m_decoder(new QAudioDecoder(this))
{
    QAudioFormat fmt;
    fmt.setSampleRate(SAMPLE_RATE);
    fmt.setChannelCount(1);
    fmt.setSampleFormat(QAudioFormat::Float);
    m_decoder->setAudioFormat(fmt);

    connect(m_decoder, &QAudioDecoder::bufferReady, this, &AudioAnalyzer::onBufferReady);
    connect(m_decoder, &QAudioDecoder::finished,    this, &AudioAnalyzer::onDecoderFinished);

    m_fftSetup = vDSP_create_fftsetup(static_cast<vDSP_Length>(LOG2N), FFT_RADIX2);
    m_window.resize(FFT_N);
    vDSP_hann_window(m_window.data(), FFT_N, vDSP_HANN_NORM);
    m_re.resize(FFT_N / 2);
    m_im.resize(FFT_N / 2);

    m_spectrum.resize(BINS,     0.0);
    m_waveform.resize(WAVE_PTS, 0.0);
}

AudioAnalyzer::~AudioAnalyzer() { vDSP_destroy_fftsetup(m_fftSetup); }

void AudioAnalyzer::analyzeFile(const QUrl& url) {
    m_frames.clear();
    m_pcm.clear();
    m_samplesIn = 0;
    m_busy = true;
    emit busyChanged();
    m_spectrum.fill(0.0);
    m_waveform.fill(0.0);
    emit spectrumChanged();
    // Transparently handle NetEase Cloud Music encrypted files
    QUrl src = url;
    if (url.toLocalFile().toLower().endsWith(".ncm")) {
        QString decoded = NcmDecoder::decode(url.toLocalFile());
        if (!decoded.isEmpty()) src = QUrl::fromLocalFile(decoded);
    }

    m_decoder->setSource(src);
    m_decoder->start();
}

void AudioAnalyzer::onBufferReady() {
    while (m_decoder->bufferAvailable())
        processBuffer(m_decoder->read());
}

void AudioAnalyzer::processBuffer(const QAudioBuffer& buf) {
    const float* src = buf.constData<float>();
    int n = buf.frameCount();
    m_pcm.append(QVector<float>(src, src + n));
    while (m_pcm.size() >= FFT_N) {
        buildFrame((m_samplesIn * 1000LL) / SAMPLE_RATE);
        m_samplesIn += HOP;
        m_pcm.remove(0, HOP);
    }
}

void AudioAnalyzer::buildFrame(qint64 frameMs) {
    // ── FFT ──────────────────────────────────────────────────────────────
    QVector<float> windowed(FFT_N);
    vDSP_vmul(m_pcm.constData(), 1, m_window.constData(), 1, windowed.data(), 1, FFT_N);

    DSPSplitComplex split{ m_re.data(), m_im.data() };
    vDSP_ctoz(reinterpret_cast<const DSPComplex*>(windowed.constData()),
               2, &split, 1, static_cast<vDSP_Length>(FFT_N / 2));
    vDSP_fft_zrip(m_fftSetup, &split, 1,
                   static_cast<vDSP_Length>(LOG2N), FFT_FORWARD);

    QVector<float> mag2(FFT_N / 2);
    vDSP_zvmags(&split, 1, mag2.data(), 1, static_cast<vDSP_Length>(FFT_N / 2));

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
        float db = 10.0f * std::log10((iHi >= iLo ? sum / (iHi - iLo + 1) : 0.0f) + 1e-10f);
        bins[b] = std::clamp((db + 80.0f) / 80.0f, 0.0f, 1.0f);
    }

    // ── Waveform: downsample FFT_N → WAVE_PTS ────────────────────────────
    QVector<float> wave(WAVE_PTS);
    int chunk = FFT_N / WAVE_PTS;
    for (int i = 0; i < WAVE_PTS; i++) {
        float s = 0;
        for (int j = 0; j < chunk; j++) s += m_pcm[i * chunk + j];
        wave[i] = std::clamp(s / chunk, -1.0f, 1.0f);
    }

    m_frames.append({ frameMs, bins, wave });
}

void AudioAnalyzer::onDecoderFinished() {
    m_busy = false;
    emit busyChanged();
    emit analysisComplete();
}

void AudioAnalyzer::seekTo(qint64 positionMs) {
    if (m_frames.isEmpty()) return;

    int lo = 0, hi = m_frames.size() - 1;
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        if (m_frames[mid].timeMs < positionMs) lo = mid + 1;
        else hi = mid;
    }

    const Frame& f = m_frames[lo];
    bool changed = false;

    for (int i = 0; i < BINS; i++) {
        float t = (i < f.spec.size()) ? f.spec[i] : 0.0f;
        float c = static_cast<float>(m_spectrum[i]);
        float rate = (t > c) ? 0.5f : 0.2f;
        float next = c + (t - c) * rate;
        if (std::abs(next - c) > 0.001f) { m_spectrum[i] = next; changed = true; }
    }
    for (int i = 0; i < WAVE_PTS; i++) {
        float t = (i < f.wave.size()) ? f.wave[i] : 0.0f;
        float c = static_cast<float>(m_waveform[i]);
        float next = c * 0.4f + t * 0.6f;
        if (std::abs(next - c) > 0.001f) { m_waveform[i] = next; changed = true; }
    }

    if (changed) emit spectrumChanged();
}
