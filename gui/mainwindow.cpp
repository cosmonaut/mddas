#include <QtGui>

#include "samplingthreadinterface.h"
#include "fits.h"
#include "mddasconfigdialog.h"
#include "mddasplotconfig.h"
#include "mddasdatapoint.h"
#include "specmonbox.h"
#include "specbox.h"
#include "histbox.h"
#include "specplotbox.h"
#include "numberbutton.h"
#include "mainwindow.h"
#include "atomic.xpm"
#include "eye.xpm"
#include "save.xpm"
#include "xicon.xpm"

#define MDDAS_PLUGIN_PATH "../mddasplugins"

class MyToolBar: public QToolBar {
public:
    MyToolBar(MainWindow *parent):
        QToolBar(parent)
    {}
    void addSpacing(int spacing) {
        QLabel *label = new QLabel(this);
        addWidget(label);
        label->setFixedWidth(spacing);
    }    
};

class StatusNumber: public QWidget
{
public:
    StatusNumber(QWidget *parent, const QString &prefix, double val):
        QWidget(parent)
    {
        QHBoxLayout *layout = new QHBoxLayout(this);
        layout->setContentsMargins(QMargins(0,0,0,0));

        if (!prefix.isEmpty()) {
            label = new QLabel(prefix + " ", this);
            //layout->addWidget(new QLabel(prefix + " ", this));
            layout->addWidget(label);
        } else {
            label = new QLabel("", this);
            layout->addWidget(label);
        }
            
        d_number = new QLabel(this);
        d_number->setNum(val);

        layout->addWidget(d_number);
        layout->setAlignment(label, Qt::AlignLeft);
        layout->setAlignment(d_number, Qt::AlignRight);
    }

