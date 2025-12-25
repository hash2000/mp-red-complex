#include "Launcher/widgets/animation/gl_widget.h"
#include <QOpenGLBuffer>
#include <QOpenGLContext>


static const char* vertexShaderSource = R"(
attribute vec2 aPos;
varying vec2 vTexCoord;
uniform mat4 uModel;
uniform mat4 uProjection;
void main() {
    vTexCoord = aPos * 0.5 + 0.5;
    vTexCoord.y = 1.0 - vTexCoord.y;
    gl_Position = uProjection * uModel * vec4(aPos, 0.0, 1.0);
}
)";

static const char* fragmentShaderSource = R"(
varying vec2 vTexCoord;
uniform sampler2D uAtlas;
uniform vec4 uUVRect; // x=u0, y=v0, z=u1, w=v1
void main() {
    vec2 uv = mix(uUVRect.xy, uUVRect.zw, vTexCoord);
    gl_FragColor = texture2D(uAtlas, uv);
}
)";


FrmGlWidget::FrmGlWidget(QWidget *parent)
  : QOpenGLWidget(parent) {
	setObjectName("FrmGlWidget");
}

FrmGlWidget::~FrmGlWidget() {
	cleanup();
}

void FrmGlWidget::initializeGL() {
	if (_initialized) {
		return;
	}

	initializeOpenGLFunctions();
	glClearColor(0, 0, 0, 0);

	// --- Shaders ---
	_program.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
	_program.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);

	_program.link();

	// --- VAO + VBO (fullscreen quad) ---
	if (!_vao.create()) {
		qWarning() << "Failed to create VAO";
		return;
	}

	QOpenGLVertexArrayObject::Binder vaoBinder(&_vao);

	_vbo.create();
	_vbo.bind();
	_vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);

	static const GLfloat verts[] = {
		-1.0f, -1.0f,
		 1.0f, -1.0f,
		-1.0f,  1.0f,
		 1.0f,  1.0f
	};
	_vbo.allocate(verts, sizeof(verts));

	int posLoc = _program.attributeLocation("aPos");
	_program.enableAttributeArray(posLoc);
	_program.setAttributeBuffer(posLoc, GL_FLOAT, 0, 2, 0);
	_program.release();
	// _vbo.release();

	_initialized = true;
}

void FrmGlWidget::resizeGL(int w, int h) {
	_viewWidth = w;
	_viewHeight = h;
}

void FrmGlWidget::paintGL() {
	if (!_initialized) {
		return;
	}


	glViewport(0, 0, _viewWidth, _viewHeight);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	doUpdateAtlas();
	doApplyCurrentFrame();

	if (!_texture) {
		return;
	}

	QOpenGLVertexArrayObject::Binder vaoBinder(&_vao);

	_program.bind();
	_texture->bind();

	QMatrix4x4 proj;
	proj.ortho(0, _viewWidth, _viewHeight, 0, -1, 1);
	_program.setUniformValue("uProjection", proj);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	_program.release();
}

void FrmGlWidget::doUpdateAtlas() {
	if (!_atlasPending) {
		return;
	}

	if (!_texture) {
		_texture = std::make_unique<QOpenGLTexture>(QOpenGLTexture::Target2D);
	}

	_texture->setMinificationFilter(QOpenGLTexture::Nearest);
	_texture->setMagnificationFilter(QOpenGLTexture::Nearest);
	_texture->setWrapMode(QOpenGLTexture::ClampToEdge);
	_texture->setData(_atlas->atlas());
	_atlasPending = false;
}

void FrmGlWidget::doApplyCurrentFrame() {
	if (!_applyCurrentFrame) {
		return;
	}

	const auto& e = _atlas->entries()[_currentFrame];

	_program.bind();
	_program.setUniformValue("uUVRect", QVector4D(e.u0, e.v0, e.u1, e.v1));

	QMatrix4x4 model;
	model.translate(e.offsetX, e.offsetY);
	_program.setUniformValue("uModel", model);
	_applyCurrentFrame = false;
}

void FrmGlWidget::cleanup() {
	if (_texture) {
		_texture->destroy();
	}

	_vbo.destroy();
	_vao.destroy();
}

void FrmGlWidget::uploadFrames(const std::vector<Proto::Frame>& frames, const Proto::Pallete& pallete) {
	if (!_initialized && frames.empty()) {
		return;
	}

	// rebuild atlas
	_atlas = std::make_unique<DataFormat::Frm::AtlasBuilder>();
	_atlas->build(frames, pallete);

	// update texture
	_atlasPending = true;
}

void FrmGlWidget::setCurrentFrame(int value) {
	if (value < 0 || value >= _atlas->entries().size()) {
		return;
	}

	if (_currentFrame == value) {
		return;
	}

	_currentFrame = value;
	_applyCurrentFrame = true;
}

void FrmGlWidget::setFrames(const std::vector<Proto::Frame> &frames, const Proto::Pallete &pallete) {
  uploadFrames(frames, pallete);
	setCurrentFrame(0);
  update(); // → paintGL()
}
