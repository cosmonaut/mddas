#ifndef CHAP10PLUGIN_H
#define CHAP10PLUGIN_H

#include <stdint.h>
#include "samplingthreadinterface.h"
#include "samplingthreadplugin.h"

class UDPSocket;

class Chap10Plugin : public SamplingThreadPlugin {
    Q_OBJECT

public:
    Chap10Plugin();
    ~Chap10Plugin();

protected:
    void run();

private:
    QVector<MDDASDataPoint> parse_data(void);

    UDPSocket *_sock;
    uint16_t data_buf[4096];
    /* Last word in data_buf */
    uint32_t data_buf_ind;
};
    

#endif

