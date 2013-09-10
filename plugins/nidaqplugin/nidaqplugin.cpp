/* Author: Nicholas Nell
   email: nicholas.nell@colorado.edu

   Sampling thread for the NI PCI-DIO-32HS and PCI-6601 that act as
   NSROC telemtery.  
*/

#include <QDebug>
#include <regex.h>

#include "nidaqplugin.h"


NIDAQPlugin::NIDAQPlugin() : SamplingThreadPlugin() {
    FILE *fp;
    char line_buffer[255];
    uint32_t line_number = 0;
    int err = 0;

    regex_t daq_card_reg;
    regex_t timer_card_reg;

    regmatch_t daq_match[2];
    regmatch_t timer_match[2];

    if (regcomp(&daq_card_reg, DAQ_PATTERN, 0)) {
        qDebug() << "DAQ regcomp fail";
        throw;
    }

    if (regcomp(&timer_card_reg, TIMER_PATTERN, 0)) {
        qDebug() << "DAQ regcomp fail";
        throw;
    }

    fp = fopen(COM_PROC, "r");
    if (fp == NULL) {
        qDebug() << "Can't open " << COM_PROC;
        throw;
    }

    /* Find comedi device file names... */
    while (fgets(line_buffer, sizeof(line_buffer), fp)) {
        ++line_number;
        if (!regexec(&daq_card_reg, line_buffer, 2, daq_match, 0)) {
            daq_dev_file = (char *)malloc(strlen(COM_PREFIX) + 2);
            sprintf(daq_dev_file, 
                    "%s%.*s", 
                    COM_PREFIX, 
                    daq_match[1].rm_eo - daq_match[1].rm_so, 
                    &line_buffer[daq_match[1].rm_so]);
            qDebug() << daq_dev_file;
        }

        if (!regexec(&timer_card_reg, line_buffer, 2, timer_match, 0)) {
            timer_dev_file = (char *)malloc(strlen(COM_PREFIX) + 2);
            sprintf(timer_dev_file, 
                    "%s%.*s", 
                    COM_PREFIX, 
                    timer_match[1].rm_eo - timer_match[1].rm_so, 
                    &line_buffer[timer_match[1].rm_so]);
            qDebug() << timer_dev_file;
        }

    }

    regfree(&daq_card_reg);
    regfree(&timer_card_reg);

    /* open dio device */
    dio_dev = comedi_open(daq_dev_file);
    if (dio_dev == NULL) {
        qDebug() << "Error opening dio dev file: " << daq_dev_file;
        throw;
    }
    
    /* lock the DIO device (dubdev 0) */
    err = comedi_lock(dio_dev, 0);
    if (err < 0) {
        qDebug() << "Error locking comedi device, subdevice: " << daq_dev_file << 0;
        throw;
    }

    timer_dev = comedi_open(timer_dev_file);
    if (timer_dev == NULL) {
        qDebug() << "Error opening timer dev file: " << timer_dev_file;
        throw;
    }

    /* lock the timer device (dubdev 2) */
    err = comedi_lock(timer_dev, 2);
    if (err < 0) {
        qDebug() << "Error locking comedi device, subdevice: " << timer_dev_file << 2;
        throw;
    }


    qDebug() << "NIDAQ Init";

    abort = false;
    pauseSampling = false;
    _pc = new MDDASPlotConfig();
    _pc->setXMax(8192);
    _pc->setXMin(0);
    _pc->setYMax(8192);
    _pc->setYMin(0);
    _pc->setPMax(256);
    _pc->setPMin(0);

    if (fclose(fp)) {
        qDebug() << "Error closing " << COM_PROC;
        throw;
    }
}

/* close thread */
NIDAQPlugin::~NIDAQPlugin() {
    comedi_close(dio_dev);
    comedi_close(timer_dev);
    
    free(daq_dev_file);
    free(timer_dev_file);
}


void NIDAQPlugin::run() {
    int i = 0;
    int j = 0;
    int num_photons = 0;

    QMutex sleepM;
    QVector<MDDASDataPoint> v;
    sleepM.lock();

    forever {
        mutex.lock();
        if (pauseSampling) {
            condition.wait(&mutex);
        }
        mutex.unlock();

        if (abort) {
            qDebug() << "called abort!" << QThread::currentThread();
            return;
        }
        
        //qDebug() << "nidaq!";

        condition.wait(&sleepM, 1);
            

        //qDebug() << "blip" << QThread::currentThread();
        //sleep();
        /* Use a wait condition instead of sleep() so that the thread
           can be awoken. */
        //condition.wait(&sleepM, 100);

    }
}

