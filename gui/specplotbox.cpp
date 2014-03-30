#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QGroupBox>

#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QValidator>
#include <QMessageBox>
#include <QComboBox>

#include "specplotbox.h"
#include "specplot.h"
#include "mddasplotconfig.h"
#include "numberbutton.h"


static QVector<uint> get_divisors(uint);
static bool ptwo(uint);


/* Control 3 snapshot spectrum plots to plot key information */

SpecPlotBox::SpecPlotBox(QMap<QString, QVariant> settings, QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    QHBoxLayout *hbox = new QHBoxLayout();

    _settings = new QMap<QString, QVariant>();
    *_settings = settings;

    QWidget *hFiller = new QWidget;
    hFiller->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    _plot1 = new SpecPlot(this, 1, 1, 1);
    _plot2 = new SpecPlot(this, 1, 1, 1);
    _plot3 = new SpecPlot(this, 1, 1, 1);

    QwtText plot_title("CHESS");
    QFont font = _plot1->title().font();
    font.setPointSize(8);
    plot_title.setFont(font);
    _plot1->setTitle(plot_title);

    _replotTimer = new QTimer(this);
    _replotTimer->setInterval(200);
    _replotTimer->setSingleShot(false);
    _replotTimer->stop();

    _specPlotConf = new SpecPlotConfig(settings, this);

    _boxConfigBox = new QGroupBox(this);
    _boxConfigBox->setTitle("Configuration");

    QPushButton *boxConfigButton = new QPushButton("Set Boxes");
    
    QHBoxLayout *boxLayout = new QHBoxLayout();
    boxLayout->addWidget(boxConfigButton);

    _boxConfigBox->setLayout(boxLayout);

    hbox->addWidget(_boxConfigBox);
    hbox->addWidget(hFiller);

    connect(boxConfigButton, SIGNAL( clicked() ), this, SLOT( setPrefs() ));

    connect(_replotTimer, SIGNAL( timeout() ), this, SLOT( replot() ));

    connect(_plot1, SIGNAL( boxSizeChanged(uint, uint, uint, uint) ), this, SLOT( alertBoxChange1(uint, uint, uint, uint) ));
    connect(_plot2, SIGNAL( boxSizeChanged(uint, uint, uint, uint) ), this, SLOT( alertBoxChange2(uint, uint, uint, uint) ));
    connect(_plot3, SIGNAL( boxSizeChanged(uint, uint, uint, uint) ), this, SLOT( alertBoxChange3(uint, uint, uint, uint) ));

    connect(_specPlotConf, SIGNAL( updated() ), this, SLOT( configBoxes() ));

    layout->addWidget(_plot1);
    layout->addWidget(_plot2);
    layout->addWidget(_plot3);

    layout->addLayout(hbox);
    setLayout(layout);

    _plot1->replot();
    _plot2->replot();
    _plot3->replot();

    setMaximumSize(500, 500);
}

void SpecPlotBox::setPrefs(void) {
    _specPlotConf->exec();
}

void SpecPlotBox::configure(MDDASPlotConfig pc) {
    _plot1->clear();
    _plot1->configure(pc);

    _plot2->clear();
    _plot2->configure(pc);

    _plot3->clear();
    _plot3->configure(pc);

    /* Easy way to manually test box stuff */
    // _plot1->setBox(4400, 4528, 3000, 5048);
    // _plot1->setRebinFactor(32);

    // _plot2->setBox(4000, 4128, 3000, 5048);
    // _plot2->setRebinFactor(16);

    // _plot3->setBox(3600, 3728, 3000, 5048);
    // _plot3->setRebinFactor(16);

    _plot1->setBox((*_settings)["b1x"].toUInt(), 
                   ((*_settings)["b1x"].toUInt() + (*_settings)["b1w"].toUInt()),
                   (*_settings)["b1y"].toUInt(), 
                   ((*_settings)["b1y"].toUInt() + (*_settings)["b1h"].toUInt()));
    _plot1->setRebinFactor((*_settings)["b1r"].toUInt());

    _plot2->setBox((*_settings)["b2x"].toUInt(), 
                   ((*_settings)["b2x"].toUInt() + (*_settings)["b2w"].toUInt()),
                   (*_settings)["b2y"].toUInt(), 
                   ((*_settings)["b2y"].toUInt() + (*_settings)["b2h"].toUInt()));
    _plot2->setRebinFactor((*_settings)["b2r"].toUInt());

    _plot3->setBox((*_settings)["b3x"].toUInt(), 
                   ((*_settings)["b3x"].toUInt() + (*_settings)["b3w"].toUInt()),
                   (*_settings)["b3y"].toUInt(), 
                   ((*_settings)["b3y"].toUInt() + (*_settings)["b3h"].toUInt()));
    _plot3->setRebinFactor((*_settings)["b3r"].toUInt());

}

