#include "TestSetupDialog.h"
#include "ui_testsetupdialog.h"
#include "Settings.h"
#include "scriptParsing.h"

TestSetupDialog::TestSetupDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TestSetupDialog)
{
    ui->setupUi(this);

    ui->scriptTextEdit->setPlainText( g_testScriptString );
}

TestSetupDialog::~TestSetupDialog()
{
    delete ui;
}

QString TestSetupDialog::getScriptString()
{
    return ui->scriptTextEdit->toPlainText();
}

void TestSetupDialog::on_scriptTextEdit_textChanged()
{
    QStringList lines = parseScriptIntoLines( ui->scriptTextEdit->toPlainText() );

    if ( lines.empty() ) {
        ui->parsedResult->setText( "<nothing>" );
    }
    else {

        std::vector<TestTask*> tasks;
        int numTasks = parseLinesIntoTasks(lines, tasks);

        QString result = "";

        for (int i = 0; i < numTasks; i++) {
            TestTask* task = tasks[i];
            result += task->shortDescription() + "\n";
            delete task;
        }

        ui->parsedResult->setText( result );
    }
}
