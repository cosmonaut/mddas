#ifndef CHAP10PLUGIN_H
#define CHAP10PLUGIN_H


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
    UDPSocket *_sock;
};
    

#endif

