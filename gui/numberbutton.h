#ifndef _NUMBERBUTTON_H_
#define _NUMBERBUTTON_H_

#include <QWidget>

class QSpinBox;
class QRadioButton;

class NumberButton: public QWidget
{
    Q_OBJECT
public:
    NumberButton(QWidget *parent, 
                 const QString &prefix, const QString &suffix,
                 int min, int max, int step);

    int value() const;
    bool isChecked();

signals:
    void toggled(bool);
    void valueChanged(int);

public slots:
    void toggle();
    void setChecked(bool);
    void setUnchecked(bool);
    void setEnabled(bool);
    void setAntiEnabled(bool);
    void setValue(int);

private:
    QRadioButton *d_radio;
    QSpinBox *d_counter;
};

#endif // NUMBERBUTTON_H
