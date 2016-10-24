#ifndef CALIBRATIONDIALOG_H
#define CALIBRATIONDIALOG_H

#include <QDialog>

namespace Ui {
class CalibrationDialog;
}

class CalibrationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CalibrationDialog(QWidget *parent = 0);
    ~CalibrationDialog();

    void closeEvent(QCloseEvent * e);
    void keyPressEvent(QKeyEvent *e);

private slots:
    void on_measureZeroButton_clicked();

    void on_measureKnownMassButton_clicked();

    void on_buttonBox_accepted();

    void on_measureVoltageZeroButton_clicked();

    void on_measureVoltageBatteryButton_clicked();

    void on_measureCurrentZeroButton_clicked();

    void on_measureKnownCurrentButton_clicked();

private:
    Ui::CalibrationDialog *ui;
};

#endif // CALIBRATIONDIALOG_H