void SpecPlotBox::activate(bool b) {
    if (b) {
        _boxConfigBox->setEnabled(false);
        _replotTimer->start();
    } else {
        /* stop replot timer */
        _replotTimer->stop();
        _boxConfigBox->setEnabled(true);
    }
    _active = b;
}


void SpecPlotBox::clear() {
    _plot1->clear();
    _plot2->clear();
    _plot3->clear();
}

void SpecPlotBox::append(const QVector<MDDASDataPoint> &dv) {
    /* This is where the fish lives */
    _plot1->appendPoints(dv);
    _plot2->appendPoints(dv);
    _plot3->appendPoints(dv);
}

void SpecPlotBox::replot() {
    _plot1->replot();
    _plot2->replot();
    _plot3->replot();
}

bool SpecPlotBox::isActive() {
    return _active;
}

void SpecPlotBox::alertBoxChange1(uint x_min, uint x_max, uint y_min, uint y_max) {
    //qDebug() << x_min << " " << x_max << " " << y_min << " " << y_max;
    emit boxSizeChanged1(x_min, x_max, y_min, y_max);
}

void SpecPlotBox::alertBoxChange2(uint x_min, uint x_max, uint y_min, uint y_max) {
    //qDebug() << x_min << " " << x_max << " " << y_min << " " << y_max;
    emit boxSizeChanged2(x_min, x_max, y_min, y_max);
}

void SpecPlotBox::alertBoxChange3(uint x_min, uint x_max, uint y_min, uint y_max) {
    //qDebug() << x_min << " " << x_max << " " << y_min << " " << y_max;
    emit boxSizeChanged3(x_min, x_max, y_min, y_max);
}

void SpecPlotBox::configBoxes(void) {
    QMap<QString, uint> m;

    m = _specPlotConf->getParams();

    _plot1->setBox(m["b1x"], (m["b1x"] + m["b1w"]), m["b1y"], (m["b1y"] + m["b1h"]));
    _plot1->setRebinFactor(m["b1r"]);

    _plot2->setBox(m["b2x"], (m["b2x"] + m["b2w"]), m["b2y"], (m["b2y"] + m["b2h"]));
    _plot2->setRebinFactor(m["b2r"]);

    _plot3->setBox(m["b3x"], (m["b3x"] + m["b3w"]), m["b3y"], (m["b3y"] + m["b3h"]));
    _plot3->setRebinFactor(m["b3r"]);


    //_settings
    (*_settings)["b1x"] = m["b1x"];
    (*_settings)["b1y"] = m["b1y"];
    (*_settings)["b1w"] = m["b1w"];
    (*_settings)["b1h"] = m["b1h"];
    (*_settings)["b1r"] = m["b1r"];

    (*_settings)["b2x"] = m["b2x"];
    (*_settings)["b2y"] = m["b2y"];
    (*_settings)["b2w"] = m["b2w"];
    (*_settings)["b2h"] = m["b2h"];
    (*_settings)["b2r"] = m["b2r"];

    (*_settings)["b3x"] = m["b3x"];
    (*_settings)["b3y"] = m["b3y"];
    (*_settings)["b3w"] = m["b3w"];
    (*_settings)["b3h"] = m["b3h"];
    (*_settings)["b3r"] = m["b3r"];


    // emit settings changed!
    emit settingsChanged();
}

/* Signal for spectroscatterplot to set box position */
void SpecPlotBox::setBoxXY(uint box, uint x, uint y) {
    /* reconfig plot
       update settings
       update specplotconfig settings
       emit settings changed */
    if (box == 1) {
        _plot1->setBox(x,
                       (x + (*_settings)["b1w"].toUInt()),
                       y,
                       (y + (*_settings)["b1h"].toUInt()));
        _plot1->setRebinFactor((*_settings)["b1r"].toUInt());

        (*_settings)["b1x"] = x;
        (*_settings)["b1y"] = y;

    } else if (box == 2) {
        _plot2->setBox(x,
                       (x + (*_settings)["b2w"].toUInt()),
                       y,
                       (y + (*_settings)["b2h"].toUInt()));
        _plot2->setRebinFactor((*_settings)["b2r"].toUInt());

        (*_settings)["b2x"] = x;
        (*_settings)["b2y"] = y;

    } else if (box == 3) {
        _plot3->setBox(x,
                       (x + (*_settings)["b3w"].toUInt()),
                       y,
                       (y + (*_settings)["b3h"].toUInt()));
        _plot3->setRebinFactor((*_settings)["b3r"].toUInt());

        (*_settings)["b3x"] = x;
        (*_settings)["b3y"] = y;
    }
    _specPlotConf->setSettings(*_settings);

    emit settingsChanged();
}

