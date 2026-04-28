#pragma once
#include <QOpenGLFunctions>

#ifdef QT_DEBUG
#define GL_CHECK_ERRORS() { \
    auto f = QOpenGLContext::currentContext()->functions(); \
    GLenum err; \
    while ((err = f->glGetError()) != GL_NO_ERROR) { \
        qWarning() << "OpenGL error:" << err << "at" << __FILE__ << ":" << __LINE__; \
    } \
}
#else
#define GL_CHECK_ERRORS()
#endif
