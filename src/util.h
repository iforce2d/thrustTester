#ifndef UTIL_H
#define UTIL_H

#include <set>
#include <string.h>
#include <QString>
#include <QColor>
#include <QComboBox>
#include "json/json.h"
#include <QFileInfo>

#define MAX_THREADS 16

class Vector2i {
public:
    Vector2i(int _x = 0, int _y = 0) : x(_x), y(_y) {}

public:
    int x, y;

    Vector2i operator-(const Vector2i& other) { Vector2i v( x - other.x, y - other.y ); return v; }
    bool operator==(const Vector2i& other) { return x == other.x && y == other.y; }
};

struct _keyState {
private:
    bool keys[256];
    std::set<int> extraKeys;
public:
    void init() {
        memset(keys, 0, sizeof(keys));
        extraKeys.clear();
    }

    void keyDown(int k) {
        if ( k < 256 )
            keys[k] = true;
        else
            extraKeys.insert(k);
    }

    void keyUp(int k) {
        if ( k < 256 )
            keys[k] = false;
        else
            extraKeys.erase(k);
    }

    bool isKeyDown(int k) {
        if ( k < 256 )
            return keys[k];
        else
            return extraKeys.find(k) != extraKeys.end();
    }
};

std::string currentTime(bool showDate);

class SetBoolTemporarily {
    bool* m_theVariable;
    bool m_originalValue;
public:
    SetBoolTemporarily(bool* theVariable, bool tempSetting) {
        m_theVariable = theVariable;
        m_originalValue = *theVariable;
        *theVariable = tempSetting;
    }
    ~SetBoolTemporarily() { *m_theVariable = m_originalValue; }
};

class SetVoidPtrTemporarily {
    void** m_theVariable;
    void* m_originalValue;
public:
    SetVoidPtrTemporarily(void** theVariable, void* tempSetting) {
        m_theVariable = theVariable;
        m_originalValue = *theVariable;
        *theVariable = tempSetting;
    }
    ~SetVoidPtrTemporarily() { *m_theVariable = m_originalValue; }
};

class SetIntTemporarily {
    int* m_theVariable;
    int m_originalValue;
public:
    SetIntTemporarily(int* theVariable, int tempSetting) {
        m_theVariable = theVariable;
        m_originalValue = *theVariable;
        *theVariable = tempSetting;
    }
    ~SetIntTemporarily() { *m_theVariable = m_originalValue; }
};

QString getUnknownString();
QString getBackupsFolderString();
QString getUntitledString(int i);
QString getTrueFalseString(bool b);
QString getMultipleValuesString();
QString getUnsetCustomPropertyValueString();
QString getNotApplicableString();
QString getUnsupportedJointTypeString();
QString getMultipleJointTypesString();
QString getAngleUnitsDisplayString();
QString getFeatureDisabledTrialString();
QString getNoBodyForImageString();
QString getNoBodyForSamplerString();

std::string floatToHex(float f);
float hexToFloat(std::string str);
void floatToJson(bool useHumanReadableFloats, const char* name, float f, Json::Value& value);
float jsonToFloat(const char* name, Json::Value& value, float defaultValue = 0);

std::string convertQStringToUTF8String( QString qs );
QString sanitizeStringForJSON( QString s );
std::string sanitizeStringForJSON(std::string s);
QString sanitizeStringForJSON_escapeDoubleQuotes( QString s );
std::string sanitizeStringForJSON_escapeDoubleQuotes(std::string s);

class ScopeLog {
public:
  QString m_functionName;
  ScopeLog(std::string fn);
  ~ScopeLog();
};

struct color3 {
    float red;
    float green;
    float blue;
    color3(float r, float g, float b) {
        red = r;
        green = g;
        blue = b;
    }
};

void setGLColor(color3 &c, float alpha);

template <typename T>
T constrain(T val, T a, T b) {
    if ( val < a ) return a;
    if ( val > b ) return b;
    return val;
}

void setCurrentIndexByItemData(QComboBox* box, QVariant v);

#endif // UTIL_H
