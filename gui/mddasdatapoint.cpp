#include "mddasdatapoint.h"

MDDASDataPoint::MDDASDataPoint(int x, int y, int p) {
    _x = x;
    _y = y;
    _p = p;
}

MDDASDataPoint::~MDDASDataPoint() {}

int MDDASDataPoint::x() const {
    return _x;
}

int MDDASDataPoint::y() const {
    return _y;
}

int MDDASDataPoint::p() const {
    return _p;
}

