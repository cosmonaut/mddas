#ifndef DEWEPLUGIN_H
#define DEWEPLUGIN_H

//#include <stdint.h>

#include "samplingthreadinterface.h"
#include "samplingthreadplugin.h"

class TCPServerSocket;
class TCPSocket;

class DewePlugin : public SamplingThreadPlugin {
    Q_OBJECT

public:
    DewePlugin();
    ~DewePlugin();

protected:
    void run();

private:
    TCPServerSocket *_serv_sock;
    TCPSocket *_sock;

    //uint8_t *_packet_buf;
    //uint16_t _p_count;
};
    

#endif

