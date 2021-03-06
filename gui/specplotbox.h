#ifndef _SPECPLOTBOX_H_
#define _SPECPLOTBOX_H_

#include <QWidget>
#include <QDialog>
#include <QMap>

QT_BEGIN_NAMESPACE
class QTimer;
class QGroupBox;
class QCheckBox;
class QLineEdit;
class QComboBox;
QT_END_NAMESPACE
class SpecPlot;
class MDDASPlotConfig;
class MDDASDataPoint;
class SpecPlotConfig;
class BoxConfigBox;
//class NumberButton;

class SpecPlotBox : public QWidget
{
    Q_OBJECT
public:
    SpecPlotBox(QMap<QString, QVariant>, QWidget *parent = NULL);
    void append(const QVector<MDDASDataPoint> &);
    void configure(MDDASPlotConfig);
    bool isActive();
    QMap<QString, QVariant> getSettings(void);

public slots:
    void activate(bool);
    void clear();
    void replot();
    void alertBoxChange1(uint, uint, uint, uint);
    void alertBoxChange2(uint, uint, uint, uint);
    void alertBoxChange3(uint, uint, uint, uint);
    void configBoxes();
    void setBoxXY(uint, uint, uint);

signals:
    void boxSizeChanged1(uint, uint, uint, uint);
    void boxSizeChanged2(uint, uint, uint, uint);
    void boxSizeChanged3(uint, uint, uint, uint);
    void settingsChanged();

private slots:
    void setPrefs(void);

private:
    SpecPlot *_plot1;
    SpecPlot *_plot2;
    SpecPlot *_plot3;
    QTimer *_replotTimer;
    bool _active;
    QGroupBox *_boxConfigBox;
    SpecPlotConfig *_specPlotConf;
    QMap<QString, QVariant> *_settings;
};

class SpecPlotConfig : public QDialog {
    Q_OBJECT
public:
    SpecPlotConfig(QMap<QString, QVariant>, QWidget *parent = 0);
    void setSettings(QMap<QString, QVariant>);

public slots:
    const QMap<QString, QVariant>* getSettings();
    QMap<QString, uint> getParams(void);

signals:
    void updated();

private slots:
    void finish();

private:
    BoxConfigBox *_plot1Widget;
    BoxConfigBox *_plot2Widget;
    BoxConfigBox *_plot3Widget;
    QMap<QString, uint> *_params;
    
    QMap<QString, QVariant> *_settings;
};


class BoxConfigBox : public QWidget {
    Q_OBJECT
public:
    BoxConfigBox(QWidget *parent = NULL);
    QVector<uint> getParams(void);
    void setParams(uint, uint, uint, uint, uint);

private slots:
    void updateCombo(void);


private:
    QLineEdit *_plotX;
    QLineEdit *_plotY;
    QLineEdit *_plotW;
    QLineEdit *_plotH;
    QComboBox *_plotR;
};

#endif // _SPECPLOTBOX_H_
