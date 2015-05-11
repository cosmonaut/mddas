/* Author: Nicholas Nell
   email: nicholas.nell@colorado.edu

   Sampling thread for the NI PCI-DIO-32HS and PCI-6601 that act as
   NSROC telemtery.  
*/

#include <QDebug>
#include <regex.h>
#include <unistd.h>

#include "nidaqplugin.h"


NIDAQPlugin::NIDAQPlugin() : SamplingThreadPlugin() {
    FILE *fp;
    char line_buffer[255];
    uint32_t line_number = 0;
    int err = 0;
    int s = 0;

    regex_t daq_card_reg;
    regex_t timer_card_reg;

    regmatch_t daq_match[2];
    regmatch_t timer_match[2];

    unsigned int chanlist[COM_N_CHAN];

    qDebug() << "NIDAQ Init Starting...";

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
            qDebug() << "DIO dev file: " << daq_dev_file;
        }

        if (!regexec(&timer_card_reg, line_buffer, 2, timer_match, 0)) {
            timer_dev_file = (char *)malloc(strlen(COM_PREFIX) + 2);
            sprintf(timer_dev_file, 
                    "%s%.*s", 
                    COM_PREFIX, 
                    timer_match[1].rm_eo - timer_match[1].rm_so, 
                    &line_buffer[timer_match[1].rm_so]);
            qDebug() << "TIMER dev file: " << timer_dev_file;
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

    s = comedi_get_buffer_size(dio_dev, 0);
    qDebug() << "NIDAQ DIO_DEV buffer size: " << s;
    s = comedi_get_max_buffer_size(dio_dev, 0);
    qDebug() << "NIDAQ DIO_DEV max buffer size: " << s;
    if (s > 0) {
        s = comedi_set_buffer_size(dio_dev, 0, s);
        qDebug() << "NIDAQ DIO_DEV buffer size: " << s;
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

    /* Don't start pulse train until we acquire */
    if (ni_gpct_stop_pulse_gen(timer_dev, 2) != 0) {
        qDebug() << "Unable to stop pulse train...";
        throw;
    }

    /* Set device file params */
    fcntl(comedi_fileno(dio_dev), F_SETFL, O_NONBLOCK);

    memset(&cmd, 0, sizeof(cmd));

    /* This command is legit for the PCI-DIO-32HS */
    cmd.subdev         = 0;
    cmd.flags          = 0;
    cmd.start_src      = TRIG_NOW;
    cmd.start_arg      = 0;
    cmd.scan_begin_src = TRIG_EXT;
    cmd.scan_begin_arg = CR_INVERT; /* Read on the trailing edge */
    cmd.convert_src    = TRIG_NOW;
    cmd.convert_arg    = 0;
    cmd.scan_end_src   = TRIG_COUNT;
    cmd.scan_end_arg   = COM_N_CHAN;
    cmd.stop_src       = TRIG_NONE;
    cmd.stop_arg       = 0;

    cmd.chanlist = chanlist;
    cmd.chanlist_len = COM_N_CHAN;

    /* Prep all channels */
    for (int i = 0; i < COM_N_CHAN; i++) {
        /* Note range is 0 (we are digital!) */
        chanlist[i] = CR_PACK(0, 0, AREF_GROUND);
    }

    /* Test the command */
    err = comedi_command_test(dio_dev, &cmd);
    if (err != 0) {
        qDebug() << "comedi command failed test.";
        qDebug() << "comedi_command_test result: " <<  err;
        int blah = comedi_get_version_code(dio_dev);
        qDebug() << 
            "COMEDI VER: " << 
            (0x0000ff & (blah >> 16)) << 
            "." << (0x0000ff & (blah >> 8)) << 
            "." << (0x0000ff & blah);

        throw;
    }

    err = comedi_dio_config(timer_dev, 1, 36, COMEDI_OUTPUT);
    if (err != 0) {
        qDebug() << "Failed to set timer to output. Error #: " << err;
        throw;
    }

    /* Start Command! */
    err = comedi_command(dio_dev, &cmd);
    if (err < 0) {
        qDebug() << "Failed to start command!";
    }

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
    
    qDebug() << "NIDAQ Init complete...";
}

/* close thread */
NIDAQPlugin::~NIDAQPlugin() {
    ni_gpct_stop_pulse_gen(timer_dev, 2);
    /* stop command */
    comedi_cancel(dio_dev, 0);

    comedi_close(dio_dev);
    comedi_close(timer_dev);
    
    free(daq_dev_file);
    free(timer_dev_file);
}


void NIDAQPlugin::run() {
    int i = 0;
    int j = 0;
    uint8_t n = 0;
    int num_photons = 0;
    int ret = 0;
    int row = 0;
    uint8_t break_sw = 0;
    
    /* Keep track of # of garbled samples */
    uint32_t garb_samps = 0;
    uint16_t cur_word[] = {0, 0, 0};
    uint16_t stored_word[] = {0, 0, 0};

    uint32_t samples = 0;
    uint32_t remainder = 0;
    uint32_t total_samples = 0;
    uint16_t first_word = 0x0000;
    /* Array of word encodings X, Y, P */
    uint16_t word_codes[] = {0x2000, 0x4000, 0x6000};

    fd_set rdset;
    struct timeval timeout;

    timeout.tv_sec = 0;
    timeout.tv_usec = 50000;

    QMutex sleepM;
    QVector<MDDASDataPoint> v;
    qDebug() << "v size: " << v.size();
    /* garbled sample log */
    QVector<uint16_t> gs;
    sleepM.lock();

    FD_ZERO(&rdset);
    FD_SET(comedi_fileno(dio_dev), &rdset);


    forever {
        mutex.lock();
        if (pauseSampling) {
            qDebug() << "pause";
            if (ni_gpct_stop_pulse_gen(timer_dev, 2) != 0) {
                qDebug() << "failed to pause!";
            }
            // Need to clear total samples and memset buf to 0
            condition.wait(&mutex);
            qDebug() << "unpause";
            ni_gpct_start_pulse_gen(timer_dev, 2, STROBE_PERIOD, STROBE_HIGH_T);
        }
        mutex.unlock();

        if (abort) {
            qDebug() << "called abort!" << QThread::currentThread();
            return;
        }
        
        //qDebug() << "nidaq!";

        
        
        ret = select(comedi_fileno(dio_dev) + 1, &rdset, NULL, NULL, &timeout);
        if (ret < 0) {
            qDebug() << "select() error!";
            perror("select()");
        } else if (ret == 0) {
            /* hit timeout, poll card */
            ret = comedi_poll(dio_dev, 0);
            if (ret < 0) {
                qDebug() << "comedi_poll() error";
                comedi_perror("comedi_poll()");
            }
        } else if (FD_ISSET(comedi_fileno(dio_dev), &rdset)) {
            /* comedi file descriptor became ready */
            ret = read(comedi_fileno(dio_dev), buf, sizeof(buf));
            if (ret < 0) {
                qDebug() << "read() error!";
                perror("read()");
            } else if (ret == 0) {
                /* no data */
                //qDebug() << "no data...";
            } else {
                //qDebug() << "Got " << ret << " samples";
                samples = ret/sizeof(lsampl_t);
                total_samples += samples;
                /* check that we always read an integer number of
                   samples */
                if (ret%4 != 0) {
                    qDebug() << "SAMPLE SIZE FAIL!";
                    qDebug() << ret%4;
                    qDebug() << ret;
                }
                //qDebug() << "samples: " << samples;
                //qDebug() << "tot samples: " << total_samples;

                i = 0;
                /* Loop through buffer looking for first non-zero
                   word */
                while (((uint16_t)buf[i]) == 0) {
                    i++;
                    /* might all be zeros... */
                    if (i >= samples) {
                        break;
                    }
                }

                //qDebug() << "zeros: " << i;
                /* Number of potentially good words left in buffer */
                remainder = samples - i;

                /* Only continue if there was a non-zero word */
                if (i < samples) {
                    //qDebug() << "buf i: " << buf[i];
                    while (i < samples) {
                        /* Find type of first word in buffer */
                        // if ((uint16_t)buf[i] != 0xdfff) {
                        //     qDebug() << (uint16_t)buf[i];
                        // }
                        first_word = ((uint16_t)buf[i]) & 0xE000;
                        switch(first_word) {
                        case 0x2000:
                            //qDebug() << "X";
                            //x = first_word;
                            cur_word[0] = ((uint16_t)buf[i] & 0x1fff);
                            n = 1;
                            break_sw = 1;
                            break;
                        case 0x4000:
                            //qDebug() << "Y";
                            n = 2;
                            break_sw = 1;
                            break;
                        case 0x6000:
                            //qDebug() << "P";
                            n = 0;
                            break_sw = 1;
                            break;
                        default:
                            /* Garbled data... */
                            //QString garbhex = QString("%1").arg(buf[i], 0, 16);
                            //qDebug() << "GARBLED DATA: " << first_word << " " << garbhex;
                            garb_samps++;
                            gs.append((uint16_t)buf[i] & 0xe000);
                            break;
                        }
                        if (break_sw) {
                            break;
                        }
                        i++;
                    }
                    
                    /* Only add first word if data really exists... */
                    if (i < samples) {
                        if (n == 0) {
                            /* We should already have X and Y */
                            cur_word[0] = stored_word[0];
                            cur_word[1] = stored_word[1];
                            cur_word[2] = ((uint16_t)buf[i] & 0x1fff);
                            v.append(MDDASDataPoint(cur_word[0], cur_word[1], cur_word[2]));
                        } else if (n == 2) {
                            /* We should already have X */
                            cur_word[0] = stored_word[0];
                            cur_word[1] = ((uint16_t)buf[i] & 0x1fff);
                        }
                    }

                    /* Loop through remainder after first non-zero
                       word */
                    for (row = i + 1; row < samples; row++) {
                        /* Get rid of all 0 samples -- no data */
                        if (((uint16_t)buf[row]) != 0) {
                            //qDebug() << (uint16_t)buf[row];
                            if (((uint16_t)buf[row] & 0xE000) == word_codes[n]) {
                                /* got next word */
                                cur_word[n] = ((uint16_t)buf[row] & 0x1fff);
                                
                                if (n == 2) {
                                    /* Append word */
                                    v.append(MDDASDataPoint(cur_word[0], cur_word[1], cur_word[2]));
                                }

                                n++;
                                n = n%3;
                                //qDebug() << "good data?";
                            } else {
                                // got bad next word
                                /* Log # of bad words */
                                /* Reset to look for X */
                                garb_samps++;
                                gs.append((uint16_t)buf[row] & 0xe000);
                            }
                        
                        }
                    }
                    
                    /* Fill stored word here */
                    /* Add better logic for this later? */
                    stored_word[0] = cur_word[0];
                    stored_word[1] = cur_word[1];
                    stored_word[2] = cur_word[2];
                    cur_word[0] = 0;
                    cur_word[1] = 0;
                    cur_word[2] = 0;

                    if (v.size() > 0) {
                        /* Send data to mddas */
                        _di.bufEnqueue(v);
                        v.clear();
                        v.squeeze();
                    }

                    /* debugging tool for garbled words... */
                    if (garb_samps > 0) {
                        //qDebug() << "Garb samps " << garb_samps;
                        // for (j = 0; j < gs.size(); j++) {
                        //     QString gshex = QString("%1").arg(gs.value(j), 0, 16);
                        //     qDebug() << gs.value(j) << "hex: " << gshex;
                        // }
                        //QString gshex = QString("%1").arg(gs.value(garb_samps - 1), 0, 16);
                        //qDebug() << gs.value(garb_samps - 1) << "hex: " << gshex;
                        // QString swx = QString("%1").arg(stored_word[0], 0, 16);
                        // qDebug() << swx;
                        gs.clear();
                    }
                    garb_samps = 0;
                    n = 0;

                }
            }
        }
                
            
        




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

