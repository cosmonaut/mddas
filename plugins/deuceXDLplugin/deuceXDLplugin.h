#ifndef DEUCEXDLPLUGIN_H
#define DEUCEXDLPLUGIN_H

#include <stdint.h>

#include "samplingthreadinterface.h"
#include "samplingthreadplugin.h"

class Socket;

class DEUCEXDLplugin : public SamplingThreadPlugin {
    Q_OBJECT

public:
    DEUCEXDLplugin();
    ~DEUCEXDLplugin();

protected:
    void run();

private:
    Socket *_s;
    Socket *_config_s;
    uint8_t *_packet_buf;
    uint8_t *_config_buf;
    uint16_t _p_count;

    uint16_t _t_ind[3];
    uint16_t _i_ind[4];
    double _i_cal[4];
    uint16_t _v_ind[4];
    double _v_cal[4];
};
    

#endif