int NIDAQPlugin::ni_gpct_start_pulse_gen(comedi_t *device, 
                                         unsigned subdevice, 
                                         unsigned period_ns, 
                                         unsigned up_time_ns) {
    int retval;
    lsampl_t counter_mode;
    const unsigned clock_period_ns = 50; /* 20MHz clock */
    unsigned up_ticks, down_ticks;

    retval = comedi_reset(device, subdevice);
    if (retval < 0) return retval;

    retval = comedi_set_gate_source(device, 
                                    subdevice, 
                                    0, 
                                    0, 
                                    NI_GPCT_DISABLED_GATE_SELECT | CR_EDGE);
    if (retval < 0) return retval;
    retval = comedi_set_gate_source(device, 
                                    subdevice, 
                                    0, 
                                    1, 
                                    NI_GPCT_DISABLED_GATE_SELECT | CR_EDGE);
    if (retval < 0) {
        //fprintf(stderr, "Failed to set second gate source.  This is expected for older boards (e-series, etc.)\n"
        //        "that don't have a second gate.\n");
        qDebug() << "Failed to set second gate source.  This is expected for older boards (e-series, etc.)";
        qDebug() << "that don't have a second gate.";
            
    }

    counter_mode = NI_GPCT_COUNTING_MODE_NORMAL_BITS;
    /* toggle output on terminal count */
    counter_mode |= NI_GPCT_OUTPUT_TC_TOGGLE_BITS;
    /* load on terminal count */
    counter_mode |= NI_GPCT_LOADING_ON_TC_BIT;
    /* alternate the reload source between the load a and load b registers */
    counter_mode |= NI_GPCT_RELOAD_SOURCE_SWITCHING_BITS;
    /* count down */
    counter_mode |= NI_GPCT_COUNTING_DIRECTION_DOWN_BITS;
    /* initialize load source as load b register */
    counter_mode |= NI_GPCT_LOAD_B_SELECT_BIT;
    /* don't stop on terminal count */
    counter_mode |= NI_GPCT_STOP_ON_GATE_BITS;
    /* don't disarm on terminal count or gate signal */
    counter_mode |= NI_GPCT_NO_HARDWARE_DISARM_BITS;
    retval = comedi_set_counter_mode(device, subdevice, 0, counter_mode);
    if (retval < 0) return retval;

    /* 20MHz clock */
    retval = comedi_set_clock_source(device, 
                                     subdevice, 
                                     0, 
                                     NI_GPCT_TIMEBASE_1_CLOCK_SRC_BITS, 
                                     clock_period_ns);
    if (retval < 0) return retval;

    up_ticks = (up_time_ns + clock_period_ns / 2) / clock_period_ns;
    down_ticks = (period_ns + clock_period_ns / 2) / clock_period_ns - up_ticks;
    /* set initial counter value by writing to channel 0 */
    retval = comedi_data_write(device, subdevice, 0, 0, 0, down_ticks);
    if (retval < 0) return retval;
    /* set "load a" register to the number of clock ticks the counter
       output should remain low by writing to channel 1. */
    comedi_data_write(device, subdevice, 1, 0, 0, down_ticks);
    if (retval < 0) return retval;
    /* set "load b" register to the number of clock ticks the counter
       output should remain high by writing to channel 2 */
    comedi_data_write(device, subdevice, 2, 0, 0, up_ticks);
    if(retval < 0) return retval;

    retval = comedi_arm(device, subdevice, NI_GPCT_ARM_IMMEDIATE);
    if (retval < 0) return retval;

    return 0;
}

int NIDAQPlugin::ni_gpct_stop_pulse_gen(comedi_t *device, unsigned subdevice) {
    comedi_insn insn;
    lsampl_t data;
    
    memset(&insn, 0, sizeof(comedi_insn));
    insn.insn = INSN_CONFIG;
    insn.subdev = subdevice;
    insn.chanspec = 0;
    insn.data = &data;
    insn.n = 1;
    data = INSN_CONFIG_DISARM;

    if (comedi_do_insn(device, &insn) >= 0) {
        return 0;
    } else {
        return -1; 
    }
}

Q_EXPORT_PLUGIN2(nidaqplugin, NIDAQPlugin);

