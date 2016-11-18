#-------------------------------------------------
#
# Project created by QtCreator 2016-03-13T22:20:24
#
#-------------------------------------------------

QT += core gui opengl network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = tts
TEMPLATE = app
macx:INCLUDEPATH += /usr/local/include
macx:QMAKE_LIBDIR += /usr/local/lib

SOURCES += main.cpp\
        mainwindow.cpp \
    port.cpp \
    Log.cpp \
    util.cpp \
    jsoncpp.cpp \
    version.cpp \
    messages.cpp \
    GLCanvas.cpp \
    Canvas.cpp \
    Settings.cpp \
    vec2.cpp \
    TestTask.cpp \
    TestTask_ConstantThrottle.cpp \
    TestTask_ConstantThrust.cpp \
    PIDController.cpp \
    NoFocusButton.cpp \
    TestTask_Beep.cpp \
    CalibrationDialog.cpp \
    TestTask_ConstantRPM.cpp \
    LogTextBrowser.cpp \
    TestSetupDialog.cpp \
    scriptParsing.cpp \
    TestTask_Wait.cpp \
    PartSelectionDialog.cpp \
    parts.cpp \
    ExportDialog.cpp

HEADERS  += mainwindow.h \
    port.h \
    Log.h \
    util.h \
    json/json-forwards.h \
    json/json.h \
    version.h \
    messages.h \
    GLCanvas.h \
    Canvas.h \
    Settings.h \
    vec2.h \
    TestTask.h \
    TestTask_ConstantThrottle.h \
    TestTask_ConstantThrust.h \
    PIDController.h \
    NoFocusButton.h \
    TestTask_Beep.h \
    CalibrationDialog.h \
    TestTask_ConstantRPM.h \
    LogTextBrowser.h \
    TestSetupDialog.h \
    scriptParsing.h \
    TestTask_Wait.h \
    PartSelectionDialog.h \
    parts.h \
    ExportDialog.h

FORMS    += mainwindow.ui \
    calibrationdialog.ui \
    testsetupdialog.ui \
    partselectiondialog.ui \
    exportdialog.ui

win32:QMAKE_CXXFLAGS += -mno-ms-bitfields
win32:LIBS += ..\src\libserialport\libserialport.a -lopengl32 -lsetupapi

unix:LIBS += -lserialport
unix:!macx += -lX11 -lOpenGL

