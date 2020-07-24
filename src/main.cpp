#include "ui_window.h"
#include <kiss_fftr.h>
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <sndfile.hh>
#include <complex>
#include <cmath>

class App : public QApplication {
public:
    using QApplication::QApplication;
    void init();

private:
    void updateMix();
    void changeFile(int which);

    static std::vector<float> createXfadeMix(const float *a, const float *b, size_t n, float mix);
    static std::vector<float> createSpectralMix(const float *a, const float *b, size_t n, float mix);

private:
    QWidget *win_ = nullptr;
    Ui::Window ui_;
};

void App::init()
{
    setApplicationName("Wavetables");

    QWidget *win = new QWidget;
    ui_.setupUi(win);

    ///
    constexpr size_t n = 1024;
    float sine[n];
    float ramp[n];
    for (size_t i = 0; i < n; ++i) {
        double p = double(i) / double(n - 1);
        sine[i] = std::sin(2 * M_PI * p);
        auto wrap = [](double x) -> double { return x - long(x); };
        ramp[i] = wrap(p + 0.5f) * 2.0f - 1.0;
    }

    ///
    ui_.viewFileA->setData(sine, n);
    ui_.viewFileB->setData(ramp, n);
    updateMix();

    connect(
        ui_.valMixAB, &QSlider::valueChanged,
        this, &App::updateMix);

    connect(
        ui_.btnFileA, &QPushButton::clicked,
        this, [this]() { changeFile(0); });
    connect(
        ui_.btnFileB, &QPushButton::clicked,
        this, [this]() { changeFile(1); });

    ///
    win->setWindowTitle(applicationDisplayName());
    win->show();
}

static std::vector<float> resample(const std::vector<float> &in, size_t newSize)
{
    std::vector<float> out(newSize);
    size_t oldSize = in.size();

    if (oldSize < 1)
        return out;

    for (size_t i = 0; i < newSize; ++i) {
        float j = i * (float(oldSize) / float(newSize));
        size_t j1 = std::min((size_t)j, oldSize - 1);
        size_t j2 = std::min(j1 + 1, oldSize - 1);
        float mu = j - j1;
        out[i] = mu * in[j2] + (1.0f - mu) * in[j1];
    }

    return out;
}

void App::updateMix()
{
    constexpr size_t n = 1024;
    std::vector<float> dataA = resample(ui_.viewFileA->data(), n);
    std::vector<float> dataB = resample(ui_.viewFileB->data(), n);
    float mix = ui_.valMixAB->value() * 0.01f;

    std::vector<float> xfade = createXfadeMix(dataA.data(), dataB.data(), n, mix);
    std::vector<float> spectral = createSpectralMix(dataA.data(), dataB.data(), n, mix);

    ui_.viewXfadeMix->setData(xfade.data(), xfade.size());
    ui_.viewSpectralMix->setData(spectral.data(), spectral.size());
}

void App::changeFile(int which)
{
    QString filename = QFileDialog::getOpenFileName(win_, tr("Load sound file"));

    if (filename.isEmpty())
        return;

    SndfileHandle snd(filename.toStdString().c_str());
    if (!snd) {
        QMessageBox::warning(win_, tr("Error loading"), tr("Cannot open the selected file as audio."));
        return;
    }
    if (snd.channels() != 1) {
        QMessageBox::warning(win_, tr("Error loading"), tr("The audio file must contain exactly one channel."));
        return;
    }

    sf_count_t frames = snd.frames();
    std::vector<float> data(frames);
    snd.read(data.data(), frames);

    switch (which) {
    case 0:
        ui_.viewFileA->setData(data.data(), data.size());
        break;
    case 1:
        ui_.viewFileB->setData(data.data(), data.size());
        break;
    }

    updateMix();
}

std::vector<float> App::createXfadeMix(const float *a, const float *b, size_t n, float mix)
{
    std::vector<float> ab(n);
    for (size_t i = 0; i < n; ++i)
        ab[i] = a[i] * (1 - mix) + b[i] * mix;
    return ab;
}

std::vector<float> App::createSpectralMix(const float *a, const float *b, size_t n, float mix)
{
    typedef std::complex<float> cfloat;
    static_assert(sizeof(cfloat) == sizeof(kiss_fft_cpx), "kiss FFT complex type not matching");

    std::vector<float> ab(n);
    std::vector<cfloat> cpxA(n / 2 + 1);
    std::vector<cfloat> cpxB(n / 2 + 1);

    kiss_fftr_cfg forward = kiss_fftr_alloc(n, 0, nullptr, nullptr);
    kiss_fftr(forward, a, (kiss_fft_cpx *)cpxA.data());
    kiss_fftr(forward, b, (kiss_fft_cpx *)cpxB.data());
    for (size_t i = 0; i < n / 2 + 1; ++i) {
        cpxA[i] *= 1.0f / n;
        cpxB[i] *= 1.0f / n;
    }
    kiss_fftr_free(forward);

    for (size_t i = 0; i < n / 2 + 1; ++i) {
        cfloat binA = cpxA[i];
        cfloat binB = cpxB[i];

        float magA = std::abs(binA);
        float magB = std::abs(binB);
        float phaseA = std::arg(binA);
        float phaseB = std::arg(binB);

#if 1
        float magAB = magA * (1 - mix) + magB * mix;
        float phaseAB = phaseA * (1 - mix) + phaseB * mix;
        cfloat bin = std::polar(magAB, phaseAB);
#else
        cfloat bin = binA * (1 - mix) + binB * mix;
#endif

        cpxA[i] = bin;
    }

    kiss_fftr_cfg backward = kiss_fftr_alloc(n, 1, nullptr, nullptr);
    kiss_fftri(backward, (const kiss_fft_cpx *)cpxA.data(), ab.data());

    return ab;
}

int main(int argc, char *argv[])
{
    App app(argc, argv);
    app.init();
    return app.exec();
}
