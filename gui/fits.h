#ifndef _FITS_H_
#define _FITS_H_

#include <QVector>
#include <QMap>

class MDDASDataPoint;
class MDDASPlotConfig;
class QDateTime;

int writeFitsImage(const QVector<MDDASDataPoint> *, 
                   const QMap<uint, double> *, 
                   const QDateTime,
                   const double, 
                   const MDDASPlotConfig*);

#endif // _FITS_H_
