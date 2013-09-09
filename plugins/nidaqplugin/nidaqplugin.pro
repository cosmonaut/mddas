TEMPLATE        = lib
CONFIG         += plugin
CONFIG         += thread
INCLUDEPATH    += ../../gui ../ ../common
LIBS           += -lcomedi
HEADERS         = nidaqplugin.h \
                  ../common/samplingthreadplugin.h \
                  ../../gui/mddasdatainterface.h \
                  ../../gui/mddasdatapoint.h \
                  ../../gui/mddasplotconfig.h 
SOURCES         = nidaqplugin.cpp \
                  ../common/samplingthreadplugin.cpp \
                  ../../gui/mddasdatainterface.cpp \
                  ../../gui/mddasdatapoint.cpp \
                  ../../gui/mddasplotconfig.cpp
TARGET          = $$qtLibraryTarget(nidaqplugin)
DESTDIR         = ../../mddasplugins

message("Creating NIDAQ plugin!")
message($${TARGET})

