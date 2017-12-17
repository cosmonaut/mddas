#include "superdata.h"




SuperData::SuperData(uint x_max, uint y_max, uint p_max) {
    
    x_lim = x_max;
    y_lim = y_max;
    p_lim = p_max;
    
    _rebin = false;
    //_rebin_size = 1;
    _rebin_size_x = 1;
    _rebin_size_y = 1;
    // TODO: won't need if we implement square rebin.
    _can_rebin = false;
    
    // pointers
    this->vals = new uint32_t[x_max*y_max];
    memset(vals, 0, x_max*y_max*sizeof(uint32_t));
    _max_v = 0;
    
    this->rebin_vals = NULL;
    this->_x_hash = NULL;
    this->_y_hash = NULL;
    _rebin_max_v = 0;

    _x_list = new QVector<uint>;
    _y_list = new QVector<uint>;
}


SuperData::~SuperData() {
    /* IMPORTANT! Delete the vals array */
    delete[] this->vals;
    if (this->rebin_vals) {
        delete[] this->rebin_vals;
    }
    if (this->_x_hash) {
        delete this->_x_hash;
    }
    if (this->_y_hash) {
        delete this->_y_hash;
    }
    if (this->_x_list) {
        delete this->_x_list;
    }
    if (this->_y_list) {
        delete this->_y_list;
    }
}

void SuperData::append(const QVector<MDDASDataPoint> &vec) {
    /* Append new z vals. */
    for (uint i = 0; i < (uint)vec.size(); i++) {
        if (((uint)vec[i].x() < x_lim) && ((uint)vec[i].y() < y_lim)) {
            /* Keep track of photon list as well */
            _x_list->append((uint)vec[i].x());
            _y_list->append((uint)vec[i].y());

            vals[y_lim*(uint)vec[i].x() + (uint)vec[i].y()] += 1;
            // TODO -- cehck this rebin logic.
            if (_rebin) {
                ++rebin_vals[_rebin_size_x*_x_hash->value((uint)vec[i].x()) + _y_hash->value((uint)vec[i].y())];

                if (rebin_vals[_rebin_size_x*_x_hash->value((uint)vec[i].x()) + _y_hash->value((uint)vec[i].y())] > _rebin_max_v) {
                    ++_rebin_max_v;
                }
            }

            /* This handles max amplitude without having to loop
               over the entire matrix */
            if (vals[y_lim*(uint)vec[i].x() + (uint)vec[i].y()] > _max_v) {
                _max_v += 1;
            }
                
        }
    }
}


void SuperData::clear() {
    /* Set all values to 0. */
    memset(vals, 0, x_lim*y_lim*sizeof(uint32_t));
    if (_rebin) {
        memset(rebin_vals, 0, _rebin_size_x*_rebin_size_y*sizeof(uint32_t));
        _rebin_max_v = 0;
    }
    
    /* Set max amplitude to 0 */
    _max_v = 0;
    
    /* Clear photon list */
    _x_list->clear();
    _x_list->squeeze();
    _y_list->clear();
    _y_list->squeeze();
}        


/* Rebin the image by an integer factor. Rebinning is currently
   only allowed for square images. */
int SuperData::rebin(uint factor) {
    uint i = 0;
    uint px = 0;
    uint py = 0;
    uint temp_v = 0;

    
    /* Save axis size of rebin image */
    //_rebin_size = x_lim/factor;

    _rebin_size_x = x_lim/factor;
    _rebin_size_y = y_lim/factor;

    

    if (_can_rebin) {
        if (factor > 1) {
            /* Create rebinned image buffer */
            if (this->rebin_vals) {
                delete[] this->rebin_vals;
                this->rebin_vals = NULL;
            }
            this->rebin_vals = new uint32_t[_rebin_size_x*_rebin_size_y];
            /* Initialize image */
            memset(rebin_vals, 0, _rebin_size_x*_rebin_size_y*sizeof(uint32_t));

            /* Create hash for rebin image. This is a lookup for
               rebin pixel location from full res pixel
               location. */
            if (this->_x_hash) {
                delete this->_y_hash;
                this->_x_hash = NULL;
            }
            if (this->_y_hash) {
                delete this->_y_hash;
                this->_y_hash = NULL;
            }

            _x_hash = new QHash<uint, uint>;
            _y_hash = new QHash<uint, uint>;
            /* Populate hashes */
            for (i = 0; i < x_lim; i++) {
                _x_hash->insert((uint)i, px);

                if ((i%factor) == (factor - 1)) {
                    px++;
                }
            }
            for (i = 0; i < y_lim; i++) {
                _y_hash->insert((uint)i, py);

                if ((i%factor) == (factor - 1)) {
                    py++;
                }
            }

            /* Reset _rebin_max_v because it will be recalced */
            _rebin_max_v = 0;

            /* Populate rebin image from photon list. Note that
               for large images this method is faster than
               building the rebin from the full resolution
               image. */
            for (i = 0; i < (uint)_x_list->size(); i++) {
                temp_v = ++this->rebin_vals[_rebin_size_x*_x_hash->value(_x_list->value(i)) + _y_hash->value(_y_list->value(i))];

                if (temp_v > _rebin_max_v) {
                    _rebin_max_v = temp_v;
                }

            }

            /* We are in rebinned mode now... */
            _rebin = true;

            /* Update axes to reflect rebin image size */
            setInterval( Qt::XAxis, QwtInterval( 0.0, _rebin_size_x ) );
            setInterval( Qt::YAxis, QwtInterval( 0.0, _rebin_size_y ) );
            //setInterval( Qt::ZAxis, QwtInterval( 0.0, 1.0 ) );
            //updateScale();

        } else {
            /* Back to full resolution. */
            _rebin = false;
            if (this->rebin_vals) {
                delete[] this->rebin_vals;
                this->rebin_vals = NULL;
            }

            setInterval( Qt::XAxis, QwtInterval( 0.0, x_lim) );
            setInterval( Qt::YAxis, QwtInterval( 0.0, y_lim ) );
            //setInterval( Qt::ZAxis, QwtInterval( 0.0, 1.0 ) );
            //updateScale();

        }
        
        setInterval( Qt::ZAxis, QwtInterval( 0.0, 1.0 ) );
        //updateScale();
    } else {
        /* Return an error if we can't rebin */
        return(-1);
    }

    return(0);
}
