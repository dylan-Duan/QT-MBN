QT       += core gui charts widgets sql


SOURCES += main.cpp \
           dbloader.cpp \
           kiss_fft.c \
           mainwindow.cpp \
           signalprocessor.cpp \
           sqlite3.c


HEADERS += mainwindow.h \
           dbloader.h \
           kiss_fft.h \
           kiss_fft_log.h \
           kiss_fftr.h \
           signalprocessor.h \
           sqlite3.h \
           sqlite3ext.h


FORMS += mainwindow.ui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

INCLUDEPATH += path/to/kissfft

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    sqlite3.def \
    sqlite3.dll
