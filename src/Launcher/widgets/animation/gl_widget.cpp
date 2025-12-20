#include "Launcher/widgets/animation/gl_widget.h"

FrmGlWidget::FrmGlWidget(std::unique_ptr<OpenGLFramesRenderer> backend, QWidget *parent)
  : QOpenGLWidget(parent)
	, _backend(std::move(backend)) {
	}

void FrmGlWidget::initializeGL() {
	_backend->initialize();
}

void FrmGlWidget::resizeGL(int w, int h) {
	_backend->resize(w, h);
}

void FrmGlWidget::paintGL() {
	_backend->render();
}

void FrmGlWidget::setFrames(const std::vector<Proto::Frame> &frames, const Proto::Pallete &pallete) {
  _backend->uploadFrames(frames, pallete);
	_backend->setCurrentFrame(0);
  update(); // → paintGL()
}
