#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMap>
#include <QVariant>
#include <QGroupBox>
#include <QCheckBox>
#include <QDebug>
#include "mddasconfigdialog.h"

MDDASConfigDialog::MDDASConfigDialog(QWidget *parent) : QDialog(parent){

    _defaultSettings = new QMap<QString, QVariant>();
    _settings = new QMap<QString, QVariant>();

    QPushButton *closeButton = new QPushButton(tr("Close"));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(finish()));

    QPushButton *defaultsButton = new QPushButton(tr("Set Defaults"));
    connect(defaultsButton, SIGNAL(clicked()), this, SLOT( displayDefaults() ));
    //connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));

    QGroupBox *fitsBox = new QGroupBox("Fits Settings");
    QVBoxLayout *fitsLayout = new QVBoxLayout(fitsBox);

    _fitsImage = new QCheckBox("Save FITS Image", this);
    _fitsImage->setChecked(_settings->value("fitsImage").toBool());

    fitsLayout->addWidget(_fitsImage);
    //fitsBox->addLayout(fitsLayout);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    //buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(defaultsButton);
    buttonsLayout->addWidget(closeButton);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    // mainLayout->addLayout(horizontalLayout);
    mainLayout->addWidget(fitsBox);
    mainLayout->addStretch(1);
    mainLayout->addSpacing(12);

    mainLayout->addLayout(buttonsLayout);
    setLayout(mainLayout);

    setWindowTitle(tr("MDDAS Configuration"));
}

void MDDASConfigDialog::displayDefaults() {
    _fitsImage->setChecked(_defaultSettings->value("fitsImage").toBool());
    *_settings = *_defaultSettings;
}

void MDDASConfigDialog::setSettings(QMap<QString, QVariant> settings) {
    (*_settings) = settings;
    //qDebug() << "sets: " << _settings->value("fitsImage");
    _fitsImage->setChecked(_settings->value("fitsImage").toBool());
}

void MDDASConfigDialog::setDefaults(QMap<QString, QVariant> defaults) {
    (*_defaultSettings) = defaults;
    //qDebug() << "conf dialog: " << _defaultSettings->keys();
}

void MDDASConfigDialog::finish() {
    _settings->insert("fitsImage", _fitsImage->isChecked());
    close();
}

const QMap<QString, QVariant>* MDDASConfigDialog::getSettings() {
    return _settings;
}

    
    
