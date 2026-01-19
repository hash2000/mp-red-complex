#pragma once
#include "Content/Shared/Draw/draw_buffer.h"
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <vector>
#include <memory>

class DrawProgram: protected QOpenGLFunctions_3_3_Core {
public:
	void create(const QString& vertCode, const QString& fragCode);
	void bind();
	void release();

	void setUniformValue(const char* name, const QMatrix4x4& value);
	void setUniformValue(const char* name, const QVector3D& value);
	void setUniformValue(const char* name, float value);

	void add(std::unique_ptr<DrawBuffer> buffer);
	void render();

	bool validate() const;

private:
	QOpenGLShaderProgram _program;
	std::vector<std::unique_ptr<DrawBuffer>> _buffers;
};
