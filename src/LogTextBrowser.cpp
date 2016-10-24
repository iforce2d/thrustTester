#include <QScrollBar>
#include "LogTextBrowser.h"

LogTextBrowser::LogTextBrowser(QWidget *parent) :
    QTextBrowser(parent)
{
    m_atBottom = false;
    connect( this, SIGNAL( textChanged() ), this, SLOT( scrollToBottom() ) );
    connect( this->verticalScrollBar(), SIGNAL( valueChanged( int ) ), this, SLOT( scrolledTo( int ) ) );
}

void LogTextBrowser::scrollToBottom( void )
{
    if( m_atBottom )
    {
        this->verticalScrollBar()->setValue( this->verticalScrollBar()->maximum() );
    }
}

void LogTextBrowser::scrolledTo( int val )
{
    m_atBottom = val == this->verticalScrollBar()->maximum();
}
