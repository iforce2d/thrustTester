#include <QFileDialog>
#include <QMessageBox>
#include <istream>
#include <fstream>
#include "ExportDialog.h"
#include "ui_exportdialog.h"
#include "mainwindow.h"
#include "Log.h"
#include "Settings.h"

ExportDialog::ExportDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ExportDialog)
{
    ui->setupUi(this);

    g_mainWindow->tasks(m_tasks);

    int resultCount = 0;

    for (int i = 0; i < (int)m_tasks.size(); i++) {

        TestTask* task = m_tasks[i];
        for (int k = 0; k < task->reportCount(); k++) {
            QStringList list = task->resultAsStringList(k);

            ui->textBrowser->append("-------------------");

            foreach(QString s, list) {
                ui->textBrowser->append(s);
            }

            resultCount++;
        }
    }

    if ( resultCount < 1 ){
        ui->uploadButton->setEnabled(false);
        ui->saveFileButton->setEnabled(false);
    }

    m_requestInProgress = false;
}

ExportDialog::~ExportDialog()
{
    delete ui;
}

QString ExportDialog::getSaveFilename()
{
    QString dir;

    if ( g_lastUsedSaveFileDirectory.length() != 0 )
        dir = g_lastUsedSaveFileDirectory;
    else if ( g_lastUsedOpenFileDirectory.length() != 0 )
        dir = g_lastUsedOpenFileDirectory;
    else
        dir = QApplication::applicationDirPath();
    dir += QString::fromUtf8("/") + QString::fromUtf8("thrustTestReport.json");

    QString filename = QFileDialog::getSaveFileName(g_mainWindow, tr("Save results"),
                                dir,
                                tr("JSON files (*.json);;All files(*)"));
    return filename;
}

void ExportDialog::on_saveFileButton_clicked()
{

    /*Json::FastWriter writer;
    QString str = QString::fromUtf8( writer.write(arrayValue).c_str() );

    g_log.log(LL_DEBUG, str);*/

    QString filename = getSaveFilename();

    if ( filename.length() == 0 ) {
        g_log.log(LL_ERROR, "Couldn't figure out which directory to save to");
        return;
    }

    QFileInfo fi(filename);
    g_lastUsedSaveFileDirectory = fi.path();
    writeSettings();

    string utf8String = convertQStringToUTF8String( filename );

    std::ofstream ofs;
    ofs.open(utf8String.c_str(), std::ios::out);
    if (!ofs) {
        QString errorMsg = QObject::tr("Could not open file '%1' for writing.").arg(filename);
        g_log.log(LL_ERROR, errorMsg);
        QMessageBox msgBox(QMessageBox::Critical, QString::fromUtf8("File problem"), errorMsg, QMessageBox::Ok, g_mainWindow );
        msgBox.exec();
        return;
    }

    Json::Value arrayValue(Json::arrayValue);

    for (int i = 0; i < (int)m_tasks.size(); i++) {
        TestTask* task = m_tasks[i];
        task->allResultsAsJSON(arrayValue);
    }

    Json::StyledStreamWriter writer("  ");
    writer.write( ofs, arrayValue );

    ofs.close();
}

