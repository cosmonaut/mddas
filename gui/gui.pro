# QT stuff
QT += widgets
# Qwt stuff
INCLUDEPATH += /usr/include/qwt
#INCLUDEPATH += /usr/include/qt4/QtGui
LIBS += -lqwt -lCCfits -lcfitsio

CONFIG -= release
CONFIG += debug

TARGET = mddas

#packagesExist(glib-2.0) {
#    message("")
#}


HEADERS       = atomic.xpm \
                boxpicker.h \
                colormaps.h \
                fits.h \
                mainwindow.h \
                mddasconfigdialog.h \
                mddasdatainterface.h \
                mddasdatapoint.h \
                mddasplotconfig.h \ 
                numberbutton.h \
                histbox.h \
                incrementalhistplot.h \
                samplingthreadinterface.h \
                scrollbar.h \
                scrollzoomer.h \
                specbox.h \
                specmonbox.h \
                specplot.h \
                specplotbox.h \
                spectromonitorscatterplot.h \
                spectroscatterplot.h \
                superdata.h \
                zoomer.h \
                eye.xpm \
                save.xpm \
                xicon.xpm

SOURCES       = boxpicker.cpp \
                fits.cpp \
                mainwindow.cpp \
                main.cpp \
                mddasconfigdialog.cpp \
                mddasdatainterface.cpp \
                mddasdatapoint.cpp \
                mddasplotconfig.cpp \ 
                numberbutton.cpp \
                histbox.cpp \
                incrementalhistplot.cpp \
                scrollbar.cpp \
                scrollzoomer.cpp \
                specbox.cpp \
                specmonbox.cpp \
                specplot.cpp \
                specplotbox.cpp \
                spectromonitorscatterplot.cpp \
                spectroscatterplot.cpp \
                superdata.cpp



# install
#target.path = $$[QT_INSTALL_EXAMPLES]/mainwindows/menus
#sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS menus.pro
#sources.path = $$[QT_INSTALL_EXAMPLES]/mainwindows/menus
#INSTALLS += target sources

