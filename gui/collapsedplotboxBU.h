#ifndef _COLLPLOTBOX_H_
#define _COLLPLOTBOX_H_

#include <QHash>
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
    
    void appendPoints( const QVector<MDDASDataPoint> &);
    void clear();
    //void setRebinFactor(uint);
    
    
                                   
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
    
    QwtPlotHistogram *d_curve;
    QHash<uint, uint> *_hash;
    
    uint _x_min;
    uint _x_max;
    uint _y_min;
    uint _y_max;
    uint _x_interval;
    uint _y_interval;
    uint _rebin_f;
};

#endif // _COLLPLOTBOX_H_
