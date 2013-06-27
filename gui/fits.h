#ifndef _FITS_H_
#define _FITS_H_

#include <QVector>
#include <QMap>

class MDDASDataPoint;
class MDDASPlotConfig;
class QDateTime;

int writeFitsImage(const QVector<MDDASDataPoint> *, 
                   const QVector<double> *,
                   const QDateTime,
                   const double, 
                   const MDDASPlotConfig*,
                   const QMap<QString, QVariant>*);

#endif // _FITS_H_
