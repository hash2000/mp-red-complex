#include "Launcher/widgets/animation/frames_renderer_gl.h"
#include <QOpenGLBuffer>
#include <QOpenGLContext>


static const char *vertexShaderSource = R"(
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

static const char *fragmentShaderSource = R"(
varying vec2 vTexCoord;
uniform sampler2D uAtlas;
uniform vec4 uUVRect; // x=u0, y=v0, z=u1, w=v1
void main() {
    vec2 uv = mix(uUVRect.xy, uUVRect.zw, vTexCoord);
    gl_FragColor = texture2D(uAtlas, uv);
}
)";

OpenGLFramesRenderer::OpenGLFramesRenderer() {
}

OpenGLFramesRenderer::~OpenGLFramesRenderer() {
	cleanup();
}

QString OpenGLFramesRenderer::name() const {
	return "OpenGL";
}

void OpenGLFramesRenderer::cleanup() {
	if (_texture) {
		_texture->destroy();
	}

	_vbo.destroy();
	_vao.destroy();
}

void OpenGLFramesRenderer::initialize() {
  if (_initialized) {
    return;
  }

  initializeOpenGLFunctions();

  // --- Shaders ---
  if (!_program.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
    qWarning() << "Vertex shader failed:" << _program.log();
	}

	if (!_program.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource)) {
		qWarning() << "Fragment shader failed:" << _program.log();
	}

  if (!_program.link()) {
    qWarning() << "Shader program link failed:" << _program.log();
	}

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

void OpenGLFramesRenderer::render() {

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

	if (!_texture){
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

void OpenGLFramesRenderer::uploadFrames(const std::vector<Proto::Frame> &frames, const Proto::Pallete &pallete) {
	if (!_initialized && frames.empty()) {
		return;
	}

	// rebuild atlas
	_atlas = std::make_unique<DataFormat::Frm::AtlasBuilder>();
	_atlas->build(frames, pallete);

	// update texture
	_atlasPending = true;
}

void OpenGLFramesRenderer::doUpdateAtlas() {
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

void OpenGLFramesRenderer::setCurrentFrame(int value) {

	if (value < 0 || value >= _atlas->entries().size()) {
		return;
	}

	if (_currentFrame == value) {
		return;
	}

	_currentFrame = value;
	_applyCurrentFrame = true;
}

void OpenGLFramesRenderer::doApplyCurrentFrame() {
	if (!_applyCurrentFrame) {
		return;
	}

	const auto &e = _atlas->entries()[_currentFrame];

	_program.bind();
	_program.setUniformValue("uUVRect", QVector4D(e.u0, e.v0, e.u1, e.v1));

	QMatrix4x4 model;
  model.translate(e.offsetX, e.offsetY);
  _program.setUniformValue("uModel", model);
	_applyCurrentFrame = false;
}

void OpenGLFramesRenderer::resize(int width, int height) {
	_viewWidth = width;
  _viewHeight = height;
}


bool OpenGLFramesRenderer::initialized() const {
	return _initialized;
}
