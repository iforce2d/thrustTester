#ifndef Canvas_H
#define Canvas_H

#include "GLCanvas.h"
#include "util.h"

enum _CanvasType {
    DCT_EDITOR,
    DCT_PLAYER
};

enum _mouseButtonState {
    MS_LEFT     = 0x1,
    MS_RIGHT    = 0x2,
    MS_MIDDLE   = 0x4
};

struct _mouseState {
    int buttonState;
    Vector2i screenPosition;
    Vector2i leftButtonDownScreenPosition;
    Vector2i rightButtonDownScreenPosition;
};

class Canvas : public GLCanvas
{
    Q_OBJECT
protected:
    _CanvasType m_canvasType;
    float m_minZoom;
    float m_maxZoom;

    _mouseState m_mouseState;
    _keyState m_keyState;

public :

    explicit Canvas(QWidget* parent = 0, const QGLWidget * shareWidget = 0);
    virtual ~Canvas();

    void setCanvasType(_CanvasType type);

    static void setupGLFormat();

    QSize sizeHint() const { return QSize(480,360); }
    void OnInit();

    const _mouseState& getMouseState() { return m_mouseState; }
    const _keyState& getKeyState() { return m_keyState; }

    inline bool isKeyDown(int k) { return m_keyState.isKeyDown(k); }

    void focusOutEvent( QFocusEvent * event );

    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void mouseMoveEvent(QMouseEvent * event);
    void mousePressEvent( QMouseEvent * event );
    void mouseReleaseEvent( QMouseEvent * event );
    void wheelEvent( QWheelEvent * event );

    virtual void keyDown(Qt::Key key);
    virtual void keyUp(Qt::Key key);

    virtual void mouseDown(QMouseEvent* mouseEvent);
    virtual void mouseUp(QMouseEvent* mouseEvent);
    void clearMouseButtonState();

    virtual void mouseMove(QMouseEvent* mouseEvent);
    virtual void leaveEvent(QEvent* event);
    void updateMouseWorldPosition();

    void drawDragZoomRectangle(QColor baseColor);
    void startDragZoom();
    void endDragZoom();
    float pixelToWorld_dimension(float pixelDimension);
    float worldToPixel_dimension(float worldDimension);

    void prepareScene();
    void drawTextAtViewPosition(QPainter *painter, const QString &text, Vector2i &pos, QColor &backgroundColor , QColor &foregroundColor);

public:

public slots:

};

#endif // Canvas_H
