#include <qwt_picker_machine.h>

#include "boxpicker.h"


BoxPicker::BoxPicker( int xAxis, int yAxis, QWidget *canvas ):
    QwtPlotPicker(xAxis, yAxis, canvas) {

    QwtPickerMachine* pickerMachine = new QwtPickerClickPointMachine();
    setStateMachine(pickerMachine);

    /* Nifty purple... */
    //setTrackerPen(QColor(217, 168, 225));
    setTrackerPen(QColor(Qt::white));
    setTrackerMode(QwtPicker::AlwaysOn);

}


BoxPicker::~BoxPicker() {
}

void BoxPicker::setColor(QColor c) {
    setTrackerPen(c);
}


// QwtPlotPicker* plotPicker = new QwtPlotPicker(this->xBottom, this->yLeft, QwtPicker::CrossRubberBand, QwtPicker::AlwaysOn, this->canvas());
// QwtPickerMachine* pickerMachine = new QwtPickerClickPointMachine();
// plotPicker->setStateMachine(pickerMachine);
// connect(plotPicker, SIGNAL(selected(const QPointF&)), this, SLOT(onSelected(const QPointF&)));
// plotPicker->setTrackerPen(QColor(217, 168, 225));
// //plotPicker->setCursor(Qt::ForbiddenCursor);
// plotPicker->setEnabled(false);
