
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <QResizeEvent>
#include "GLCanvas.h"
#include "Log.h"

// Platform-specific headers
#ifdef Q_WS_X11
    #include <Qt/qx11info_x11.h>
    #include <X11/Xlib.h>
#endif

QGLWidget* g_shareWidget = NULL;

GLCanvas::GLCanvas(const QGLFormat &format, QWidget *parent)
    : QGLWidget(format, parent, g_shareWidget), m_initialized (false)
{
    m_initialized = false;

    // Setup some states to allow direct rendering into the widget
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_ContentsPropagated);

    // Set strong focus to enable keyboard events to be received
    setFocusPolicy(Qt::StrongFocus);

    setAutoFillBackground(false);
    setAutoBufferSwap(false);

    m_renderer = NULL;
}


////////////////////////////////////////////////////////////
/// Destructor
////////////////////////////////////////////////////////////
GLCanvas::~GLCanvas()
{
    // Nothing to do...
}


////////////////////////////////////////////////////////////
/// Notification for the derived class that moment is good
/// for doing initializations
////////////////////////////////////////////////////////////
void GLCanvas::OnInit()
{
    // Nothing to do by default...
}


////////////////////////////////////////////////////////////
/// Called when the widget is shown ;
/// we use it to initialize our SFML window
////////////////////////////////////////////////////////////
void GLCanvas::showEvent(QShowEvent*)
{
    if (!m_initialized)
    {
        // Under X11, we need to flush the commands sent to the server to ensure that
        // SFML will get an updated view of the windows
        #ifdef Q_WS_X11
            XFlush(QX11Info::display());
        #endif

        // Create the SFML window with the widget handle
        //Create(winId());

        // Let the derived class do its specific stuff
        OnInit();

        m_initialized = true;
    }
}

////////////////////////////////////////////////////////////
/// Called when the widget needs to be painted ;
/// we use it to display a new frame
////////////////////////////////////////////////////////////
void GLCanvas::paintEvent(QPaintEvent*)
{
    // Let the derived class do its specific stuff    
    if ( m_renderer )
        m_renderer->render();
    this->swapBuffers();
}

void GLCanvas::focusOutEvent(QFocusEvent * /*event*/)
{
    if ( m_renderer )
        m_renderer->onFocusOut();
}

void GLCanvas::resizeGL(int w, int h)
{
    glViewport(0, 0, (GLint)w, (GLint)h);
}

