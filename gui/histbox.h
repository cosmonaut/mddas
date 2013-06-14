#ifndef _HISTBOX_H_
#define _HISTBOX_H_

#include <QWidget>

QT_BEGIN_NAMESPACE
class QTimer;
class QGroupBox;
QT_END_NAMESPACE
class IncrementalHistPlot;
class MDDASPlotConfig;
class MDDASDataPoint;
class NumberButton;

class HistBox : public QWidget
{
    Q_OBJECT
public:
    HistBox(QWidget *parent = NULL);
    void append(const QVector<MDDASDataPoint> &);
    void configure(MDDASPlotConfig);
    bool isActive();

public slots:
    void activate(bool);
    void clear();
    void replot();

private:
    IncrementalHistPlot *_plot;
    QTimer *_replotTimer;
    bool _active;
    QGroupBox* _binsBox;
    NumberButton* _numBins;
    NumberButton* _binSize;
};

#endif // _HISTBOX_H_
