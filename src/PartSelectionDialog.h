#ifndef PARTSELECTIONDIALOG_H
#define PARTSELECTIONDIALOG_H

#include <QDialog>
#include <QHttp>
#include <QHash>

namespace Ui {
class PartSelectionDialog;
}

class PartSelectionDialog : public QDialog
{
    Q_OBJECT

    enum partCategories {
        PC_MOTOR,
        PC_ESC,
        PC_PROP,
        PC_MCU,
        PC_MATERIAL,

        PC_MAX
    };

    bool m_requestInProgress;

    bool m_ignoreComboboxSelections;

    QStringList m_partNames;
    QStringList m_settingsKeys;

    QHash<int, QString> m_urls;
    int m_currentPartCategory;
    QHttp* m_http;

public:
    explicit PartSelectionDialog(QWidget *parent = 0);
    ~PartSelectionDialog();

private slots:
    void on_partsDownloadButton_clicked();

    void on_motorSelectComboBox_currentIndexChanged(int index);

    void on_propSelectComboBox_currentIndexChanged(int index);

    void on_escSelectComboBox_currentIndexChanged(int index);

private:
    Ui::PartSelectionDialog *ui;

    void startRequest();
    void updateMotorCombobox();
    void updateESCCombobox();
    void updatePropCombobox();
    void updateMcuCombobox();
    void updateMaterialCombobox();

public slots:
    void onRequestCompleted(bool error);
    void accept();
    void reject();
};

#endif // PARTSELECTIONDIALOG_H
