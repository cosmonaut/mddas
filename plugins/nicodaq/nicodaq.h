#ifndef NICODAQPLUGIN_H
#define NICODAQPLUGIN_H

#include <stdint.h>

#include "samplingthreadinterface.h"
#include "samplingthreadplugin.h"

// see socket.h
class Socket;

class DIODAQPlugin : public SamplingThreadPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "nicodaqplugin")

public:
    DIODAQPlugin();
    ~DIODAQPlugin();

protected:
    void run();

private:
    QVector<MDDASDataPoint> parse_data(void);
    void config(uint8_t);

    // socket
    Socket *_s;
    // packet info
    uint8_t *_packet_buf;
    uint16_t _p_count;

    uint8_t *_c_pbuf;
    // counter for prse errors
    uint64_t bad_words;

    // flag to show partial packet parse
    uint32_t g_pflag;
    // variables to preserve packets across state...
    uint8_t g_state;
    uint16_t g_x;
    uint16_t g_y;
    uint16_t g_p;
};
    

#endif

