#ifndef DETCONBASEPLUGIN_H
#define DETCONBASEPLUGIN_H

#include <stdint.h>

#include "samplingthreadinterface.h"
#include "samplingthreadplugin.h"

class Socket;

class CU40MMXSPlugin : public SamplingThreadPlugin {
    Q_OBJECT

public:
    CU40MMXSPlugin();
    ~CU40MMXSPlugin();

protected:
    void run();

private:
    Socket *_s;
    uint8_t *_packet_buf;
    uint16_t _p_count;
};
    

#endif

