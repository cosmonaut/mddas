#include "mddasplotconfig.h"

MDDASPlotConfig::MDDASPlotConfig(int x_max, 
                                 int x_min, 
                                 int y_max, 
                                 int y_min, 
                                 int p_max, 
                                 int p_min) {
    _x_max = x_max;
    _x_min = x_min;
    _y_max = y_max;
    _y_min = y_min;
    _p_max = p_max;
    _p_min = p_min;
}

void MDDASPlotConfig::setXMax(int x_max) {
    _x_max = x_max;
}

void MDDASPlotConfig::setXMin(int x_min) {
    _x_min = x_min;
}

void MDDASPlotConfig::setYMax(int y_max) {
    _y_max = y_max;
}

void MDDASPlotConfig::setYMin(int y_min) {
    _y_min = y_min;
}

void MDDASPlotConfig::setPMax(int p_max) {
    _p_max = p_max;
}

void MDDASPlotConfig::setPMin(int p_min) {
    _p_min = p_min;
}


int MDDASPlotConfig::getXMax(void) const {
    return _x_max;
}

int MDDASPlotConfig::getXMin(void) const {
    return _x_min;
}

int MDDASPlotConfig::getYMax(void) const {
    return _y_max;
}

int MDDASPlotConfig::getYMin(void) const {
    return _y_min;
}

int MDDASPlotConfig::getPMax(void) const {
    return _p_max;
}

int MDDASPlotConfig::getPMin(void) const {
    return _p_min;
}
