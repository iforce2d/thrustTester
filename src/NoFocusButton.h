#ifndef NOFOCUSBUTTON_H
#define NOFOCUSBUTTON_H

#include <QPushButton>
#include <QKeyEvent>

class NoFocusButton : public QPushButton
{
    Q_OBJECT
public:
    explicit NoFocusButton(QWidget *parent = 0);

    void keyPressEvent(QKeyEvent *event);

signals:

public slots:

};

#endif // NOFOCUSBUTTON_H
