#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTime>
#include <stdint.h>

QT_BEGIN_NAMESPACE
class QAction;
class QActionGroup;
class QLabel;
class QMenu;
class QPluginLoader;
class QTimer;
class QErrorMessage;
class QGroupBox;
QT_END_NAMESPACE
class MyToolBar;
class SpecMonBox;
class SpecBox;
class HistBox;
class MDDASPlotConfig;
class MDDASDataPoint;
class SamplingThreadInterface;
class StatusNumber;
class NumberButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();

protected:

signals:
    void acquisitionRun(bool);

private slots:
    bool listPlugins();
    void setPlugin(const QString &);
    void threadStartPause(bool);
    bool unloadPlugin();
    void appendData();
    void toggleAcq(bool);
    void clearPlots();
    void dispCountRate();
    void saveFits();

private:
    void createActions();
    void createMenus();
    bool loadPlugin(QString);
    void configurePlots();

    QToolBar *toolBar();
    QToolBar *plotToolBar();
    QToolBar *statToolBar();

    QLabel *_infoLabel;

    QMenu *fileMenu;
    QMenu *acqMenu;

    QAction *exitAct;
    QAction *acqLoadAct;
    QAction *unloadAction;
    QAction *d_monitorAction;
    QAction *_acquireAction;
    QAction *_saveAction;
    QAction *_clearAction;
    QAction *_specMonAction;
    QAction *_specAction;
    QAction *_histAction;

    /* Sampling Thread Plugin stuff */
    SamplingThreadInterface *sti;
    QVector<QString> pluginsList;
    QString loadedPlugin;
    QPluginLoader *pluginLoader;

    /* Toolbars */
    MyToolBar *_acqtb;
    MyToolBar *_plottb;
    MyToolBar *_stattb;

    QList<QAction *> *_plotActionList;
    MDDASPlotConfig *_pc;

    /* Plot Widgets */
    SpecMonBox *_specMon;
    SpecBox *_spec;
    HistBox *_hist;

    QTimer *_acqTimer;
    QTimer *_rateTimer;
    QTime _expTime;

    StatusNumber *_countRateDisp;
    StatusNumber *_totalCountsDisp;
    StatusNumber *_timeLeftDisp;
    NumberButton *_expTimeDisp;
    NumberButton *_expCountsDisp;

    QGroupBox *_expGroup;

    QDateTime _obsUTCTimeStamp;

    uint16_t _acqMode;
    uint64_t _totalCounts;
    int _count;
    int _rcount;
    int _avgcount;
    /* Stored exposure time */
    double _expTimeTotal;

    QVector<MDDASDataPoint> *_mddasData;
    //QHash<uint, double> *_mddasTimeData;
    /* Use a QMap because it is sorted by key. This is a key:value
       pair of position:time for the _mddasData. */
    QMap<uint, double> *_mddasTimeData;
};

#endif