QMap<QString, QVariant> SpecPlotBox::getSettings(void) {
    QMap<QString, QVariant> m;
    m = (*_settings);
    return(m);
}

SpecPlotConfig::SpecPlotConfig(QMap<QString, QVariant> settings, QWidget *parent) : QDialog(parent){

    _settings = new QMap<QString, QVariant>();

    _params = new QMap<QString, uint>;

    // set with settings
    (*_params)["b1x"] = 0;
    (*_params)["b1y"] = 0;
    (*_params)["b1w"] = 0;
    (*_params)["b1h"] = 0;
    (*_params)["b1r"] = 1;

    (*_params)["b2x"] = 0;
    (*_params)["b2y"] = 0;
    (*_params)["b2w"] = 0;
    (*_params)["b2h"] = 0;
    (*_params)["b2r"] = 1;

    (*_params)["b3x"] = 0;
    (*_params)["b3y"] = 0;
    (*_params)["b3w"] = 0;
    (*_params)["b3h"] = 0;
    (*_params)["b3r"] = 1;


    QPushButton *closeButton = new QPushButton(tr("Close"));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(finish()));

    QVBoxLayout *mainLayout = new QVBoxLayout;


    QGroupBox *plot1Box = new QGroupBox(this);
    plot1Box->setTitle("Box 1");

    QHBoxLayout *plot1HBox = new QHBoxLayout(this);
    _plot1Widget = new BoxConfigBox(this);
    plot1HBox->addWidget(_plot1Widget);

    plot1Box->setLayout(plot1HBox);


    QGroupBox *plot2Box = new QGroupBox(this);
    plot2Box->setTitle("Box 2");

    QHBoxLayout *plot2HBox = new QHBoxLayout(this);
    _plot2Widget = new BoxConfigBox(this);
    plot2HBox->addWidget(_plot2Widget);

    plot2Box->setLayout(plot2HBox);


    QGroupBox *plot3Box = new QGroupBox(this);
    plot3Box->setTitle("Box 3");

    QHBoxLayout *plot3HBox = new QHBoxLayout(this);
    _plot3Widget = new BoxConfigBox(this);
    plot3HBox->addWidget(_plot3Widget);

    plot3Box->setLayout(plot3HBox);


    _plot1Widget->setParams(settings["b1x"].toUInt(),
                            settings["b1y"].toUInt(),
                            settings["b1w"].toUInt(),
                            settings["b1h"].toUInt(),
                            settings["b1r"].toUInt());

    _plot2Widget->setParams(settings["b2x"].toUInt(),
                            settings["b2y"].toUInt(),
                            settings["b2w"].toUInt(),
                            settings["b2h"].toUInt(),
                            settings["b2r"].toUInt());

    _plot3Widget->setParams(settings["b3x"].toUInt(),
                            settings["b3y"].toUInt(),
                            settings["b3w"].toUInt(),
                            settings["b3h"].toUInt(),
                            settings["b3r"].toUInt());


    mainLayout->addWidget(plot1Box);
    mainLayout->addWidget(plot2Box);
    mainLayout->addWidget(plot3Box);


    QWidget *hFiller = new QWidget;
    hFiller->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addWidget(hFiller);
    buttonsLayout->addWidget(closeButton);

    mainLayout->addStretch(1);
    mainLayout->addSpacing(12);

    mainLayout->addLayout(buttonsLayout);
    setLayout(mainLayout);

    setWindowTitle(tr("CHESS magic"));
}

void SpecPlotConfig::setSettings(QMap<QString, QVariant> settings) {
    (*_settings) = settings;

    _plot1Widget->setParams(settings["b1x"].toUInt(),
                            settings["b1y"].toUInt(),
                            settings["b1w"].toUInt(),
                            settings["b1h"].toUInt(),
                            settings["b1r"].toUInt());

    _plot2Widget->setParams(settings["b2x"].toUInt(),
                            settings["b2y"].toUInt(),
                            settings["b2w"].toUInt(),
                            settings["b2h"].toUInt(),
                            settings["b2r"].toUInt());

    _plot3Widget->setParams(settings["b3x"].toUInt(),
                            settings["b3y"].toUInt(),
                            settings["b3w"].toUInt(),
                            settings["b3h"].toUInt(),
                            settings["b3r"].toUInt());

}

