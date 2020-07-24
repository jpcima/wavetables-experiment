#include "waveview.h"
#include <QPainter>
#include <QPainterPath>

WaveView::WaveView(QWidget *parent)
    : QWidget(parent)
{
}

void WaveView::setData(const float *data, size_t size)
{
    data_.assign(data, data + size);
    repaint();
}

void WaveView::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    int w = width();
    int h = height();

    const float *data = data_.data();
    size_t size = data_.size();

    if (size == 0) {
        static const float zerodata[1] = {};
        data = zerodata;
        size = 1;
    }

    painter.fillRect(0, 0, w, h, Qt::black);

    painter.setPen(QPen(Qt::gray, 1.0));
    painter.drawLine(0, h * 0.5f, w - 1, h * 0.5f);

    auto yFromValue = [h](float v) -> float {
        return (1.0f - v) * (h * 0.5f);
    };
    auto lerpValue = [data, size](float index) -> float {
         size_t i1 = std::min((size_t)index, size - 1);
         size_t i2 = std::min(i1 + 1, size - 1);
         float mu = index - i1;
         return mu * data[i2] + (1.0f - mu) * data[i1];
    };

    QPainterPath path;
    path.moveTo(0.0, yFromValue(data[0]));
    for (int x = 1; x < w; ++x) {
        qreal k = qreal(size - 1) / qreal(w - 1);
        path.lineTo(x, yFromValue(lerpValue(x * k)));
    }
    painter.setPen(QPen(Qt::red, 2.0));
    painter.drawPath(path);
}
