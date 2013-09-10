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

Q_EXPORT_PLUGIN2(nidaqplugin, NIDAQPlugin);