void SpecPlotConfig::finish() {
    QVector<uint> b1;
    QVector<uint> b2;
    QVector<uint> b3;

    b1 = _plot1Widget->getParams();
    b2 = _plot2Widget->getParams();
    b3 = _plot3Widget->getParams();

    if (!ptwo(b1[3]) || !ptwo(b2[3]) || !ptwo(b3[3])) {
        QMessageBox::warning(this, tr("SAD PANDA!"),
                             tr("All heights must be a power of two!"),
                             QMessageBox::Ok);
        return;
    }

    //_params.clear();
    (*_params)["b1x"] = b1[0];
    (*_params)["b1y"] = b1[1];
    (*_params)["b1w"] = b1[2];
    (*_params)["b1h"] = b1[3];
    (*_params)["b1r"] = b1[4];

    //_params->insert("b2x", b2[0]);
    (*_params)["b2x"] = b2[0];
    (*_params)["b2y"] = b2[1];
    (*_params)["b2w"] = b2[2];
    (*_params)["b2h"] = b2[3];
    (*_params)["b2r"] = b2[4];

    //_params->insert("b3x", b3[0]);
    (*_params)["b3x"] = b3[0];
    (*_params)["b3y"] = b3[1];
    (*_params)["b3w"] = b3[2];
    (*_params)["b3h"] = b3[3];
    (*_params)["b3r"] = b3[4];

    close();

    emit updated();
}

const QMap<QString, QVariant>* SpecPlotConfig::getSettings() {
    return _settings;
}

QMap<QString, uint> SpecPlotConfig::getParams(void) {
    QMap<QString, uint> m;
    m = (*_params);
    return(m);
}
    
/* Configuration for a spectrum box */
BoxConfigBox::BoxConfigBox(QWidget *parent) : QWidget(parent){
    QHBoxLayout *hBox = new QHBoxLayout(this);

    QLabel *xlabel = new QLabel(this);
    xlabel->setText("X: ");

    QLabel *ylabel = new QLabel(this);
    ylabel->setText("Y: ");

    QLabel *wlabel = new QLabel(this);
    wlabel->setText("Width: ");

    QLabel *hlabel = new QLabel(this);
    hlabel->setText("Height ");

    QLabel *rlabel = new QLabel(this);
    rlabel->setText("Rebin ");

    _plotX = new QLineEdit(this);
    _plotY = new QLineEdit(this);
    _plotW = new QLineEdit(this);
    _plotH = new QLineEdit(this);
    _plotR = new QComboBox(this);

    QIntValidator *v = new QIntValidator(this);

    _plotX->setValidator(v);
    _plotY->setValidator(v);
    _plotW->setValidator(v);
    _plotH->setValidator(v);

    _plotX->setText("0");
    _plotY->setText("0");
    _plotW->setText("0");
    _plotH->setText("2");
    _plotR->addItem("1");

    hBox->addWidget(xlabel);
    hBox->addWidget(_plotX);
    hBox->addWidget(ylabel);
    hBox->addWidget(_plotY);
    hBox->addWidget(wlabel);
    hBox->addWidget(_plotW);
    hBox->addWidget(hlabel);
    hBox->addWidget(_plotH);
    hBox->addWidget(rlabel);
    hBox->addWidget(_plotR);

    connect(_plotH, SIGNAL( editingFinished() ), this, SLOT( updateCombo() ));
    
}

QVector<uint> BoxConfigBox::getParams(void) {
    QVector<uint> v;

    v.append(_plotX->text().toInt());
    v.append(_plotY->text().toInt());
    v.append(_plotW->text().toInt());
    v.append(_plotH->text().toInt());
    v.append(_plotR->itemText(_plotR->currentIndex()).toInt());

    return v;
}

void BoxConfigBox::setParams(uint x, uint y, uint w, uint h, uint r) {
    _plotX->setText(QString::number(x));
    _plotY->setText(QString::number(y));
    _plotW->setText(QString::number(w));
    _plotH->setText(QString::number(h));
    /* update combo box so that rebin can be set */
    updateCombo();

    /* Set saved rebin if possible... */
    if (_plotR->findText(QString::number(r)) > -1) {
        _plotR->setCurrentIndex(_plotR->findText(QString::number(r)));
    } else {
        qDebug() << "WARNING: could not set rebin param";
        _plotR->setCurrentIndex(0);
    }
}

void BoxConfigBox::updateCombo(void) {
    int i = 0;
    QVector<uint> v;
    v = get_divisors(_plotH->text().toInt());

    _plotR->clear();

    for (i = 0; i < v.size(); i++) {
        _plotR->addItem(QString::number(v.value(i)));
    }
}

/* Helper to find all divisors of an axis size */
static QVector<uint> get_divisors(uint n) {
    uint i = 2;
    QVector<uint> divs;

    /* We add 1 as an option to go back to full resolution */
    divs.append(1);

    //while(i < sqrt(n)) {
    while(i < n/2) {
        if(n%i == 0) {
            divs.append(i);
        }

        i++;
    }
    if(i*i == n) {
        divs.append(i);
    }

    return(divs);
}


/* check if a number is a power of two... */
static bool ptwo(uint num) {
    if (!(num & (num - 1)) && num) {
        return true;
    } else {
        return false;
    }
}
