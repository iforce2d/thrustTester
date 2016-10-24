#include <QtGui/QApplication>
#include <QThread>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "Log.h"
#include "mainwindow.h"
#include "util.h"

bool g_showDebugMessages = true;

Log g_log;

const char* getLogLevelLabel(int level)
{
    switch (level) {
    case LL_SYSTEM_INFO :
    case LL_SYSTEM_OUTPUT : return "  SYSTEM "; break;
    case LL_SCRIPT :        return "  SCRIPT "; break;
    case LL_DEBUG :         return "  DEBUG  "; break;
    case LL_INFO :          return "  INFO   "; break;
    case LL_WARNING :       return "  WARNING"; break;
    case LL_ERROR :         return "* ERROR  "; break;
    case LL_FATAL :         return "* FATAL  "; break;
    }
    return "";
}

Log::Log()
{
    m_level = LL_INFO;
}

Log::~Log() {
    log(LL_INFO, QObject::tr("Finished log\n"));
}

void Log::setFile(string p_filename, bool p_append)
{
    m_logFileName = p_filename;

    if ( !p_append ) {
        if ( FILE* t_logfile = fopen(m_logFileName.c_str(), "w") )
            fclose(t_logfile);
        log(LL_INFO, QObject::tr("Started log"));
    }
}

//http://blog.julipedia.org/2011/09/using-vacopy-to-safely-pass-ap.html
void Log::vlog_ap(const char *fmt, va_list ap)
{
    va_list ap2;
    va_copy(ap2, ap);
    vsnprintf(m_printBuffer, sizeof(m_printBuffer), fmt, ap2);
    va_end(ap2);
}

void Log::log(int p_level, QString fmt, ...)
{
    if ( p_level == LL_DEBUG && !g_showDebugMessages )
        return;

    //if ( (p_level & m_level) == 0 )
    if ( p_level < m_level )
        return;

    FILE* t_target = NULL;
    if ( !m_logFileName.empty() )
        t_target = fopen(m_logFileName.c_str(), "a");

    va_list ap;
    va_start(ap, fmt);
    vlog_ap(fmt.toUtf8(), ap);
    va_end(ap);

    if (t_target) {
        fprintf(t_target, "%s [%s]: %s\n", currentTime(false).c_str(), getLogLevelLabel(p_level), m_printBuffer);
        fclose(t_target);//ensures tail works properly
    }
    else {
        printf("%s [%s]: %s\n", currentTime(false).c_str(), getLogLevelLabel(p_level), m_printBuffer);
        fflush(stdout);
    }

    if ( g_mainWindow )
        g_mainWindow->log(p_level, currentTime(false), m_printBuffer);
}


