QT += widgets
TEMPLATE        = lib
CONFIG         += plugin
CONFIG         += thread
INCLUDEPATH    += ../../gui ../ ../common
HEADERS         = chap10plugin.h \
                  socket.h \
                  ../common/samplingthreadplugin.h \
                  ../../gui/mddasdatainterface.h \
                  ../../gui/mddasdatapoint.h \
                  ../../gui/mddasplotconfig.h 
SOURCES         = chap10plugin.cpp \
                  socket.cpp \
                  ../common/samplingthreadplugin.cpp \
                  ../../gui/mddasdatainterface.cpp \
                  ../../gui/mddasdatapoint.cpp \
                  ../../gui/mddasplotconfig.cpp
TARGET          = $$qtLibraryTarget(chap10plugin)
DESTDIR         = ../../mddasplugins

message("CHAPTER 10 PLUGIN ---")
message($${TARGET})