    void setNum(double value, char format = 'g', int precision = 6) { 
        QString lab_str;
        lab_str.setNum(value, format, precision);
        d_number->setText(lab_str);
    }

private:
    QLabel *label;
    QLabel *d_number;
};

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent) {
    loadedPlugin = "";

    /* Configuration for the plots */
    _pc = new MDDASPlotConfig();
    //_pc->getXMax();

    _mddasData = new QVector<MDDASDataPoint>();
    //_mddasTimeData = new QHash<uint, double>();
    //_mddasTimeData = new QMap<uint, double>();
    _mddasTimeData = new QVector<double>;

    QWidget *widget = new QWidget(this);
    setCentralWidget(widget);

    qDebug() << "Creating settings...";

    _defaultSettings = new QMap<QString, QVariant>();
    _currentSettings = new QMap<QString, QVariant>();
    //_defaultSettings->insert("fitsImage", true);
    createDefaultSettings();
    (*_currentSettings) = (*_defaultSettings);

    _configDialog = new MDDASConfigDialog(this);
    //_configDialog->setDefaults((*_defaultSettings));
    //_configDialog->setSettings((*_currentSettings));
    

    _mddasSettings = new QSettings(QSettings::NativeFormat, QSettings::UserScope, "mddas", "default", this);
    //_mddasSettings = new QSettings(_settingsFileName, QSettings::NativeFormat, this);
    _mddasSettings->sync();
    
    if (_mddasSettings->status() != 0) {
        qDebug() << "Error reading config file, creating new one";
        makeDefaultConf();
        // create default conf;
    }

    if (_defaultSettings->size() == _mddasSettings->allKeys().size()) {
        for (int i = 0; i < _mddasSettings->allKeys().size(); i++) {
            if (!(_defaultSettings->keys().contains(_mddasSettings->allKeys()[i]))) {
                // create default conf
                qDebug() << "bad key, setting defaults";
                makeDefaultConf();
                break;
            }
        }
    } else {
        // create default conf
        qDebug() << "no conf file, creating default";
        makeDefaultConf();
    }
    
    *_currentSettings = getMapFromSettings();

    _configDialog->setDefaults(*_defaultSettings);
    _configDialog->setSettings(*_currentSettings);
    
    // qDebug() << _mddasSettings->allKeys();
    // qDebug() << _mddasSettings->fileName();
    // qDebug() << _mddasSettings->scope();
    // qDebug() << _mddasSettings->status();

    /* List of plugins */
    pluginsList = QVector<QString>(0);

    /* Holds and controls the monitor widget */
    _specMon = new SpecMonBox(this);
    _specMon->setVisible(false);

    /* Holds and controls the spectrogram widget */
    _spec = new SpecBox(this);
    _spec->setVisible(false);

    _hist = new HistBox(this);
    _hist->setVisible(false);

    _specPlot = new SpecPlotBox((*_currentSettings), this);
    _specPlot->setVisible(false);

    /* Timer for reading data from the sampling thread */
    _acqTimer = new QTimer(this);
    _acqTimer->setInterval(0);
    _acqTimer->setSingleShot(false);
    _acqTimer->stop();
    
    /* Timer to determine count rate */
    _rateTimer = new QTimer(this);
    _rateTimer->setInterval(100);
    _rateTimer->setSingleShot(false);
    _rateTimer->stop();

    _count = 0;
    _rcount = 0;
    _avgcount = 0;

    /* top space filler */
    QWidget *topFiller = new QWidget;
    topFiller->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    _infoLabel = new QLabel(tr("Choose an acquisition plugin"));
    _infoLabel->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    _infoLabel->setAlignment(Qt::AlignCenter);

    QWidget *bottomFiller = new QWidget;
    bottomFiller->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QWidget *sideFiller = new QWidget;
    sideFiller->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Ignored);

    /* Layout for the plot widgets */
    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->addWidget(_specMon);
    hbox->addWidget(_spec);
    hbox->addWidget(_hist);
    hbox->addWidget(_specPlot);
    hbox->addWidget(sideFiller);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(5);
    layout->addWidget(topFiller);
    layout->addLayout(hbox);
    layout->addWidget(_infoLabel);
    layout->addWidget(bottomFiller);
    widget->setLayout(layout);

    createActions();
    createMenus();

    QString message = tr("STATUS");
    statusBar()->showMessage(message);    

    addToolBar(toolBar());
    addToolBarBreak();
    addToolBar(plotToolBar());
    addToolBar(Qt::BottomToolBarArea, statToolBar());

    connect(d_monitorAction, SIGNAL( toggled(bool) ), this, SLOT( threadStartPause(bool) ));
    connect(d_monitorAction, SIGNAL( toggled(bool) ), unloadAction, SLOT( setDisabled(bool) ));
    connect(d_monitorAction, SIGNAL( toggled(bool) ), _plottb, SLOT( setDisabled(bool) ));
    connect(d_monitorAction, SIGNAL( toggled(bool) ), _specMon, SLOT( activate(bool) ));
    connect(d_monitorAction, SIGNAL( toggled(bool) ), _spec, SLOT( activate(bool) ));
    connect(d_monitorAction, SIGNAL( toggled(bool) ), _hist, SLOT( activate(bool) ));
    connect(d_monitorAction, SIGNAL( toggled(bool) ), _specPlot, SLOT( activate(bool) ));
    connect(d_monitorAction, SIGNAL( toggled(bool) ), _clearAction, SLOT( setDisabled(bool) ));
    connect(d_monitorAction, SIGNAL( toggled(bool) ), _acquireAction, SLOT( setDisabled(bool) ));

    connect(_acquireAction, SIGNAL( toggled(bool) ), d_monitorAction, SLOT( setDisabled(bool) ));
    connect(_acquireAction, SIGNAL( toggled(bool) ), _clearAction, SLOT( setDisabled(bool) ));
    connect(_acquireAction, SIGNAL( toggled(bool) ), _plottb, SLOT( setDisabled(bool) ));
    connect(_acquireAction, SIGNAL( toggled(bool) ), unloadAction, SLOT( setDisabled(bool) ));

    connect(_acquireAction, SIGNAL( toggled(bool) ), this, SLOT( threadStartPause(bool) ));
    connect(_acquireAction, SIGNAL( toggled(bool) ), _specMon, SLOT( activate(bool) ));
    connect(_acquireAction, SIGNAL( toggled(bool) ), _spec, SLOT( activate(bool) ));
    connect(_acquireAction, SIGNAL( toggled(bool) ), _hist, SLOT( activate(bool) ));
    connect(_acquireAction, SIGNAL( toggled(bool) ), _specPlot, SLOT( activate(bool) ));
    connect(_acquireAction, SIGNAL( toggled(bool) ), this, SLOT( toggleAcq(bool) ));

    connect(_acqTimer, SIGNAL( timeout() ), this, SLOT( appendData() ));
    connect(d_monitorAction, SIGNAL( toggled(bool) ), this, SLOT( toggleAcq(bool) ));
    connect(_clearAction, SIGNAL( triggered() ), this, SLOT( clearPlots() ));
    connect(_rateTimer, SIGNAL( timeout() ), this, SLOT( dispCountRate() ));

    connect(_specPlot, SIGNAL( boxSizeChanged1(uint, uint, uint, uint) ), _spec, SLOT( setBox1(uint, uint, uint, uint) ));
    connect(_specPlot, SIGNAL( boxSizeChanged2(uint, uint, uint, uint) ), _spec, SLOT( setBox2(uint, uint, uint, uint) ));
    connect(_specPlot, SIGNAL( boxSizeChanged3(uint, uint, uint, uint) ), _spec, SLOT( setBox3(uint, uint, uint, uint) ));
    connect(_specPlot, SIGNAL( settingsChanged() ), this, SLOT( updateSettingsFromPlot() ));

    connect(_saveAction, SIGNAL( triggered() ), this, SLOT( saveFits() ));

    //connect(_prefAct, SIGNAL( triggered() ), _configDialog, SLOT( exec() ));
    connect(_prefAct, SIGNAL( triggered() ), this, SLOT( setPrefs() ));

    //connect(this, SIGNAL( acquisitionRun(bool) ), this, SLOT( toggleAcq(bool) ));
    connect(this, SIGNAL( acquisitionRun(bool) ), _acquireAction, SLOT( setChecked(bool) ));

    pluginLoader = new QPluginLoader();

    setWindowTitle(tr("MDDAS"));
    setMinimumSize(800, 600);
    resize(800, 600);
}

