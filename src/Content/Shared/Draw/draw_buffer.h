#pragma once
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <vector>

class DrawProgram;

class DrawBuffer: protected QOpenGLFunctions_3_3_Core {
public:
	struct Attribute {
		int location = 0;
		int size = 3;
		GLenum type = GL_FLOAT;
		bool normalized = false;
		int stride = 0;
		void* offset = nullptr;
	};

public:
	virtual ~DrawBuffer() = default;

	void create(
		const std::vector<float>& data,
		const std::vector<Attribute>& attributes);

	void bind();
	void release();

	virtual void render(DrawProgram* program, QOpenGLFunctions* fn) { }

private:
	QOpenGLVertexArrayObject _vao;
	QOpenGLBuffer _vbo;
};
