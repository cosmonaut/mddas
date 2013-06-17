#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <stdint.h>

QT_BEGIN_NAMESPACE
class QAction;
class QActionGroup;
class QLabel;
class QMenu;
class QPluginLoader;
class QTimer;
class QErrorMessage;
QT_END_NAMESPACE
class MyToolBar;
class SpecMonBox;
class SpecBox;
class HistBox;
class MDDASPlotConfig;
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

private slots:
    bool listPlugins();
    void setPlugin(const QString &);
    void threadStartPause(bool);
    bool unloadPlugin();
    void appendData();
    void toggleAcq(bool);
    void clearPlots();
    void dispCountRate();

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

    StatusNumber *_countRateDisp;
    StatusNumber *_totalCountsDisp;
    StatusNumber *_timeLeftDisp;
    NumberButton *_expTimeDisp;
    NumberButton *_expCountsDisp;


    uint64_t _totalCounts;
    int _count;
    int _rcount;
    int _avgcount;
};

#endif