MainWindow::~MainWindow() {
    unloadPlugin();
}

void MainWindow::toggleAcq(bool b) {
    int n = 0;
    QString teststr;
    if (b) {
        _expGroup->setEnabled(false);

        clearPlots();
        _acqTimer->start();
        _rateTimer->start();
        /* Start this regardless of mode to get exposure time */
        _expTime.start();

        _obsUTCTimeStamp = QDateTime::currentDateTimeUtc();

        if (d_monitorAction->isChecked()) {
            _acqMode = 0;
        } else {
            if (_expTimeDisp->isChecked()) {
                _acqMode = 1;
            }
            
            if (_expCountsDisp->isChecked()){
                _acqMode = 2;
            }
        }
        qDebug() << "start acq";

    } else {
        _rateTimer->stop();
        _acqTimer->stop();

        _expTimeTotal = (double)_expTime.elapsed()/1000.0;

        _totalCountsDisp->setNum(_totalCounts);

        n = sti->bufCount();
        teststr.setNum(n);
        qDebug() << "packets left: " << teststr;
        qDebug() << "time elapsed: " << _expTime.elapsed()/1000;
        qDebug() << "total counts: " << _totalCounts;
        _expGroup->setEnabled(true);

        if (_acqMode > 0) {
            _saveAction->setEnabled(true);
        }
    }
}

void MainWindow::appendData() {
    int n = 0;
    QString db;
    double t = 0.0;
    //int i = 0;
    n = sti->bufCount();
    if (n > 0) {
        for (int i = 0; i < n; i++) {
            QVector<MDDASDataPoint> v = sti->bufDequeue();
            
            /* Store data in acquisition mode */
            if (_acquireAction->isChecked()) {
                //_mddasTimeData->insert(_totalCounts, (double)_expTime.elapsed()/1000.0);
                t = (double)_expTime.elapsed()/1000.0;
                //(*_mddasData) += v;
                for (int n = 0; n < v.size(); n++) {
                    _mddasData->append(v.value(n));
                    _mddasTimeData->append(t);
                }
            }

            _count += v.size();
            _totalCounts += v.size();

            if (_specMon->isVisible()) {
                _specMon->append(v);
            }

            if (_spec->isVisible()) {
                _spec->append(v);
            }

            if (_hist->isVisible()) {
                _hist->append(v);
            }

            if (_specPlot->isVisible()) {
                _specPlot->append(v);
            }


        }
    }

    /* acqusition mode decision tree */
    switch(_acqMode) {
    case 0:
        break;
    case 1:
        if ((_expTime.elapsed()/1000) >= _expTimeDisp->value()) {
            //qDebug() << "elapse over";
            emit acquisitionRun(false);
        }
        break;
    case 2:

        if (_totalCounts >= _expCountsDisp->value()) {
            //qDebug() << "counts over!";
            emit acquisitionRun(false);
        }
        break;
    default:
        break;
    }
}

