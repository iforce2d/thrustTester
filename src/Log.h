
#ifndef FILE_LOG_H
#define FILE_LOG_H

#include <QString>
#include <stdarg.h>
#include <string>
using namespace std;

#define LOG_PRINT_BUFFER_SIZE 8192

#define LL_OFF				0
#define LL_DEBUG			1 << 0
#define LL_INFO				1 << 1
#define LL_SYSTEM_INFO      1 << 2
#define LL_SYSTEM_OUTPUT    1 << 3
#define LL_SCRIPT           1 << 4
#define LL_WARNING			1 << 5
#define LL_ERROR			1 << 6
#define LL_FATAL			1 << 7

#define LL_ALL (LL_FATAL | LL_ERROR | LL_WARNING | LL_INFO | LL_DEBUG | LL_SCRIPT | LL_SYSTEM_INFO | LL_SYSTEM_OUTPUT)

/**
Handles logging with different error levels.
Logs a formatted string to either a file or standard 
out, error messages will be ignored if the logging 
level is too high for them. The available log levels
are in order of least output to most output:  LL_OFF, 
LL_FATAL, LL_ERROR, LL_WARNING, LL_INFO, LL_DEBUG.
Initially the logger prints to standard out at log 
level LL_INFO, this can be changed by calling
setLevel() and setFile(). If a file is specified and
logging to it fails for some reason, output will go 
to standard error.
*/
class Log
{
private:

        string m_logFileName;
        int m_level;
        char m_printBuffer[LOG_PRINT_BUFFER_SIZE];

public:

        Log();
        ~Log();

	/**
	Sets the log level to use for selecting output. 
	This level can be changed at anytime.
        @param level a log level (one or more of LL_OFF,
	LL_FATAL, LL_ERROR, LL_WARNING, LL_INFO, LL_DEBUG)
	*/
        void setLevel(int p_level) { m_level = p_level; }
        void addLevel(int p_level) { m_level |= p_level; }
        void removeLevel(int p_level) { m_level &= ~p_level; }

	/**
	Sets a file to direct output to. The file for
	output can be changed at any time but after doing 
	so, output cannot be directed to standard out again.
	@param filename The new file, should be writable
	@param append Whether to append to an existing file 
	if there is one. If this is false the contents of
	an existing file will be lost.
	*/
        void setFile(string filename, bool append);

	/**
	Logs a formatted string to the currently specified
	output destination. 
	@param level The priority of this message (one of  
	LL_FATAL, LL_ERROR, LL_WARNING, LL_INFO, LL_DEBUG)
	@param msg A format string as for printf()
	@param ... Parameters to the format string
	*/
        void log(int level, QString msg, ...);

private:
        void vlog_ap(const char *fmt, va_list ap);

};

extern Log g_log;

#endif
