#ifndef NIDAQPLUGIN_H
#define NIDAQPLUGIN_H

#include <stdint.h>

#include "samplingthreadinterface.h"
#include "samplingthreadplugin.h"

class NIDAQPlugin : public SamplingThreadPlugin {
    Q_OBJECT

public:
    NIDAQPlugin();
    ~NIDAQPlugin();

protected:
    void run();

private:
};
    

#endif

