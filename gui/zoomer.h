#ifndef _ZOOMER_H
#define _ZOOMER_H

#include <qwt/qwt_plot_canvas.h>

#include "scrollzoomer.h"

class Zoomer: public ScrollZoomer
{
public:
    Zoomer(QwtPlotCanvas *canvas, uint x_max = 0):
        ScrollZoomer(canvas)
    {
        x_lim = x_max;
#if 0
        setRubberBandPen(QPen(Qt::white, 2, Qt::DotLine));
#else
        setRubberBandPen(QPen(Qt::white));
#endif
    }

    virtual QwtText trackerTextF(const QPointF &pos) const
    {
        QColor bg(Qt::white);

        QwtText text = QwtPlotZoomer::trackerTextF(pos);
        text.setBackgroundBrush( QBrush( bg ));
        return text;
    }

    virtual void rescale()
    {
        QwtScaleWidget *scaleWidget = plot()->axisWidget(yAxis());
        QwtScaleDraw *sd = scaleWidget->scaleDraw();

        int minExtent = 0;
        if ( zoomRectIndex() > 0 )
        {
            /* When scrolling in vertical direction
               the plot is jumping in horizontal direction
               because of the different widths of the labels
               So we better use a fixed extent. */
            
            minExtent = sd->spacing() + sd->maxTickLength() + 1;
            minExtent += sd->labelSize(
                scaleWidget->font(), x_lim).width();
        }

        sd->setMinimumExtent(minExtent);

        ScrollZoomer::rescale();
    }
    
private:
    uint x_lim;
};

#endif
