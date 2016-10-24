#include "util.h"
#include <QDateTime>
#include "Log.h"
#include <time.h>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

std::string currentTime(bool showDate)
{
    const char* dateAndTime = "%c";
    const char* timeOnly = "%H:%M:%S";
    const char* fmt = showDate ? dateAndTime : timeOnly;
    time_t t;
    time(&t);
    char buf[64];
    strftime(buf, 64, fmt, localtime(&t));
    std::string s(buf);
    return s;
}

std::string floatToHex(float f)
{
    char buf[16];
    //sprintf(buf, "%08X", *((int*)(&f)) ); dereferencing type-punned pointer will break strict-aliasing rules
    int* i = (int*)(&f);
    sprintf(buf, "%08X", *i );
    return std::string(buf);
}

float hexToFloat(std::string str)
{
    int strLen = 8;//32 bit float
    unsigned char bytes[4];
    int bptr = (strLen / 2) - 1;

    for (int i = 0; i < strLen; i++){
        unsigned char   c;
        c = str[i];
        if (c > '9') c -= 7;
        c <<= 4;
        bytes[bptr] = c;

        ++i;
        c = str[i];
        if (c > '9') c -= 7;
        c -= '0';
        bytes[bptr] |= c;

        --bptr;
    }

    //return *((float*)bytes); dereferencing type-punned pointer will break strict-aliasing rules
    float* f = (float*)bytes;
    return *f;
}

void floatToJson(bool useHumanReadableFloats, const char* name, float f, Json::Value& value)
{
    //cut down on file space for common values
    if ( f == 0 )
        value[name] = 0;
    else if ( f == 1 )
        value[name] = 1;
    else {
        if ( useHumanReadableFloats )
            value[name] = f;
        else
            value[name] = floatToHex(f);
    }
}

float jsonToFloat(const char* name, Json::Value& value, float defaultValue)
{
    if ( ! value.isMember(name) )
        return defaultValue;

    if ( value[name].isNull() )
        return defaultValue;
    else if ( value[name].isInt() )
        return value[name].asInt();//usually 0 or 1
    else if ( value[name].isString() )
        return hexToFloat( value[name].asString() );
    else
        return value[name].asFloat();
}

bool renameExistingFileAppendModifiedTime( QString filename )
{
    QFileInfo fileInfo( filename );
    QDateTime dt = fileInfo.lastModified();
    QString thePath = fileInfo.path();
    QString theBaseName = fileInfo.completeBaseName();
    QString theSuffix = fileInfo.suffix();
    QString newName = QString::fromUtf8("%1/%2-%3%4%5-%6%7%8.%9")
            .arg(thePath)
            .arg(theBaseName)
            .arg(dt.date().year()-2000,2,10,QChar::fromAscii('0'))
            .arg(dt.date().month(),2,10,QChar::fromAscii('0'))
            .arg(dt.date().day(),2,10,QChar::fromAscii('0'))
            .arg(dt.time().hour(),2,10,QChar::fromAscii('0'))
            .arg(dt.time().minute(),2,10,QChar::fromAscii('0'))
            .arg(dt.time().second(),2,10,QChar::fromAscii('0'))
            .arg(theSuffix);
    QFile f(filename);
    bool ret = f.rename( newName );
    if ( ret )
        g_log.log(LL_DEBUG, QObject::tr("Renamed file '%1' to '%2'").arg(filename).arg(newName));
    else
        g_log.log(LL_WARNING, QObject::tr("Failed to rename file '%1' to '%2'").arg(filename).arg(newName));
    return ret;
}

std::string convertQStringToUTF8String( QString qs )
{
    const QByteArray asc = qs.toUtf8();
    return string( asc.constData(), asc.length() );
}

QString sanitizeStringForJSON( QString s )
{
    s = s.replace( QString::fromUtf8("\\"), QString::fromUtf8("\\\\") );
    s = s.replace( QString::fromUtf8("\n"), QString::fromUtf8("") );
    s = s.replace( QString::fromUtf8("\\n"), QString::fromUtf8("") );
    s = s.replace( QString::fromUtf8("'"), QString::fromUtf8("\\'") );
    return s;
}

string sanitizeStringForJSON(string s)
{
    QString qs = QString::fromUtf8(s.c_str());
    qs = sanitizeStringForJSON(qs);
    return convertQStringToUTF8String(qs);
}

QString sanitizeStringForJSON_escapeDoubleQuotes( QString s )
{
    s = s.replace( QString::fromUtf8("\\"), QString::fromUtf8("\\\\") );
    s = s.replace( QString::fromUtf8("\n"), QString::fromUtf8("") );
    s = s.replace( QString::fromUtf8("\\n"), QString::fromUtf8("") );
    s = s.replace( QString::fromUtf8("\""), QString::fromUtf8("\\\"") );
    return s;
}

string sanitizeStringForJSON_escapeDoubleQuotes(string s)
{
    QString qs = QString::fromUtf8(s.c_str());
    qs = sanitizeStringForJSON_escapeDoubleQuotes(qs);
    return convertQStringToUTF8String(qs);
}

ScopeLog::ScopeLog(std::string fn) {
    m_functionName = QString::fromUtf8(fn.c_str());
    g_log.log( LL_DEBUG, QString::fromUtf8("Entered %1").arg(m_functionName) );
}
ScopeLog::~ScopeLog() {
    g_log.log( LL_DEBUG, QString::fromUtf8("Exiting %1").arg(m_functionName) );
}

void setGLColor(color3& c, float alpha)
{
    glColor4f(c.red, c.green, c.blue, alpha);
}

void setCurrentIndexByItemData(QComboBox *box, QVariant v)
{
    for (int i = 0; i < box->count(); i++) {
        if ( box->itemData(i) == v ) {
            box->setCurrentIndex(i);
            return;
        }
    }
}
