
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <QMouseEvent>
#include <QStatusBar>

#include <stdio.h>

#include "Log.h"
#include "Canvas.h"
#include "vec2.h"
#include "mainwindow.h"

QGLFormat g_glFormat;

Canvas::Canvas(QWidget *parent, const QGLWidget */*shareWidget*/) : GLCanvas(g_glFormat, parent)
{
    m_canvasType = DCT_EDITOR;
    m_minZoom = 0.01f;
    m_maxZoom = 100000;
}

Canvas::~Canvas()
{
}

void Canvas::setCanvasType(_CanvasType type)
{
    m_canvasType = type;
}

void Canvas::setupGLFormat()
{
    //g_glFormat.setDepthBufferSize();
    g_glFormat.setSamples(4);
}

void Canvas::OnInit()
{
    setMouseTracking(true);

    m_mouseState.buttonState = 0;
    m_mouseState.screenPosition = Vector2i(-1,-1);

    m_keyState.init();

    makeCurrent();
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POINT_SMOOTH);

}

int stepCount = 0;

void Canvas::keyDown(Qt::Key key)
{
    m_keyState.keyDown(key);
}

#ifdef USE_PLAYER_CONTROLS
float fovy = 10;
#endif

void Canvas::keyUp(Qt::Key key)
{
    m_keyState.keyUp(key);

#ifdef USE_PLAYER_CONTROLS
    if ( key == Qt::Key_Up ) {
        fovy *= 1.05;
        g_log.log(LL_DEBUG, QString::fromUtf8("fovy: %1").arg(fovy));
    }
    else if ( key == Qt::Key_Down ) {
        fovy /= 1.05;
        g_log.log(LL_DEBUG, QString::fromUtf8("fovy: %1").arg(fovy));
    }
#endif
}

void Canvas::mouseDown(QMouseEvent *mouseEvent)
{
    m_mouseState.screenPosition.x = mouseEvent->x();
    m_mouseState.screenPosition.y = height() - mouseEvent->y();

    if ( mouseEvent->button() == Qt::LeftButton ) {
        m_mouseState.buttonState |= MS_LEFT;
        m_mouseState.leftButtonDownScreenPosition = m_mouseState.screenPosition;
    }
    else if ( mouseEvent->button() == Qt::RightButton ) {
        m_mouseState.buttonState |= MS_RIGHT;
        m_mouseState.rightButtonDownScreenPosition = m_mouseState.screenPosition;

    }
}

void Canvas::mouseUp(QMouseEvent *mouseEvent)
{
    m_mouseState.screenPosition.x = mouseEvent->x();
    m_mouseState.screenPosition.y = height() - mouseEvent->y();

    if ( mouseEvent->button() == Qt::LeftButton )
        m_mouseState.buttonState &= ~MS_LEFT;
    else if ( mouseEvent->button() == Qt::RightButton ) {
        m_mouseState.buttonState &= ~MS_RIGHT;
    }
}

void Canvas::clearMouseButtonState()
{
    m_mouseState.buttonState = 0;
}

void Canvas::mouseMove(QMouseEvent* mouseEvent)
{
    if ( mouseEvent->modifiers() & Qt::ShiftModifier )
        m_keyState.keyDown(Qt::Key_Shift);
    else
        m_keyState.keyUp(Qt::Key_Shift);
    if ( mouseEvent->modifiers() & Qt::ControlModifier )
        m_keyState.keyDown(Qt::Key_Control);
    else
        m_keyState.keyUp(Qt::Key_Control);

    m_mouseState.screenPosition.x = mouseEvent->x();
    m_mouseState.screenPosition.y = height() - mouseEvent->y();

}

void Canvas::leaveEvent(QEvent* /*event*/)
{
    m_mouseState.screenPosition = Vector2i(-1,-1);
}

void Canvas::focusOutEvent( QFocusEvent* /*event*/ )
{
    m_keyState.init();
}

void Canvas::keyPressEvent(QKeyEvent *event)
{
    if ( ! event->isAutoRepeat() )
        keyDown((Qt::Key)event->key());

    if ( event->key() == Qt::Key_Escape )
        g_mainWindow->panic();
    else if ( event->key() == Qt::Key_Space )
        g_mainWindow->toggleSampling();
}

void Canvas::keyReleaseEvent(QKeyEvent *event)
{
    if ( ! event->isAutoRepeat() )
        keyUp((Qt::Key)event->key());
}

void Canvas::mouseMoveEvent(QMouseEvent * event)
{
    mouseMove(event);
}

void Canvas::mousePressEvent( QMouseEvent * event )
{
    if ( event->button() == Qt::NoButton )
        return;

    mouseDown(event);
}

void Canvas::mouseReleaseEvent( QMouseEvent * event )
{
    if ( event->button() == Qt::NoButton )
        return;

    mouseUp(event);
}

void Canvas::wheelEvent( QWheelEvent * /*event*/ )
{
}

void Canvas::drawTextAtViewPosition(QPainter* painter, const QString& text, Vector2i& screenPos, QColor& backgroundColor, QColor& foregroundColor)
{
    QRect rect;
    rect.setRect(screenPos.x, height()-screenPos.y, 1000, 1000);

    QRect boundingRect = painter->fontMetrics().boundingRect(rect, Qt::AlignLeft, text);
    int dy = (int)(painter->fontMetrics().height() / 5.0);
    int dx = 2 * dy;
    boundingRect.adjust(0,0,2*dx,2*dy);
    painter->fillRect(boundingRect, backgroundColor);

    QColor outlineColor = foregroundColor;
    outlineColor.setAlpha(outlineColor.alpha()/2);
    painter->setPen(outlineColor);
    painter->drawRect(boundingRect);

    rect.adjust(dx,dy,0,0);
    painter->setPen(foregroundColor);
    painter->drawText( rect, Qt::AlignLeft, text );
}




