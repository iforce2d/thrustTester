#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <vector>
#include <QDialog>
#include <QNetworkReply>
#include "TestTask.h"

namespace Ui {
class ExportDialog;
}

class ExportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExportDialog(QWidget *parent = 0);
    ~ExportDialog();

    QString getSaveFilename();

private slots:
    void on_saveFileButton_clicked();

    void on_uploadButton_clicked();

private:
    Ui::ExportDialog *ui;

    std::vector<TestTask*> m_tasks;

    bool m_requestInProgress;

public slots:
    void serviceRequestFinished(QNetworkReply *reply);
    void accept();
    void reject();
};

#endif // EXPORTDIALOG_H