/* Display count rate */
void MainWindow::dispCountRate() {
    if (_acquireAction->isChecked()) {
        if (_expTimeDisp->isChecked()) {
            //qDebug() << "elapsed: " << _expTime.elapsed();
            _timeLeftDisp->setNum(_expTimeDisp->value() - ((double)_expTime.elapsed()/1000.0), 'f', 1);
        }
    }

    _totalCountsDisp->setNum(_totalCounts);

    //_avgcount += (double)(((double)_count)*10.0);
    _avgcount += _count;
    _rcount += 1;
    _count = 0;

    if (_rcount > 9) {
        //_infoLabel->setNum((double)_avgcount/10.0);
        //qDebug() << "_avgcount: " << _avgcount;
        //_countRateDisp->setNum((double)_avgcount/10.0, 'f', 1);
        _countRateDisp->setNum(_avgcount);
        _count = 0;
        _rcount = 0;
        _avgcount = 0;
    }
}

/* Clear all plot widgets */
void MainWindow::clearPlots() {
    /* clear plots */
    _specMon->clear();
    _spec->clear();
    _hist->clear();
    _specPlot->clear();

    /* There is nothing to save now */
    _saveAction->setEnabled(false);

    /* Clear stored memory */
    //qDebug() << "sizes: " << _mddasData->size() << " " << _mddasTimeData->size();
    // QMapIterator<uint, double> i((*_mddasTimeData));
    // while(i.hasNext()) {
    //     i.next();
    //     qDebug() << i.key() << ": " << i.value();
    // }
    _mddasData->clear();
    _mddasData->squeeze();

    _mddasTimeData->clear();
    _mddasTimeData->squeeze();

    /* Clear count rate and total counts info */
    _totalCountsDisp->setNum(0);
    _totalCounts = 0;
    _count = 0;
    _rcount = 0;
    _avgcount = 0;
    _countRateDisp->setNum(0);
}

/* Start or pause the sampling thread */
void MainWindow::threadStartPause(bool sp) {
    if (sp) {
        sti->pause(false);
    } else {
        sti->pause(true);
    }
}

void MainWindow::saveFits() {
    qDebug() << "Saving fits";
    if (writeFitsImage(_mddasData,
                       _mddasTimeData,
                       _obsUTCTimeStamp,
                       _expTimeTotal,
                       _pc,
                       _currentSettings) == 0) {
        _saveAction->setEnabled(false);
    } else {
        qDebug() << "Error saving fits file";
    }
}

/* Primary toolbar */
QToolBar *MainWindow::toolBar() {
    _acqtb = new MyToolBar(this);

    _acqtb->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    _acqtb->setMovable(false);
    _acqtb->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    /* Set font size for toolbar */
    QFont font = _acqtb->font();
    font.setPointSize(8);
    _acqtb->setFont(font);

    unloadAction = new QAction(QIcon(xicon_xpm), "Unload", _acqtb);
    unloadAction->setEnabled(false);

    d_monitorAction = new QAction(QIcon(eye_xpm), "Monitor", _acqtb);
    d_monitorAction->setCheckable(true);
    d_monitorAction->setEnabled(true);

    _acquireAction = new QAction(QIcon(atomic_xpm), "Acquire", _acqtb);
    _acquireAction->setCheckable(true);
    _acquireAction->setEnabled(true);

    _saveAction = new QAction(QIcon(save_xpm), "Save", _acqtb);
    _saveAction->setCheckable(false);
    _saveAction->setEnabled(false);

    _clearAction = new QAction(QIcon(xicon_xpm), "Clear", _acqtb);
    _clearAction->setCheckable(false);
    _clearAction->setEnabled(true);

    _acqtb->addAction(d_monitorAction);
    _acqtb->addAction(_acquireAction);
    _acqtb->addAction(_saveAction);
    _acqtb->addAction(_clearAction);

    /* Move the unload button out of the damned way. */
    QWidget* tbspacer = new QWidget();
    tbspacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _acqtb->addWidget(tbspacer);
    _acqtb->addAction(unloadAction);
    
    connect(unloadAction, SIGNAL(triggered()), this, SLOT(unloadPlugin()));

    _acqtb->setEnabled(false);
    /* big icons look goofy */
    _acqtb->setIconSize(QSize(18, 18));

    return _acqtb;
}

