#include <QtCharts/QXYSeries>

#include "measure.h"

Measure::Measure(QObject *parent) : QIODevice(parent)
{
    fftPower = 12;
    fftSize = pow(2, fftPower);

    data = (complex *)calloc(fftSize, sizeof(complex));

    dataStack = new AudioStack(fftSize);
    referenceStack = new AudioStack(fftSize);

    QAudioDeviceInfo d = QAudioDeviceInfo::defaultInputDevice();
    foreach (int c, d.supportedChannelCounts()) {
        if (c > _chanelCount)
            _chanelCount = c;
    }

    format.setSampleRate(48000);
    format.setChannelCount(_chanelCount);
    format.setSampleSize(32);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::Float);

    audio = new QAudioInput(format, this);
    open(WriteOnly);
    audio->start(this);

    //qDebug() << audio->format().channelCount();

    fft = new FFT(this);

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), SLOT(transform()));
    timer->start(40); //25 per sec
}
void Measure::setActive(bool active)
{
    _active = active;
    _level  = 0;

    emit activeChanged();
    emit levelChanged();
}
qint64 Measure::readData(char *data, qint64 maxlen)
{
    Q_UNUSED(data);
    Q_UNUSED(maxlen);

    return -1;
}

qint64 Measure::writeData(const char *data, qint64 len)
{
    Sample s;
    int currentChanel = 0;

    for (qint64 i = 0; i < len; i += 4) {
        s.c[0] = data[i];
        s.c[1] = data[i + 1];
        s.c[2] = data[i + 2];
        s.c[3] = data[i + 3];

        if (currentChanel == _dataChanel) {
            dataStack->add(s.f);
        }

        if (currentChanel == _referenceChanel) {
            referenceStack->add(s.f);
        }

        currentChanel ++;
        if (currentChanel == _chanelCount)
            currentChanel = 0;
    }
    return len;
}
void Measure::transform()
{
    _level = 0.0;

    dataStack->reset();
    for (int i = 0; i < fftSize; i++) {
        data[i] = dataStack->current();
        if (dataStack->current() > _level)
            _level = dataStack->current();

        dataStack->next();
    }

    fft->transform(data, fftSize);

    emit readyRead();
    emit levelChanged();
}
void Measure::updateRTASeries(QAbstractSeries *series)
{
    if (series) {
        QXYSeries *xySeries = static_cast<QXYSeries *>(series);

        QVector<QPointF> points;

        int i = 0;
        float startFrequency     = 6.875, //(note A)
                frequencyFactor  = pow(2, 1.0 / _pointsPerOctave),
                currentFrequency = startFrequency,
                nextFrequency    = currentFrequency * frequencyFactor,
                currentLevel     = 0.0,
                rateFactor       = audio->format().sampleRate() / fftSize,
                m, y, f
                ;
        int     currentCount     = 0;

        for (i = 0; i < fftSize / 2; i ++) {
            m = std::abs(data[i]);
            y = 20.0 * log10(m); // log10(m / 1.0); 1.0f - 0dB point
            f = i * rateFactor;
            currentCount ++;

            if (_pointsPerOctave >= 1) {

                if (f > currentFrequency + (nextFrequency - currentFrequency) / 2) {

                    y = 20.0 * log10(currentLevel / currentCount);
                    points.append(QPointF(currentFrequency, y));
                    currentLevel     = 0.0;
                    currentCount     = 0;

                    while (f > currentFrequency + (nextFrequency - currentFrequency) / 2) {
                        currentFrequency = nextFrequency;
                        nextFrequency    = currentFrequency * frequencyFactor;
                    }
                }

                currentLevel += m;
            } else {
                //without grouping by freq data
                if (f == 0)
                    f = std::numeric_limits<float>::min();
                points.append(QPointF(f, y));
            }
        }

        xySeries->replace(points);
    }
}
