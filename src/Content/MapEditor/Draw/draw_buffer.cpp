#include "Content/MapEditor/Draw/draw_buffer.h"
#include <QOpenGLContext>
#include <QOpenGLFunctions>

void DrawBuffer::create(
	const std::vector<float>& data,
	const std::vector<Attribute>& attributes)
{
	auto* f = QOpenGLContext::currentContext()->functions();

	_vbo.create();
	_vbo.bind();
	_vbo.allocate(data.data(), static_cast<int>(data.size() * sizeof(float)));

	_vao.create();
	_vao.bind();

	for (const auto& attr : attributes) {
		f->glEnableVertexAttribArray(attr.location);
		f->glVertexAttribPointer(
			attr.location,
			attr.size,
			attr.type,
			attr.normalized ? GL_TRUE : GL_FALSE,
			attr.stride,
			attr.offset
		);
	}

	_vbo.release();
	_vao.release();
}

void DrawBuffer::bind() {
	_vao.bind();
}

void DrawBuffer::release() {
	_vao.release();
}

void DrawBuffer::enable(bool set) {
	_enabled = set;
}

bool DrawBuffer::isEnabled() const {
	return _enabled;
}
