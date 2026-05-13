#include "Graphics/shaders/uniform_buffer.h"
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLBuffer>

class UniformBuffer::Private : public QOpenGLFunctions_4_5_Core {
public:
	Private(UniformBuffer* parent) : q(parent) {}
	UniformBuffer* q;

	QOpenGLBuffer buffer;
	size_t bufferSize = 0;
	GLuint currentBindingPoint = GL_INVALID_INDEX;
	bool created = false;
};

UniformBuffer::UniformBuffer()
	: d(std::make_unique<Private>(this)) {
}

UniformBuffer::~UniformBuffer() {
	destroy();
}

bool UniformBuffer::create(size_t size) {
	if (d->created) {
		return true;
	}

	d->initializeOpenGLFunctions();

	if (!d->buffer.create()) {
		qWarning() << "UniformBuffer: failed to create buffer";
		return false;
	}

	d->buffer.bind();
	d->buffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
	d->buffer.allocate(static_cast<int>(size));
	d->buffer.release();

	d->bufferSize = size;
	d->created = true;
	return true;
}

bool UniformBuffer::isCreated() const {
	return d->created;
}

void UniformBuffer::destroy() {
	if (d->created) {
		d->buffer.destroy();
		d->created = false;
		d->bufferSize = 0;
	}
}

void UniformBuffer::write(size_t offset, const void* data, size_t size) {
	if (!d->created) {
		return;
	}

	d->buffer.bind();
	d->glBufferSubData(GL_UNIFORM_BUFFER,
		static_cast<GLintptr>(offset),
		static_cast<GLsizeiptr>(size),
		data);
	d->buffer.release();
}

void UniformBuffer::bindToSlot(uint32_t bindingPoint) {
	if (!d->created) {
		return;
	}

	d->currentBindingPoint = bindingPoint;
	d->glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, d->buffer.bufferId());
}

uint32_t UniformBuffer::bufferId() const {
	return d->buffer.bufferId();
}

size_t UniformBuffer::size() const {
	return d->bufferSize;
}
