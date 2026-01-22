#include "Content/Shared/Draw/draw_program.h"
#include <QOpenGLContext>
#include <QOpenGLFunctions>

void DrawProgram::create(const QString& vertCode, const QString& fragCode)
{
	if (!_program.addShaderFromSourceCode(QOpenGLShader::Vertex, vertCode)) {
		qWarning() << "Vertex shader failed:" << _program.log();
	}

	if (!_program.addShaderFromSourceCode(QOpenGLShader::Fragment, fragCode)) {
		qWarning() << "Fragment shader failed:" << _program.log();
	}

	if (!_program.link()) {
		qFatal("Shader linking failed: %s", qPrintable(_program.log()));
	}
}

void DrawProgram::setUniformValue(const char* name, const QMatrix4x4& value) {
	_program.setUniformValue(name, value);
}

void DrawProgram::setUniformValue(const char* name, const QVector3D& value) {
	_program.setUniformValue(name, value);
}

void DrawProgram::setUniformValue(const char* name, float value) {
	_program.setUniformValue(name, value);
}

void DrawProgram::add(std::unique_ptr<DrawBuffer> buffer) {
	_buffers.push_back(std::move(buffer));
}

void DrawProgram::render() {

	auto* f = QOpenGLContext::currentContext()->functions();

	_program.bind();

	for (auto& buffer : _buffers)
	{
		if (!buffer->isEnabled()) {
			continue;
		}

		buffer->bind();
		buffer->render(this, f);
		buffer->release();
	}

	_program.release();
}

bool DrawProgram::validate() const
{
	return _program.isLinked();
}

void DrawProgram::bind()
{
	_program.bind();
}

void DrawProgram::release()
{
	_program.release();
}


