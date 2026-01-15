#include "Content/ParticlesEditor/particles_view.h"

ParticlesView::ParticlesView(const QVariantMap& params, QWidget* parent)
	: QOpenGLWidget(parent) {
}

ParticlesView::~ParticlesView() {
}

void ParticlesView::initializeGL() {
	initializeOpenGLFunctions();


}

void ParticlesView::resizeGL(int w, int h) {
	_viewWidth = w;
	_viewHeight = h;
}

void ParticlesView::paintGL() {
	glViewport(0, 0, _viewWidth, _viewHeight);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);





	update();
}

