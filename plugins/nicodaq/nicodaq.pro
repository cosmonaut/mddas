# Plugin for custom DAQ card built for UV Rockets

QT             += widgets
TEMPLATE        = lib
CONFIG         += plugin
CONFIG         += thread
INCLUDEPATH    += ../../gui ../ ../common
HEADERS         = nicodaq.h \
                  socket.h \
                  ../common/samplingthreadplugin.h \
                  ../../gui/mddasdatainterface.h \
                  ../../gui/mddasdatapoint.h \
                  ../../gui/mddasplotconfig.h 
SOURCES         = nicodaq.cpp \
                  socket.cpp \
                  ../common/samplingthreadplugin.cpp \
                  ../../gui/mddasdatainterface.cpp \
                  ../../gui/mddasdatapoint.cpp \
                  ../../gui/mddasplotconfig.cpp
TARGET          = $$qtLibraryTarget(nicodaqplugin)
DESTDIR         = ../../mddasplugins

message("NICODAQ BUILD!\n\n")
message($${TARGET})

# install
#target.path = $$[QT_INSTALL_EXAMPLES]/tools/echoplugin/plugin
#sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS plugin.pro
#sources.path = $$[QT_INSTALL_EXAMPLES]/tools/echoplugin/plugin
#INSTALLS += target sources

#symbian {
#    include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
#    TARGET.EPOCALLOWDLLDATA = 1
#}

#maemo5: include($$QT_SOURCE_TREE/examples/maemo5pkgrules.pri)
#symbian: warning(This example might not fully work on Symbian platform)
#maemo5: warning(This example might not fully work on Maemo platform)
#simulator: warning(This example might not fully work on Simulator platform)
