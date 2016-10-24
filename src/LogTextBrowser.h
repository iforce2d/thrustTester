#ifndef LOGTEXTBROWSER_H
#define LOGTEXTBROWSER_H

#include <QTextBrowser>

class LogTextBrowser : public QTextBrowser
{
    Q_OBJECT

    bool m_atBottom;
public:
    explicit LogTextBrowser(QWidget *parent = 0);

    QSize sizeHint() const { return QSize(320,64); }

signals:

public slots:
    void scrollToBottom( void );
    void scrolledTo( int val );

};

#endif // LOGTEXTBROWSER_H
