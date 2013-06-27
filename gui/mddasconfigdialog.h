#ifndef _MDDASCONFIGDIALOG_H_
#define _MDDASCONFIGDIALOG_H_

#include <QDialog>

class QCheckBox;
//class QMap

class MDDASConfigDialog : public QDialog {
    Q_OBJECT
public:
    MDDASConfigDialog(QWidget *parent = 0);
    void setSettings(QMap<QString, QVariant>);
    void setDefaults(QMap<QString, QVariant>);

public slots:
    const QMap<QString, QVariant>* getSettings();

private slots:
    void finish();
    void displayDefaults();

private:
    QCheckBox *_fitsImage;

    QMap<QString, QVariant> *_defaultSettings;
    QMap<QString, QVariant> *_settings;
};

#endif
