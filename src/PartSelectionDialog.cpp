#include <QUrl>
#include <QCloseEvent>
#include <QKeyEvent>
#include "json/json.h"
#include "PartSelectionDialog.h"
#include "ui_partselectiondialog.h"
#include "Log.h"
#include "Settings.h"
#include "util.h"

PartSelectionDialog::PartSelectionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PartSelectionDialog)
{
    ui->setupUi(this);

    m_ignoreComboboxSelections = false;

    m_urls[PC_MOTOR]    = "/tts/motors.php";
    m_urls[PC_ESC]      = "/tts/escs.php";
    m_urls[PC_PROP]     = "/tts/props.php";
    m_urls[PC_MCU]      = "/tts/mcus.php";
    m_urls[PC_MATERIAL] = "/tts/materials.php";

    // these will be displayed in the dialog
    m_partNames.append("motor");
    m_partNames.append("ESC");
    m_partNames.append("propeller");
    m_partNames.append("MCU");
    m_partNames.append("material");

    // these are for keys in the saved settings
    m_settingsKeys.append("motor");
    m_settingsKeys.append("esc");
    m_settingsKeys.append("prop");
    m_settingsKeys.append("mcu");
    m_settingsKeys.append("material");

    m_requestInProgress = false;

    //m_http = new QHttp(this);
    //connect(m_http, SIGNAL(done(bool)), this, SLOT(onRequestCompleted(bool)));

    {
        SetBoolTemporarily dummy(&m_ignoreComboboxSelections, true);
        updateMotorCombobox();
        updateESCCombobox();
        updatePropCombobox();
        updateMcuCombobox();
        updateMaterialCombobox();
    }
}

PartSelectionDialog::~PartSelectionDialog()
{
    delete ui;
}

void PartSelectionDialog::accept()
{
    if ( m_requestInProgress )
        return;

    writeSettings();
    QDialog::accept();
}

void PartSelectionDialog::reject()
{
    if ( m_requestInProgress )
        return;

    writeSettings();
    QDialog::reject();
}

void PartSelectionDialog::on_partsDownloadButton_clicked()
{
    ui->partsDownloadButton->setEnabled(false);

    m_currentPartCategory = 0;
    m_requestInProgress = true;

    ui->partsDownloadTextBrowser->append( "Contacting server..." );

    startRequest();
}

void PartSelectionDialog::startRequest()
{
    QUrl url;
    url.setPath( m_urls[m_currentPartCategory] );

    //url.addQueryItem( "param" , value);

    /*m_http->setHost( "www.iforce2d.net" );
    m_http->get(url.toString());*/

    QUrl serviceUrl = QUrl( QString("http://www.iforce2d.net/") + m_urls[m_currentPartCategory] );
    QNetworkRequest request( serviceUrl );

    QByteArray data;

    QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
    connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onRequestCompleted(QNetworkReply*)));
    networkManager->post(request, data );

    g_log.log(LL_DEBUG, QString("Downloading %1 data...").arg( m_partNames[m_currentPartCategory]));
}

