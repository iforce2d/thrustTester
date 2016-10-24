#include "json/json.h"
#include "Log.h"
#include "parts.h"
#include "Settings.h"

QString getDisplayNameForMotor(int id)
{
    Json::Value arrayValue;
    Json::Reader reader;
    if ( reader.parse( g_motorData.toStdString(), arrayValue) ) {

        for (int i = 0; i < (int)arrayValue.size(); i++) {
            Json::Value part = arrayValue[i];

            int partId = part.get("id",0).asInt();
            if ( partId != id )
                continue;

            string maker = part.get("maker","").asString();
            string name = part.get("name","").asString();
            int diameter = part.get("diameter",0).asInt();
            int height = part.get("height",0).asInt();
            int kv = part.get("kv",0).asInt();

            QString displayText = QString("%1 %2 %3%4 %5kv").arg(maker.c_str()).arg(name.c_str()).arg(diameter).arg(height,2,10,QChar('0')).arg(kv);

            return displayText;
        }
    }
    else
        g_log.log(LL_ERROR, "Error parsing g_motorData string");

    return "Unknown";
}


QString getDisplayNameForESC(int id)
{
    Json::Value arrayValue;
    Json::Reader reader;
    if ( reader.parse( g_escData.toStdString(), arrayValue) ) {

        for (int i = 0; i < (int)arrayValue.size(); i++) {
            Json::Value part = arrayValue[i];

            int partId = part.get("id",0).asInt();
            if ( partId != id )
                continue;

            string maker = part.get("maker","").asString();
            string name = part.get("name","").asString();
            int current = part.get("current",0).asInt();

            QString displayText = QString("%1 %2 %3A").arg(maker.c_str()).arg(name.c_str()).arg(current);

            return displayText;
        }
    }
    else
        g_log.log(LL_ERROR, "Error parsing g_escData string");

    return "Unknown";
}


QString getDisplayNameForProp(int id)
{
    Json::Value arrayValue;
    Json::Reader reader;
    if ( reader.parse( g_propData.toStdString(), arrayValue) ) {

        for (int i = 0; i < (int)arrayValue.size(); i++) {
            Json::Value part = arrayValue[i];

            int partId = part.get("id",0).asInt();
            if ( partId != id )
                continue;

            string maker = part.get("maker","").asString();
            string name = "";//part.get("name","").asString();
            float diameter = part.get("diameter",0).asFloat();
            int blades = part.get("blades",0).asInt();
            float pitch = part.get("pitch",0).asFloat();
            string material = part.get("material","").asString();

            QString displayText = QString("%1 %2 %3x%4 %5 blade %6").arg(maker.c_str()).arg(name.c_str()).arg(diameter).arg(pitch).arg(blades).arg(material.c_str());

            return displayText;
        }
    }
    else
        g_log.log(LL_ERROR, "Error parsing g_propData string");

    return "Unknown";
}
