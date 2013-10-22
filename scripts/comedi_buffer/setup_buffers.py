#!/usr/bin/python

# Script to set comedi maximum buffer sizes to something more useful
# for high speed DAQ.
#
# Must be run as root!
#
# Nicholas Nell
# nicholas.nell@colorado.edu

import comedi as c
import re

# Card definitions...
COMEDI_PROC_F = "/proc/comedi"
PCI_6033E_PATTERN = '\s*(\d):\s*ni_pcimio\s*pci-6033e\s*\d\d'
PCI_DIO_32HS_PATTERN = '\s*(\d):\s*ni_pcidio\s*pci-dio-32hs\s*\d'
MAX_BUFFER_SIZE = 262144

def main():
    # Find comedi analog device number
    try:
        with open(COMEDI_PROC_F) as f:
            #print(f)
            for line in f.readlines():
                m_analog = re.match(PCI_6033E_PATTERN, line)
                m_digital = re.match(PCI_DIO_32HS_PATTERN, line)
                if m_analog:
                    print(m_analog.group())
                    dev_number = m_analog.groups()[0]
                    analog_dev_fname = "/dev/comedi" + dev_number
                if m_digital:
                    print(m_digital.group())
                    dev_number = m_digital.groups()[0]
                    digital_dev_fname = "/dev/comedi" + dev_number
    except IOError:
        print("Could not find file: %s" % COMEDI_PROC_F)

    if (f):
        f.close()

    if (analog_dev_fname):
        analog_dev = c.comedi_open(analog_dev_fname)
        if not(analog_dev):
            print("Unable to open analog comedi device...")
        
        ret = c.comedi_lock(analog_dev, 0)
        if (ret < 0):
            print("Could not lock comedi device")

        ret = c.comedi_get_max_buffer_size(analog_dev, 0)
        if (ret > 0):
            print("analog max buffer size: %i" % ret)
        else:
            print("Failed to get analog dev max buffer size...")

        ret = c.comedi_set_max_buffer_size(analog_dev, 0, MAX_BUFFER_SIZE)
        if (ret > 0):
            print("analog dev new buffer size: %i" % ret)
        else:
            print("Failed to analog dev set max buffer size...")

        c.comedi_close(analog_dev)


    if (digital_dev_fname):
        digital_dev = c.comedi_open(digital_dev_fname)
        if not(digital_dev):
            print("Unable to open digital comedi device...")
        
        ret = c.comedi_lock(digital_dev, 0)
        if (ret < 0):
            print("Could not lock digital comedi device")

        ret = c.comedi_get_max_buffer_size(digital_dev, 0)
        if (ret > 0):
            print("analog max buffer size: %i" % ret)
        else:
            print("Failed to get digital dev max buffer size...")

        ret = c.comedi_set_max_buffer_size(digital_dev, 0, MAX_BUFFER_SIZE)
        if (ret > 0):
            print("digital dev new buffer size: %i" % ret)
        else:
            print("Failed to digital dev set max buffer size...")

        c.comedi_close(digital_dev)


if __name__ == '__main__':
    r = main()
    if (r > 0):
        print("Setup buffers!")


