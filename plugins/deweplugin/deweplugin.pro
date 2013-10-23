TEMPLATE        = lib
CONFIG         += plugin
CONFIG         += thread
INCLUDEPATH    += ../../gui ../ ../common
HEADERS         = deweplugin.h \
                  socket.h \
                  ../common/samplingthreadplugin.h \
                  ../../gui/mddasdatainterface.h \
                  ../../gui/mddasdatapoint.h \
                  ../../gui/mddasplotconfig.h 
SOURCES         = deweplugin.cpp \
                  socket.cpp \
                  ../common/samplingthreadplugin.cpp \
                  ../../gui/mddasdatainterface.cpp \
                  ../../gui/mddasdatapoint.cpp \
                  ../../gui/mddasplotconfig.cpp
TARGET          = $$qtLibraryTarget(deweplugin)
DESTDIR         = ../../mddasplugins

message("DEWEPLUGIN ---")
message($${TARGET})
