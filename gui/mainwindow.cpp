#include <QtGui>

#include "samplingthreadinterface.h"
#include "mddasplotconfig.h"
#include "mddasdatapoint.h"
#include "specmonbox.h"
#include "specbox.h"
#include "histbox.h"
#include "numberbutton.h"
#include "mainwindow.h"
#include "atomic.xpm"
#include "eye.xpm"
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

MainWindow::MainWindow() {
    loadedPlugin = "";

    /* Configuration for the plots */
    _pc = new MDDASPlotConfig();
    //_pc->getXMax();

    QWidget *widget = new QWidget(this);
    setCentralWidget(widget);

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
    connect(d_monitorAction, SIGNAL( toggled(bool) ), _clearAction, SLOT( setDisabled(bool) ));
    connect(d_monitorAction, SIGNAL( toggled(bool) ), _acquireAction, SLOT( setDisabled(bool) ));

    connect(_acquireAction, SIGNAL( toggled(bool) ), d_monitorAction, SLOT( setDisabled(bool) ));
    connect(_acquireAction, SIGNAL( toggled(bool) ), _clearAction, SLOT( setDisabled(bool) ));
    connect(_acquireAction, SIGNAL( toggled(bool) ), _plottb, SLOT( setDisabled(bool) ));
    connect(_acquireAction, SIGNAL( toggled(bool) ), unloadAction, SLOT( setDisabled(bool) ));

    connect(_acqTimer, SIGNAL( timeout() ), this, SLOT( appendData() ));
    connect(d_monitorAction, SIGNAL( toggled(bool) ), this, SLOT( toggleAcq(bool) ));
    connect(_clearAction, SIGNAL( triggered() ), this, SLOT( clearPlots() ));
    connect(_rateTimer, SIGNAL( timeout() ), this, SLOT( dispCountRate() ));

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
        //_count = 0;
        //_rcount = 0;
        clearPlots();
        _acqTimer->start();
        _rateTimer->start();
    } else {
        _rateTimer->stop();
        _acqTimer->stop();
        //_count = 0;
        //_rcount = 0;
        n = sti->bufCount();
        teststr.setNum(n);
        qDebug() << "packets left: " << teststr;
    }
}

void MainWindow::appendData() {
    int n = 0;
    //int count = 0;
    QString db;
    //int i = 0;
    n = sti->bufCount();
    if (n > 0) {
        for (int i = 0; i < n; i++) {
            QVector<MDDASDataPoint> v = sti->bufDequeue();
            _count += v.size();
            _totalCounts += v.size();
            // db.setNum(count);
            // infoLabel->setText(db);
            if (_specMon->isVisible()) {
                _specMon->append(v);
            }

            if (_spec->isVisible()) {
                _spec->append(v);
            }

            if (_hist->isVisible()) {
                _hist->append(v);
            }

        }
    }
}

/* Display count rate */
void MainWindow::dispCountRate() {
    _avgcount += _count*10;
    _rcount += 1;
    _count = 0;
    _totalCountsDisp->setNum(_totalCounts);
    if (_rcount > 9) {
        _infoLabel->setNum((double)_avgcount/10.0);
        _countRateDisp->setNum((double)_avgcount/10.0);
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

    _clearAction = new QAction(QIcon(xicon_xpm), "Clear", _acqtb);
    _clearAction->setCheckable(false);
    _clearAction->setEnabled(true);

    _acqtb->addAction(d_monitorAction);
    _acqtb->addAction(_acquireAction);
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

    _plottb->addAction(_specMonAction);
    _plottb->addAction(_specAction);
    _plottb->addAction(_histAction);

    _plotActionList->append(_specMonAction);
    _plotActionList->append(_specAction);
    _plotActionList->append(_histAction);

    _plottb->setEnabled(false);
    
    connect(_specMonAction, SIGNAL( toggled(bool) ), _specMon, SLOT( setVisible(bool) ));
    connect(_specAction, SIGNAL( toggled(bool) ), _spec, SLOT( setVisible(bool) ));
    connect(_histAction, SIGNAL( toggled(bool) ), _hist, SLOT( setVisible(bool) ));

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

    QVBoxLayout *acqLayout = new QVBoxLayout(acqGroup);
    //acqLayout->setContentsMargins(QMargins(0,0,0,0));
    
    _countRateDisp = new StatusNumber(_stattb, "count rate:", 0.0);
    _totalCountsDisp = new StatusNumber(_stattb, "total counts:", 0.0);
    _timeLeftDisp = new StatusNumber(_stattb, "time left:", 0.0);

    acqLayout->addWidget(_countRateDisp);
    acqLayout->addWidget(_totalCountsDisp);
    acqLayout->addWidget(_timeLeftDisp);

    QGroupBox *expGroup = new QGroupBox(_stattb);
    expGroup->setTitle("Exposure");

    QVBoxLayout *expLayout = new QVBoxLayout(expGroup);
    
    _expTimeDisp = new NumberButton(expGroup, "Time:", QString::null, 1, 999999, 1);
    _expTimeDisp->setChecked(true);
    _expCountsDisp = new NumberButton(expGroup, "Counts:", QString::null, 1, 99999999, 1);
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
    _stattb->addWidget(expGroup);

    return _stattb;
}

void MainWindow::createActions(){
    exitAct = new QAction(tr("&Exit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

    acqLoadAct = new QAction(tr("&Load Sampler"), this);
    acqLoadAct->setStatusTip(tr("Load a sampling thread"));

    connect(acqLoadAct, SIGNAL(triggered()), this, SLOT(listPlugins()));
}

void MainWindow::createMenus() {
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addSeparator();

    fileMenu->addAction(exitAct);

    acqMenu = menuBar()->addMenu(tr("&Acquisition"));
    acqMenu->addAction(acqLoadAct);
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
bool MainWindow::listPlugins() {
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
    return true;
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
    _specMon->configure(*_pc);
    _spec->configure(*_pc);
    _hist->configure(*_pc);
}
