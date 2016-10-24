#ifndef TESTSETUPDIALOG_H
#define TESTSETUPDIALOG_H

#include <QDialog>

namespace Ui {
class TestSetupDialog;
}

class TestSetupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TestSetupDialog(QWidget *parent = 0);
    ~TestSetupDialog();

    QString getScriptString();

private slots:
    void on_scriptTextEdit_textChanged();

private:
    Ui::TestSetupDialog *ui;
};

#endif // TESTSETUPDIALOG_H
