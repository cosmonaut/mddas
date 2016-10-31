#ifndef _COLLPLOTBOX_H_
#define _COLLPLOTBOX_H_

#include <QWidget>

QT_BEGIN_NAMESPACE
class QDoubleSpinBox;
class QTimer;
class QComboBox;
QT_END_NAMESPACE
class SpecPlot;
class MDDASPlotConfig;
class MDDASDataPoint;

class CollPlotBox : public QWidget
{
    Q_OBJECT
public:
    CollPlotBox(QWidget *parent = NULL);
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
    SpecPlot *_plot;
    QTimer *_replotTimer;
    QTimer *_refreshTimer;
    bool _active;
    QComboBox *_rebinSelector;
};

#endif // _COLLPLOTBOX_H_
