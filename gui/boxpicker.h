#ifndef _BOXPICKER_H
#define _BOXPIKCER_H

#include <qwt_plot_picker.h>

class QColor;

class BoxPicker: public QwtPlotPicker {
    Q_OBJECT
public:
    BoxPicker( int xAxis, int yAxis, QWidget *canvas );
    virtual ~BoxPicker();

    void setColor(QColor);

protected:

private Q_SLOTS:

private:

};

#endif