/* Buttons to enable/disable various plot widgets */
QToolBar *MainWindow::plotToolBar() {
    _plottb = new MyToolBar(this);
    _plotActionList = new QList<QAction *>();

    _plottb->setAllowedAreas(Qt::TopToolBarArea);
    _plottb->setMovable(false);
    _plottb->setToolButtonStyle(Qt::ToolButtonTextOnly);
    
    /* Set font size for toolbar */
    QFont font = _plottb->font();
    font.setPointSize(8);
    _plottb->setFont(font);
    
    _specMonAction = new QAction("Monitor Plot", _plottb);
    _specMonAction->setCheckable(true);

    _specAction = new QAction("Spectrogram Plot", _plottb);
    _specAction->setCheckable(true);
    
    _histAction = new QAction("Histogram", _plottb);
    _histAction->setCheckable(true);

    _specPlotAction = new QAction("CHESS", _plottb);
    _specPlotAction->setCheckable(true);

    _plottb->addAction(_specMonAction);
    _plottb->addAction(_specAction);
    _plottb->addAction(_histAction);
    _plottb->addAction(_specPlotAction);

    _plotActionList->append(_specMonAction);
    _plotActionList->append(_specAction);
    _plotActionList->append(_histAction);
    _plotActionList->append(_specPlotAction);

    _plottb->setEnabled(false);
    
    connect(_specMonAction, SIGNAL( toggled(bool) ), _specMon, SLOT( setVisible(bool) ));
    connect(_specAction, SIGNAL( toggled(bool) ), _spec, SLOT( setVisible(bool) ));
    connect(_histAction, SIGNAL( toggled(bool) ), _hist, SLOT( setVisible(bool) ));
    connect(_specPlotAction, SIGNAL( toggled(bool) ), _specPlot, SLOT( setVisible(bool) ));

    return _plottb;
}

QToolBar *MainWindow::statToolBar() {
    _stattb = new MyToolBar(this);
    
    _stattb->setAllowedAreas(Qt::BottomToolBarArea);
    _stattb->setMovable(false);
    _stattb->setToolButtonStyle(Qt::ToolButtonTextOnly);
    _stattb->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QGroupBox *acqGroup = new QGroupBox(_stattb);
    acqGroup->setTitle("Acquisition");
    acqGroup->setFixedWidth(200);

    QVBoxLayout *acqLayout = new QVBoxLayout(acqGroup);
    //acqLayout->setContentsMargins(QMargins(0,0,0,0));
    
    _countRateDisp = new StatusNumber(_stattb, "count rate:", 0.0);
    _totalCountsDisp = new StatusNumber(_stattb, "total counts:", 0.0);
    _timeLeftDisp = new StatusNumber(_stattb, "time left:", 0.0);

    acqLayout->addWidget(_countRateDisp);
    acqLayout->addWidget(_totalCountsDisp);
    acqLayout->addWidget(_timeLeftDisp);

    _expGroup = new QGroupBox(_stattb);
    _expGroup->setTitle("Exposure");

    QVBoxLayout *expLayout = new QVBoxLayout(_expGroup);
    
    _expTimeDisp = new NumberButton(_expGroup, "Time:", QString::null, 1, 999999, 1);
    _expTimeDisp->setChecked(true);
    _expCountsDisp = new NumberButton(_expGroup, "Counts:", QString::null, 1, 99999999, 1);
    _expCountsDisp->setChecked(false);
    _expCountsDisp->setEnabled(false);

    expLayout->addWidget(_expTimeDisp);
    expLayout->addWidget(_expCountsDisp);

    connect(_expTimeDisp, SIGNAL( toggled(bool) ), _expCountsDisp, SLOT( setUnchecked(bool) ));
    connect(_expCountsDisp, SIGNAL( toggled(bool) ), _expTimeDisp, SLOT( setUnchecked(bool) ));
    connect(_expTimeDisp, SIGNAL( toggled(bool) ), _expCountsDisp, SLOT( setAntiEnabled(bool) ));
    connect(_expCountsDisp, SIGNAL( toggled(bool) ), _expTimeDisp, SLOT( setAntiEnabled(bool) ));

    _expTimeDisp->setValue(1200);
    _expCountsDisp->setValue(1000000);

    _stattb->addWidget(acqGroup);
    _stattb->addWidget(_expGroup);

    return _stattb;
}