void ExportDialog::on_uploadButton_clicked()
{
    Json::Value arrayValue(Json::arrayValue);

    for (int i = 0; i < (int)m_tasks.size(); i++) {
        TestTask* task = m_tasks[i];
        task->allResultsAsJSON(arrayValue);
    }
    Json::FastWriter writer;
    QString str = QString::fromUtf8( writer.write(arrayValue).c_str() );

    /*QUrl url;
    url.setPath( "/tts/uploadReport.php" );

    url.addQueryItem( "runs" , str);

    m_http->setHost( "www.iforce2d.net" );
    m_http->post(url.toString(), );*/

    QString bound = QString::fromUtf8("margin");

    QByteArray data;
    data.append(QString(QString::fromUtf8("--") + bound + QString::fromUtf8("\r\n")).toLatin1());
    data.append(QString::fromUtf8("Content-Disposition: form-data; name=\"action\"\r\n\r\n").toLatin1());
    data.append(QString::fromUtf8("uploadReport.php\r\n").toLatin1());
    data.append((QString::fromUtf8("--") + bound + QString::fromUtf8("\r\n")).toLatin1());

    data.append(QString::fromUtf8("Content-Disposition: ftoLatin1orm-data; name=\"report\"\r\n\r\n").toLatin1());
    data.append(QString::fromUtf8("%1\r\n").arg(str).toLatin1());
    data.append((QString::fromUtf8("--") + bound + QString::fromUtf8("--\r\n")).toLatin1());  //closing boundary according to rfc 1867

    QUrl serviceUrl = QUrl( QString::fromUtf8("http://www.iforce2d.net/tts/uploadReport.php") );
    QNetworkRequest request( serviceUrl );
    request.setRawHeader(QString::fromUtf8("Content-Type").toLatin1(),(QString::fromUtf8("multipart/form-data; boundary=") + bound).toLatin1());
    request.setRawHeader(QString::fromUtf8("Content-Length").toLatin1(), QString::number(data.length()).toLatin1());

    QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
    connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(serviceRequestFinished(QNetworkReply*)));
    networkManager->post(request, data );

    m_requestInProgress = true;
    ui->saveFileButton->setEnabled(false);
    ui->uploadButton->setEnabled(false);

    g_log.log(LL_DEBUG, "Uploading report...");
}

void ExportDialog::serviceRequestFinished(QNetworkReply *reply)
{
    g_log.log(LL_DEBUG, QString::fromUtf8("Finished feedback submission, error:%1").arg(reply->error()));

    QString errorMsg = tr("<p>There was a problem sending the message. The server may be temporarily unavailable, in which case you could try "
                          "again later. If the problem persists, please send an email to iforce2d@gmail.com.<br><br>Sorry for the inconvenience.</p>"
                       );
    QString errorDlgTitle = tr("Error contacting server");

    QMessageBox msgBox(this);
    msgBox.setStandardButtons( QMessageBox::Ok );
    msgBox.setWindowTitle( errorDlgTitle );
    msgBox.setIcon( QMessageBox::NoIcon );

    m_requestInProgress = false;
    ui->saveFileButton->setEnabled(true);
    ui->uploadButton->setEnabled(true);

    if ( reply->error() ) {
        msgBox.setText( errorMsg );
        msgBox.exec();
        return;
    }

    QString jsonStr = QString::fromUtf8( reply->readAll() );
    g_log.log(LL_DEBUG, QString::fromUtf8("  response: %1").arg(jsonStr));

    Json::Value responseValue;
    Json::Reader reader;
    if ( ! reader.parse(jsonStr.toStdString(), responseValue) )
    {
        g_log.log( LL_DEBUG, QString::fromUtf8("Upload : failed to parse JSON : %1").arg( QString::fromUtf8(reader.getFormatedErrorMessages().c_str()) ) );
        msgBox.setText( errorMsg );
        msgBox.exec();
        return;
    }

    if ( !responseValue.isObject() || responseValue["success"].isNull() ) {
        g_log.log( LL_DEBUG, QString::fromUtf8("Upload : failed to find success value in JSON") );
        msgBox.setText( errorMsg );
        msgBox.exec();
        return;
    }

    if ( responseValue["success"].asString() != "yes" ) {
        g_log.log( LL_DEBUG, QString::fromUtf8("Feedback : report send failed") );
        msgBox.setText( errorMsg );
        msgBox.exec();
        return;
    }

    if ( !responseValue["rejected"].isNull() ) {
        g_log.log( LL_DEBUG, QString::fromUtf8("Upload : report rejected") );
        msgBox.setWindowTitle( tr("Message problem") );
        msgBox.setText( QString::fromUtf8( responseValue["rejected"].asString().c_str() ) );
        msgBox.exec();
        return;
    }

    g_log.log( LL_DEBUG, QString::fromUtf8("Upload : report sent ok") );
    msgBox.setWindowTitle( tr("Upload successful") );
    msgBox.setText( tr("<p>Upload has been received - thankyou!</p>" ) );
    msgBox.exec();
}

void ExportDialog::accept()
{
    if ( m_requestInProgress )
        return;
    QDialog::accept();
}

void ExportDialog::reject()
{
    if ( m_requestInProgress )
        return;
    QDialog::reject();
}

