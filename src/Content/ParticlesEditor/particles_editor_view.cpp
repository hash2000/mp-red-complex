#include "Content/ParticlesEditor/particles_editor_view.h"

ParticlesEditorView::ParticlesEditorView(const QVariantMap& params, QWidget* parent)
	: QOpenGLWidget(parent) {
}

ParticlesEditorView::~ParticlesEditorView() {
}

void ParticlesEditorView::initializeGL() {
	initializeOpenGLFunctions();


}

void ParticlesEditorView::resizeGL(int w, int h) {
	_viewWidth = w;
	_viewHeight = h;
}

void ParticlesEditorView::paintGL() {
	glViewport(0, 0, _viewWidth, _viewHeight);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);





	update();
}

