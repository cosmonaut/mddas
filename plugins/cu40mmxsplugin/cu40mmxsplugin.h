#ifndef CU40MMXSPLUGIN_H
#define CU40MMXSPLUGIN_H

#include <stdint.h>

#include "samplingthreadinterface.h"
#include "samplingthreadplugin.h"

class Socket;

class CU40MMXSPlugin : public SamplingThreadPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "cu40mmxsplugin")

public:
    CU40MMXSPlugin();
    ~CU40MMXSPlugin();

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

