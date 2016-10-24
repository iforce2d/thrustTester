#include <QMessageBox>
#include <QCloseEvent>
#include <QKeyEvent>
#include "CalibrationDialog.h"
#include "ui_calibrationdialog.h"
#include "mainwindow.h"

CalibrationDialog::CalibrationDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CalibrationDialog)
{
    ui->setupUi(this);
}

CalibrationDialog::~CalibrationDialog()
{
    delete ui;
}

void CalibrationDialog::closeEvent(QCloseEvent *e)
{
    e->ignore();
}

void CalibrationDialog::keyPressEvent(QKeyEvent *e)
{
    if( e->key() != Qt::Key_Escape )
        QDialog::keyPressEvent(e);
    else
        e->ignore();
}

void CalibrationDialog::on_measureZeroButton_clicked()
{
    g_mainWindow->setThrustCalibrationZero();
}

void CalibrationDialog::on_measureKnownMassButton_clicked()
{
    bool ok = false;
    float mass = ui->knownMassEdit->text().toFloat(&ok);

    QString problemStr = "";
    if ( !ok ) {
        problemStr = "Please enter a non-zero positive number";
    }
    else if ( mass <= 0 ) {
        problemStr = "Please enter a non-zero positive number";
    }

    if ( problemStr != "" ) {
        QMessageBox msgBox(g_mainWindow);
        msgBox.setWindowTitle( tr("Invalid value") );
        msgBox.setText( problemStr );
        msgBox.addButton( tr("OK"), QMessageBox::AcceptRole );
        msgBox.setIcon( QMessageBox::Warning );
        msgBox.exec();
        return;
    }

    g_mainWindow->setThrustCalibrationOne( mass );
}

void CalibrationDialog::on_buttonBox_accepted()
{
    g_mainWindow->finishCalibration();
}

void CalibrationDialog::on_measureVoltageZeroButton_clicked()
{
    g_mainWindow->setVoltageCalibrationZero();
}

void CalibrationDialog::on_measureVoltageBatteryButton_clicked()
{
    bool ok = false;
    float voltage = ui->knownVoltageEdit->text().toFloat(&ok);

    QString problemStr = "";
    if ( !ok ) {
        problemStr = "Please enter a non-zero positive number";
    }
    else if ( voltage <= 0 ) {
        problemStr = "Please enter a non-zero positive number";
    }

    if ( problemStr != "" ) {
        QMessageBox msgBox(g_mainWindow);
        msgBox.setWindowTitle( tr("Invalid value") );
        msgBox.setText( problemStr );
        msgBox.addButton( tr("OK"), QMessageBox::AcceptRole );
        msgBox.setIcon( QMessageBox::Warning );
        msgBox.exec();
        return;
    }

    g_mainWindow->setVoltageCalibrationOne( voltage );
}

void CalibrationDialog::on_measureCurrentZeroButton_clicked()
{
    g_mainWindow->setCurrentCalibrationZero();
}

void CalibrationDialog::on_measureKnownCurrentButton_clicked()
{
    bool ok = false;
    float current = ui->knownCurrentEdit->text().toFloat(&ok);

    QString problemStr = "";
    if ( !ok ) {
        problemStr = "Please enter a non-zero positive number";
    }
    else if ( current <= 0 ) {
        problemStr = "Please enter a non-zero positive number";
    }

    if ( problemStr != "" ) {
        QMessageBox msgBox(g_mainWindow);
        msgBox.setWindowTitle( tr("Invalid value") );
        msgBox.setText( problemStr );
        msgBox.addButton( tr("OK"), QMessageBox::AcceptRole );
        msgBox.setIcon( QMessageBox::Warning );
        msgBox.exec();
        return;
    }

    g_mainWindow->setCurrentCalibrationOne( current );
}