void PartSelectionDialog::onRequestCompleted(QNetworkReply *reply)
{
    g_log.log(LL_DEBUG, QString("Finished request, error:%1").arg(reply->error()));

    m_requestInProgress = false;

    if ( reply->error() ) {
        ui->partsDownloadTextBrowser->append( "Error contacting server." );
        ui->partsDownloadTextBrowser->append( reply->errorString() );

        ui->partsDownloadButton->setEnabled(true);

        return;
    }

    QString jsonStr = QString::fromUtf8( reply->readAll() );
    g_log.log(LL_DEBUG, QString("  response: %1").arg(jsonStr));

    Json::Value responseValue;
    Json::Reader reader;
    if ( ! reader.parse(jsonStr.toStdString(), responseValue) )
    {
        QString("Failed to parse JSON : %1").arg( QString(reader.getFormatedErrorMessages().c_str()) );
        ui->partsDownloadTextBrowser->append( "Error parsing response." );

        ui->partsDownloadButton->setEnabled(true);

        return;
    }

    if ( !responseValue.isArray()  ) {
        ui->partsDownloadTextBrowser->append( "Error finding main array." );

        ui->partsDownloadButton->setEnabled(true);

        return;
    }

    // save
    switch (m_currentPartCategory) {
    case PC_MOTOR:    g_motorData = jsonStr; break;
    case PC_ESC:      g_escData = jsonStr; break;
    case PC_PROP:     g_propData = jsonStr; break;
    case PC_MCU:      g_mcuData = jsonStr; break;
    case PC_MATERIAL: g_materialData = jsonStr; break;
    default: g_log.log(LL_ERROR, "Invalid part category");
    }

    m_currentPartCategory++;
    if ( m_currentPartCategory < PC_MAX ) {
        startRequest();
        return;
    }

    // success!

    writeSettings();

    g_log.log(LL_DEBUG, QString("... finished."));

    ui->partsDownloadTextBrowser->append( "Download finished OK." );
    ui->partsDownloadTextBrowser->append( "Updating UI..." );

    {
        SetBoolTemporarily dummy(&m_ignoreComboboxSelections, true);
        updateMotorCombobox();
        updateESCCombobox();
        updatePropCombobox();
        updateMcuCombobox();
        updateMaterialCombobox();
    }

    ui->partsDownloadTextBrowser->append( "Done." );

    ui->partsDownloadButton->setEnabled(true);
}

void PartSelectionDialog::updateMotorCombobox()
{
    g_log.log(LL_DEBUG, __PRETTY_FUNCTION__);

    if ( g_motorData == "" )
        return; // probably first time run

    Json::Value arrayValue;
    Json::Reader reader;
    if ( ! reader.parse( g_motorData.toStdString(), arrayValue) )
    {
        QString("Failed to parse motor JSON : %1").arg( QString(reader.getFormatedErrorMessages().c_str()) );
        ui->partsDownloadTextBrowser->append( "Error parsing response." );

        return;
    }

    ui->motorSelectComboBox->clear();
    ui->motorSelectComboBox->addItem("Unknown", -1);

    for (int i = 0; i < (int)arrayValue.size(); i++) {
        Json::Value part = arrayValue[i];

        int partId = part.get("id",0).asInt();
        string maker = part.get("maker","").asString();
        string name = part.get("name","").asString();
        int diameter = part.get("diameter",0).asInt();
        int height = part.get("height",0).asInt();
        int kv = part.get("kv",0).asInt();

        QString displayText = QString("%1 %2 %3%4 %5kv").arg(maker.c_str()).arg(name.c_str()).arg(diameter).arg(height,2,10,QChar('0')).arg(kv);

        ui->motorSelectComboBox->addItem(displayText, partId);
    }

    setCurrentIndexByItemData(ui->motorSelectComboBox, g_selectedMotorId);
}

void PartSelectionDialog::updateESCCombobox()
{
    g_log.log(LL_DEBUG, __PRETTY_FUNCTION__);

    if ( g_escData == "" )
        return; // probably first time run

    Json::Value arrayValue;
    Json::Reader reader;
    if ( ! reader.parse( g_escData.toStdString(), arrayValue) )
    {
        QString("Failed to parse ESC JSON : %1").arg( QString(reader.getFormatedErrorMessages().c_str()) );
        ui->partsDownloadTextBrowser->append( "Error parsing response." );

        return;
    }

    ui->escSelectComboBox->clear();
    ui->escSelectComboBox->addItem("Unknown", -1);

    for (int i = 0; i < (int)arrayValue.size(); i++) {
        Json::Value part = arrayValue[i];

        int partId = part.get("id",0).asInt();
        string maker = part.get("maker","").asString();
        string name = part.get("name","").asString();
        int current = part.get("current",0).asInt();

        QString displayText = QString("%1 %2 %3A").arg(maker.c_str()).arg(name.c_str()).arg(current);

        ui->escSelectComboBox->addItem(displayText, partId);
    }

    setCurrentIndexByItemData(ui->escSelectComboBox, g_selectedEscId);
}

