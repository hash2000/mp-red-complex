#include "Content/ShaderEditor/shader_view.h"
#include <QDateTime>

static const char* DEFAULT_VERT = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
out vec2 TexCoord;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

static const char* DEFAULT_FRAG = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;
uniform float uTime;
uniform vec2 uResolution;
void main() {
    vec2 uv = TexCoord * 2.0 - 1.0;
    float r = length(uv) + sin(uTime) * 0.1;
    FragColor = vec4(vec3(r), 1.0);
}
)";

ShaderView::ShaderView(const QVariantMap& params, QWidget* parent)
	: QOpenGLWidget(parent) {
	_vertexCode = DEFAULT_VERT;
	_fragmentCode = DEFAULT_FRAG;
	_renderTimer = new QTimer(this);
}

ShaderView::~ShaderView() {
}

void ShaderView::initializeGL() {
	initializeOpenGLFunctions();
	setupQuadGeometry();
	compileAndLink();
	setupViewport();

	_timer.start();
	connect(_renderTimer, &QTimer::timeout, this, [this]() {
		update();
		});
	_renderTimer->start(16);
}

void ShaderView::resizeGL(int w, int h) {
	setupViewport();
}

void ShaderView::setupViewport() {
	glViewport(0, 0, width(), height());
	_proj.setToIdentity();
	_proj.ortho(-1, 1, -1, 1, -1, 1);
}

void ShaderView::paintGL() {
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (!_program.isLinked()) {
		return;
	}

	_program.bind();

	// Устанавливаем стандартные uniform'ы
	float uTime = _timer.elapsed() / 1000.0f;
	float uWind = sin(uTime) * 0.5;
	_program.setUniformValue("uTime", uTime);
	_program.setUniformValue("uResolution", QSizeF(width(), height()));
	_program.setUniformValue("uWind", uWind);

	// Рисуем quad
	_vbo.bind();

	_program.enableAttributeArray(0);
	_program.enableAttributeArray(1);
	_program.setAttributeBuffer(0, GL_FLOAT, 0, 2, 4 * sizeof(GLfloat));
	_program.setAttributeBuffer(1, GL_FLOAT, 2 * sizeof(GLfloat), 2, 4 * sizeof(GLfloat));

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	_vbo.release();

	_program.release();
}

QString ShaderView::lastError() const {
	return _lastError;
}

void ShaderView::setVertexShaderCode(const QString& code) {
	_vertexCode = code;
	compileAndLink();
	update();
}

void ShaderView::setFragmentShaderCode(const QString& code) {
	_fragmentCode = code;
	compileAndLink();
	update();
}

QString ShaderView::vertexCode() const {
	return _vertexCode;
}

QString ShaderView::fragmentCode() const {
	return _fragmentCode;
}


bool ShaderView::compileAndLink() {
	_lastError.clear();

	_program.removeAllShaders();

	if (!_program.addShaderFromSourceCode(QOpenGLShader::Vertex, _vertexCode)) {
		_lastError = _program.log();
		emit shaderReloaded(false);
		return false;
	}

	if (!_program.addShaderFromSourceCode(QOpenGLShader::Fragment, _fragmentCode)) {
		_lastError = _program.log();
		emit shaderReloaded(false);
		return false;
	}

	if (!_program.link()) {
		_lastError = _program.log();
		emit shaderReloaded(false);
		return false;
	}

	emit shaderReloaded(true);
	return true;
}

void ShaderView::setupQuadGeometry() {
	// Fullscreen quad: [-1,-1] to [1,1]
	static const GLfloat vertices[] = {
			-1.0f, -1.0f, 0.0f, 0.0f,
			 1.0f, -1.0f, 1.0f, 0.0f,
			 1.0f,  1.0f, 1.0f, 1.0f,
			-1.0f,  1.0f, 0.0f, 1.0f
	};
	_vbo.create();
	_vbo.bind();
	_vbo.allocate(vertices, sizeof(vertices));
}

void ShaderView::autoRender(bool set) {
	if (set) {
		_renderTimer->start(16);
	}
	else {
		_renderTimer->stop();
	}
}
