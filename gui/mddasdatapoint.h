#ifndef MDDASDATAPOINT_H
#define MDDASDATAPOINT_H


/* A Data point for photon counting detectors! */
class MDDASDataPoint {
    //Q_OBJECT
public:
    MDDASDataPoint(int x = 0, int y = 0, int p = 0);
    ~MDDASDataPoint();
    int x() const;
    int y() const;
    int p() const;

private:
    int _x;
    int _y;
    int _p;
};

#endif
