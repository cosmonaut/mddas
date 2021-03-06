#ifndef _SPECMONBOX_H_
#define _SPECMONBOX_H_

#include <QWidget>

QT_BEGIN_NAMESPACE
class QDoubleSpinBox;
class QTimer;
class QComboBox;
QT_END_NAMESPACE
class SpectroMonitorScatterPlot;
class MDDASPlotConfig;
class MDDASDataPoint;

class SpecMonBox : public QWidget
{
    Q_OBJECT
public:
    SpecMonBox(QWidget *parent = NULL);
    void append(const QVector<MDDASDataPoint> &);
    void configure(MDDASPlotConfig);
    bool isActive();
    void replot();
                                   
public slots:
    void activate(bool);
    void setRefreshTimer(double);
    void clear();
    void updateDivisors();
    void doRebin(int);

private:
    SpectroMonitorScatterPlot *_plot;
    QDoubleSpinBox *_refreshRateSpinBox;
    QTimer *_replotTimer;
    QTimer *_refreshTimer;
    bool _active;
    QComboBox *_rebinSelector;
};

#endif // _SPECMONBOX_H_
