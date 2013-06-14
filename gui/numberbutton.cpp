#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QRadioButton>

#include "numberbutton.h"

NumberButton::NumberButton(QWidget *parent, 
                           const QString &prefix, const QString &suffix,
                           int min, int max, int step):
    QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    
    layout->setContentsMargins(QMargins(0,0,0,0));

    d_radio = new QRadioButton(this);
    d_radio->setAutoExclusive(true);
    d_radio->setText(prefix);
    layout->addWidget(d_radio);
    
    connect(d_radio, SIGNAL( toggled(bool) ), this, SIGNAL( toggled(bool) ));

    // if ( !prefix.isEmpty() )
    //     layout->addWidget(new QLabel(prefix + " ", this));
    
    d_counter = new QSpinBox(this);
    d_counter->setRange(min, max);
    d_counter->setSingleStep(step);
    layout->addWidget(d_counter);

    connect(d_counter, SIGNAL( valueChanged(int) ), this, SIGNAL( valueChanged(int) ));
    
    if ( !suffix.isEmpty() )
        layout->addWidget(new QLabel(QString(" ") + suffix, this));
}

void NumberButton::setValue(int value) { 
    d_counter->setValue(value); 
}

int NumberButton::value() const { 
    return d_counter->value(); 
}

void NumberButton::setEnabled(bool enabled) {
    d_counter->setEnabled(enabled);
}

void NumberButton::setAntiEnabled(bool anti_enabled) {
    d_counter->setEnabled(!anti_enabled);
}

void NumberButton::setChecked(bool checked) {
    d_radio->setChecked(checked);
}

void NumberButton::setUnchecked(bool unchecked) {
    d_radio->setChecked(!unchecked);
}

bool NumberButton::isChecked() {
    return d_radio->isChecked();
}

void NumberButton::toggle() {
    d_radio->toggle();
}