void MainWindow::createActions(){
    exitAct = new QAction(tr("&Exit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

    _prefAct = new QAction("Preferences", this);
    _prefAct->setStatusTip("So, this is captain sunshine?");

    acqLoadAct = new QAction(tr("&Load Sampler"), this);
    acqLoadAct->setStatusTip(tr("Load a sampling thread"));

    connect(acqLoadAct, SIGNAL(triggered()), this, SLOT(listPlugins()));
}

void MainWindow::createMenus() {
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(_prefAct);
    fileMenu->addSeparator();

    fileMenu->addAction(exitAct);

    acqMenu = menuBar()->addMenu(tr("&Acquisition"));
    acqMenu->addAction(acqLoadAct);
}

void MainWindow::setPrefs() {
    _configDialog->exec();
    (*_currentSettings) = *(_configDialog->getSettings());
    setSettingsFromMap(*_currentSettings);
    qDebug() << "mainwin settings: " << _currentSettings->value("fitsImage");
}

void MainWindow::createDefaultSettings() {
    _defaultSettings->insert("fitsImage", true);

    _defaultSettings->insert("b1x", 0);
    _defaultSettings->insert("b1y", 0);
    _defaultSettings->insert("b1w", 0);
    _defaultSettings->insert("b1h", 2);
    _defaultSettings->insert("b1r", 1);

    _defaultSettings->insert("b2x", 0);
    _defaultSettings->insert("b2y", 0);
    _defaultSettings->insert("b2w", 0);
    _defaultSettings->insert("b2h", 2);
    _defaultSettings->insert("b2r", 1);

    _defaultSettings->insert("b3x", 0);
    _defaultSettings->insert("b3y", 0);
    _defaultSettings->insert("b3w", 0);
    _defaultSettings->insert("b3h", 2);
    _defaultSettings->insert("b3r", 1);
}

void MainWindow::makeDefaultConf() {
    qDebug() << "Creating default config";
    QList<QString> k = _defaultSettings->keys();
    for (int i = 0; i < k.size(); i++) {
        _mddasSettings->setValue(k[i], _defaultSettings->value(k[i]));
    }

    _mddasSettings->sync();
    if (_mddasSettings->status() != 0) {
        qDebug() << "error creating conf file!";
    }
}

void MainWindow::setSettingsFromMap(QMap<QString, QVariant> setting_map) {
    QList<QString> k = setting_map.keys();
    for (int i = 0; i < k.size(); i++) {
        _currentSettings->insert(k[i], setting_map[k[i]]);
        _mddasSettings->setValue(k[i], setting_map.value(k[i]));
        _mddasSettings->sync();
    }
}

QMap<QString, QVariant> MainWindow::getMapFromSettings() {
    QMap<QString, QVariant> temp_map;
    QList<QString> k = _mddasSettings->allKeys();
    for (int i = 0; i < k.size(); i++) {
        temp_map.insert(k[i], _mddasSettings->value(k[i]));
    }

    return temp_map;
}

/* Function to set CHESS specplot settings */
void MainWindow::updateSettingsFromPlot() {
    QMap<QString, QVariant> m;

    m = _specPlot->getSettings();
    setSettingsFromMap(m);
}

/* Load a sampling plugin */
bool MainWindow::loadPlugin(QString pluginFileName) {
    qDebug() << "CALLED LOAD PLUGIN" << endl;

    QDir pluginsDir(qApp->applicationDirPath());

#if defined(Q_OS_WIN)
    if (pluginsDir.dirName().toLower() == "debug" || pluginsDir.dirName().toLower() == "release")
        pluginsDir.cdUp();
#elif defined(Q_OS_MAC)
    if (pluginsDir.dirName() == "MacOS") {
        pluginsDir.cdUp();
        pluginsDir.cdUp();
        pluginsDir.cdUp();
    }
#endif
    /* TODO: Make this configurable or makefile config-able */
    //pluginsDir.cd("../detconplugins");
    pluginsDir.cd(MDDAS_PLUGIN_PATH);
    //qDebug() << pluginsDir.entryList(QDir::Files);

    //QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(pluginFileName));
    //pluginLoader = new QPluginLoader(pluginsDir.absoluteFilePath(pluginFileName));
    pluginLoader->setFileName(pluginsDir.absoluteFilePath(pluginFileName));
    
    if (!pluginLoader->isLoaded()) {
        QObject *plugin = pluginLoader->instance();
        if (plugin) {
            qDebug() << "plugin loading";
            sti = qobject_cast<SamplingThreadInterface *>(plugin);
            if (sti) {
                //isPluginLoaded = true;
                loadedPlugin = pluginFileName;
                return true;
            } else {
                qDebug() << "failed to load plugin";
            }
        } else {
            qDebug() << pluginLoader->errorString();
        }
    } 

    return false;
}

/* Unload currently loaded plugin */
bool MainWindow::unloadPlugin() {
    if (pluginLoader->isLoaded()) {
        if (pluginLoader->unload()) {
            qDebug() << "unloaded plugin!";
            _infoLabel->setText("Choose an acquisition plugin");

            _acqtb->setEnabled(false);
            
            acqLoadAct->setEnabled(true);
            _plottb->setEnabled(false);
            /* Uncheck all the plot buttons */
            QList<QAction *>::iterator k;
            for (k = _plotActionList->begin(); k != _plotActionList->end(); ++k) {
                (*k)->setChecked(false);
            }
            return true;
        } else {
            qDebug() << "Failed to unload plugin!";
        }
    }

    return false;
}

/* Show all plugins in the designated plugin directory */
//bool MainWindow::listPlugins() {
void MainWindow::listPlugins() {
    qDebug() << "Listing Plugins";
    pluginsList.clear();

    QDir pluginsDir(qApp->applicationDirPath());
    //pluginsDir.cd("../detconplugins");
    pluginsDir.cd(MDDAS_PLUGIN_PATH);
    foreach (QString fileName, pluginsDir.entryList(QDir::Files)) {
        if (QLibrary::isLibrary(fileName)) {
            pluginsList.append(fileName);
        }
    }

    qDebug() << pluginsList;

    QStringList strPlugins;
    foreach(QString pstr, pluginsList) {
        strPlugins << pstr;
    }

    QInputDialog ip(this);
    ip.setOption(QInputDialog::UseListViewForComboBoxItems, true);
    ip.setComboBoxItems(strPlugins);
    connect(&ip, SIGNAL(textValueSelected(const QString &)), this, SLOT(setPlugin(const QString &)));

    ip.exec();
    //qDebug() << "listplugins done, returning";
    //return true;
}

/* Run plugin and configure plot widgets */
void MainWindow::setPlugin(const QString &pluginStr) {
    qDebug() << pluginStr;

    if (loadPlugin(pluginStr)) {
        _infoLabel->setText(pluginStr);

        qDebug() << "About to start sampling...";
        
        _acqtb->setEnabled(true);
        
        //QString test;
        //sti->getPlotConfig();
        //MDDASPlotConfig temppc = *(sti->getPlotConfig());
        *_pc = *(sti->getPlotConfig());
        configurePlots();

        //test.setNum(_pc->getXMax());
        //qDebug() << "mainwin xmax: " << test;
        //test.setNum(_pc->getYMax());
        //qDebug() << "mainwin ymax: " << test;

        /* Start thread sampling */
        sti->sample();

        acqLoadAct->setEnabled(false);
        unloadAction->setEnabled(true);
        _plottb->setEnabled(true);

        _specMonAction->setChecked(true);
        _specAction->setChecked(true);

        qDebug() << "Started thread";
        return;
    }

    _infoLabel->setText("failed to load plugin: " + pluginStr + "!");
    qDebug() << "failed to set plugin";
}

void MainWindow::configurePlots() {
    qDebug() << "Configuring spectrum monitor...";
    _specMon->configure(*_pc);
    qDebug() << "Configuring spectrogram...";
    _spec->configure(*_pc);
    qDebug() << "Configuring histogram...";
    _hist->configure(*_pc);
    qDebug() << "Configuring mini spec thing...";
    _specPlot->configure(*_pc);
    qDebug() << "All plots configured";
}

