QT += widgets

TEMPLATE    = subdirs

# CU 40mm crosstrip plugin
SUBDIRS	    = cu40mmxsplugin

# NI PCI-DIO-32HS and PCI-6601 NSROC TM simulator 
#SUBDIRS += nidaqplugin

# DEWESOFT plugin (NSROC DECOM TM Custom Interface)
#SUBDIRS += deweplugin

# Chap10 plugin (Chapter 10 ethernet telemetry reader!)
SUBDIRS += chap10plugin

# NICODAQ plugin (new parallel daq card)
SUBDIRS += nicodaq
