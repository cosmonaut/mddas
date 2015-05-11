#ifndef NIDAQPLUGIN_H
#define NIDAQPLUGIN_H

#include <stdint.h>
#include <comedilib.h>

#include "samplingthreadinterface.h"
#include "samplingthreadplugin.h"

//#define COM_BUF_LEN 100000
#define COM_BUF_LEN 262144
#define COM_N_CHAN 16
/* comedi regex patterns */
#define DAQ_PATTERN "\\s\\([0-9]\\):\\sni_pcidio\\s*pci\\-dio\\-32hs\\s*1"
#define TIMER_PATTERN "\\s\\([0-9]\\):\\sni_660x\\s*PCI\\-6601\\s*1"
/* Standard locations of comedi stuff */
#define COM_PREFIX "/dev/comedi"
#define COM_PROC "/proc/comedi"

/* Strobe period and high time in nanoseconds */
#define STROBE_PERIOD 700
#define STROBE_HIGH_T 250

class NIDAQPlugin : public SamplingThreadPlugin {
    Q_OBJECT

public:
    NIDAQPlugin();
    ~NIDAQPlugin();

protected:
    void run();

private:
    int ni_gpct_start_pulse_gen(comedi_t *, 
                                unsigned,
                                unsigned,
                                unsigned);
    int ni_gpct_stop_pulse_gen(comedi_t *, 
                               unsigned);

    lsampl_t buf[COM_BUF_LEN];
    comedi_t *dio_dev;
    comedi_t *timer_dev;
    comedi_cmd cmd;
    char *daq_dev_file = NULL;
    char *timer_dev_file = NULL;

};
    

#endif

