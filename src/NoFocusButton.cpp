#include "NoFocusButton.h"
#include "mainwindow.h"

NoFocusButton::NoFocusButton(QWidget *parent) :
    QPushButton(parent)
{
}

void NoFocusButton::keyPressEvent(QKeyEvent *event)
{
    if ( event->key() == Qt::Key_Escape )
        g_mainWindow->panic();

    else if ( event->key() == Qt::Key_Space )
        g_mainWindow->toggleSampling();
}
