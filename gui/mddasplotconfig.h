#ifndef MDDASPLOTCONFIG_H
#define MDDASPLOTCONFIG_H

class MDDASPlotConfig {
    //Q_OBJECT
public:
    MDDASPlotConfig(int x_max = 0, 
                    int x_min = 0, 
                    int y_max = 0, 
                    int y_min = 0,
                    int p_max = 0,
                    int p_min = 0);
    
    void setXMax(int);
    void setXMin(int);
    void setYMax(int);
    void setYMin(int);
    void setPMax(int);
    void setPMin(int);

    int getXMax(void) const;
    int getXMin(void) const;
    int getYMax(void) const;
    int getYMin(void) const;
    int getPMax(void) const;
    int getPMin(void) const;

private:
    int _x_max;
    int _x_min;
    int _y_max;
    int _y_min;
    int _p_max;
    int _p_min;
};

#endif

