#pragma once
#include <QWidget>
#include <vector>

class WaveView : public QWidget {
public:
    explicit WaveView(QWidget *parent = nullptr);
    void setData(const float *data, size_t size);
    const std::vector<float> &data() const { return data_; }

protected:
    void paintEvent(QPaintEvent *event);

private:
    std::vector<float> data_;
};
