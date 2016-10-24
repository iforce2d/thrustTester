#include "mainwindow.h"
#include <QApplication>
#include <QThread>
#include "Log.h"
#include "version.h"
#include "port.h"
#include "Settings.h"

QThread* g_mainThread = NULL;

int main(int argc, char *argv[])
{
    g_log.setFile("tts.log", false);
    g_log.setLevel(LL_DEBUG);
    g_log.log( LL_INFO, QObject::tr("Starting TTS %d.%d.%d - hello!"), g_ttsVersion.major, g_ttsVersion.minor, g_ttsVersion.revision);

    g_mainThread = QThread::currentThread();

    readSettings();
    writeSettings();

    QApplication application(argc, argv);
    MainWindow w;
    g_mainWindow = &w;
    w.readPositionSettings();
    w.updateDisplaySettings();
    w.updateTestList();
    w.show();

    w.updatePortList();

    int ret = application.exec();

    w.closePort();

    g_log.log( LL_INFO, QObject::tr("Exiting TTS") );

    g_mainThread = NULL; // some destructors will check this to see if app is finishing

    return ret;
}
