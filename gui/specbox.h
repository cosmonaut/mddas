#ifndef _SPECBOX_H_
#define _SPECBOX_H_

#include <QWidget>

QT_BEGIN_NAMESPACE
class QDoubleSpinBox;
class QTimer;
class QComboBox;
QT_END_NAMESPACE
class SpectroScatterPlot;
class MDDASPlotConfig;
class MDDASDataPoint;

class SpecBox : public QWidget
{
    Q_OBJECT
public:
    SpecBox(QWidget *parent = NULL);
    void append(const QVector<MDDASDataPoint> &);
    void configure(MDDASPlotConfig);
    bool isActive();
    void replot();
                                   
public slots:
    void activate(bool);
    //void setRefreshTimer(double);
    void clear();
    void updateDivisors();
    void doRebin(int);
    void setBox1(uint, uint, uint, uint);
    void setBox2(uint, uint, uint, uint);
    void setBox3(uint, uint, uint, uint);

private:
    SpectroScatterPlot *_plot;
    //QDoubleSpinBox *_refreshRateSpinBox;
    QTimer *_replotTimer;
    //QTimer *_refreshTimer;
    bool _active;
    QComboBox *_cmSelector;
    QComboBox *_rebinSelector;
};

#endif // _SPECBOX_H_
