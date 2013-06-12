# Qwt stuff
INCLUDEPATH += /usr/include/qwt
LIBS += -lqwt

CONFIG -= release
CONFIG += debug

TARGET = mddas

#packagesExist(glib-2.0) {
#    message("")
#}


HEADERS       = colormaps.h \
                mainwindow.h \
                mddasdatainterface.h \
                mddasdatapoint.h \
                mddasplotconfig.h \ 
                samplingthreadinterface.h \
                scrollbar.h \
                scrollzoomer.h \
                specbox.h \
                specmonbox.h \
                spectromonitorscatterplot.h \
                spectroscatterplot.h \
                zoomer.h \
                eye.xpm \
                xicon.xpm

SOURCES       = mainwindow.cpp \
                main.cpp \
                mddasdatainterface.cpp \
                mddasdatapoint.cpp \
                mddasplotconfig.cpp \ 
                scrollbar.cpp \
                scrollzoomer.cpp \
                specbox.cpp \
                specmonbox.cpp \
                spectromonitorscatterplot.cpp \
                spectroscatterplot.cpp 



# install
#target.path = $$[QT_INSTALL_EXAMPLES]/mainwindows/menus
#sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS menus.pro
#sources.path = $$[QT_INSTALL_EXAMPLES]/mainwindows/menus
#INSTALLS += target sources

