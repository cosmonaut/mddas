#ifndef _SUPERDATA_H_
#define _SUPERDATA_H_

#include <qwt_matrix_raster_data.h>
#include "mddasdatapoint.h"

/* A class to define a super plot data object (one per detector) to
   feed all plot widgets from without code/ram repitition. */


class SuperData: public QwtMatrixRasterData {
public:
    SuperData(uint x_max, uint y_max, uint p_max);
    ~SuperData();

    void append(const QVector<MDDASDataPoint> &vec);
    void clear();
    
    /* Rebin the image by an integer factor. */
    int rebin(uint factor);


private:
    // Matrix of pixels
    uint32_t *vals; /* matrix of pixels */
    uint32_t _max_v;

    // limits
    uint x_lim;
    uint y_lim;
    uint p_lim;

    // rebin array
    uint32_t *rebin_vals;
    bool _can_rebin;
    bool _rebin;
    uint _rebin_size_x;
    uint _rebin_size_y;
    uint32_t _rebin_max_v;
    
    // hash for fast rebin magic
    QHash<uint, uint> *_x_hash;
    QHash<uint, uint> *_y_hash;
    
    /* Vectors to hold photon list */
    QVector<uint> *_x_list;
    QVector<uint> *_y_list;
};


#endif