void PartSelectionDialog::updatePropCombobox()
{
    g_log.log(LL_DEBUG, __PRETTY_FUNCTION__);

    if ( g_propData == "" )
        return; // probably first time run

    Json::Value arrayValue;
    Json::Reader reader;
    if ( ! reader.parse( g_propData.toStdString(), arrayValue) )
    {
        QString("Failed to parse prop JSON : %1").arg( QString(reader.getFormatedErrorMessages().c_str()) );
        ui->partsDownloadTextBrowser->append( "Error parsing response." );

        return;
    }

    ui->propSelectComboBox->clear();
    ui->propSelectComboBox->addItem("Unknown", -1);

    for (int i = 0; i < (int)arrayValue.size(); i++) {
        Json::Value part = arrayValue[i];

        int partId = part.get("id",0).asInt();
        string maker = part.get("maker","").asString();
        string name = "";//part.get("name","").asString();
        float diameter = part.get("diameter",0).asFloat();
        int blades = part.get("blades",0).asInt();
        float pitch = part.get("pitch",0).asFloat();
        string material = part.get("material","").asString();

        QString displayText = QString("%1 %2 %3x%4 %5 blade %6").arg(maker.c_str()).arg(name.c_str()).arg(diameter).arg(pitch).arg(blades).arg(material.c_str());

        ui->propSelectComboBox->addItem(displayText, partId);
    }

    setCurrentIndexByItemData(ui->propSelectComboBox, g_selectedPropId);
}

void PartSelectionDialog::updateMcuCombobox()
{
    g_log.log(LL_DEBUG, __PRETTY_FUNCTION__);

    if ( g_mcuData == "" )
        return; // probably first time run

     Json::Value arrayValue;
     Json::Reader reader;
     if ( ! reader.parse( g_mcuData.toStdString(), arrayValue) )
     {
         QString("Failed to parse mcu JSON : %1").arg( QString(reader.getFormatedErrorMessages().c_str()) );
         ui->partsDownloadTextBrowser->append( "Error parsing response." );

         return;
     }

     ui->escMcuComboBox->clear();

     for (int i = 0; i < (int)arrayValue.size(); i++) {
         Json::Value part = arrayValue[i];

         int partId = part.get("id",0).asInt();
         string maker = part.get("maker","").asString();
         string mcu = part.get("mcu","").asString();

         QString displayText = QString("%1 %2").arg(maker.c_str()).arg(mcu.c_str());

         ui->escMcuComboBox->addItem(displayText, partId);
     }
}

void PartSelectionDialog::updateMaterialCombobox()
{
    g_log.log(LL_DEBUG, __PRETTY_FUNCTION__);

    if ( g_materialData == "" )
        return; // probably first time run

     Json::Value arrayValue;
     Json::Reader reader;
     if ( ! reader.parse( g_materialData.toStdString(), arrayValue) )
     {
         QString("Failed to parse material JSON : %1").arg( QString(reader.getFormatedErrorMessages().c_str()) );
         ui->partsDownloadTextBrowser->append( "Error parsing response." );

         return;
     }

     ui->propMaterialComboBox->clear();

     for (int i = 0; i < (int)arrayValue.size(); i++) {
         Json::Value part = arrayValue[i];

         int partId = part.get("id",0).asInt();
         string name = part.get("name","").asString();
         //string material = part.get("material","").asString();

         QString displayText = QString("%1").arg(name.c_str());

         ui->propMaterialComboBox->addItem(displayText, partId);
     }
}

void PartSelectionDialog::on_motorSelectComboBox_currentIndexChanged(int index)
{
    if ( m_ignoreComboboxSelections )
        return;

    int partId = ui->motorSelectComboBox->itemData( index ).toInt();

    g_log.log(LL_DEBUG, QString("Selected motor %1").arg(partId));

    g_selectedMotorId = partId;
}

void PartSelectionDialog::on_propSelectComboBox_currentIndexChanged(int index)
{
    if ( m_ignoreComboboxSelections )
        return;

    int partId = ui->propSelectComboBox->itemData( index ).toInt();

    g_log.log(LL_DEBUG, QString("Selected prop %1").arg(partId));

    g_selectedPropId = partId;
}

void PartSelectionDialog::on_escSelectComboBox_currentIndexChanged(int index)
{
    if ( m_ignoreComboboxSelections )
        return;

    int partId = ui->escSelectComboBox->itemData( index ).toInt();

    g_log.log(LL_DEBUG, QString("Selected ESC %1").arg(partId));

    g_selectedEscId = partId;
}
