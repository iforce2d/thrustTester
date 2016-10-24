
#ifndef GLCANVAS_HPP
#define GLCANVAS_HPP

#include <QGLWidget>

extern QGLWidget* g_shareWidget;

class GLRenderer {
public:
    virtual void render() = 0;
    virtual void onFocusOut() = 0;
};

////////////////////////////////////////////////////////////
/// GLCanvas allows to run SFML in a Qt control
////////////////////////////////////////////////////////////
class GLCanvas : public QGLWidget//, public sf::RenderWindow
{
public :

    explicit GLCanvas(const QGLFormat & format, QWidget* parent = 0);

    ////////////////////////////////////////////////////////////
    /// Destructor
    ///
    ////////////////////////////////////////////////////////////
    virtual ~GLCanvas();

protected:
    GLRenderer* m_renderer;
public:
    void setRenderer(GLRenderer* renderer) { m_renderer = renderer; }

private :

    ////////////////////////////////////////////////////////////
    /// Notification for the derived class that moment is good
    /// for doing initializations
    ///
    ////////////////////////////////////////////////////////////
    virtual void OnInit();

    ////////////////////////////////////////////////////////////
    /// Called when the widget is shown ;
    /// we use it to initialize our SFML window
    ///
    ////////////////////////////////////////////////////////////
    virtual void showEvent(QShowEvent*);

    ////////////////////////////////////////////////////////////
    /// Called when the widget needs to be painted ;
    /// we use it to display a new frame
    ///
    ////////////////////////////////////////////////////////////
    virtual void paintEvent(QPaintEvent*);

    virtual void focusOutEvent(QFocusEvent *event);

    void resizeGL(int w, int h);

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    bool m_initialized; ///< Tell whether the SFML window has been initialized or not

public:
};


#endif // GLCANVAS_HPP
