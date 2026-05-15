#include "Graphics/shaders/uniform_buffer/frame_uniform_buffer.h"

namespace {
	// Заполняем данные UBO
	struct alignas(16) UBOData {
		float zLevel;
		float _pad1[3];
		uint32_t flags;
		float _pad2[3];
		float time;
		float tileAnimFrame;
		QVector2D uvOffset;
		QVector2D tileSize;
		QVector4D borderColor;
		QVector4D wetParams;
		QVector4D fireParams;
		float _pad3[4];
	};
	constexpr int kUBOSize = sizeof(UBOData);
	static_assert(kUBOSize == 128, "FrameUniformBuffer UBOData size mismatch");
	static_assert(alignof(UBOData) == 16, "FrameUniformBuffer UBOData must be 16-byte aligned");
}

class FrameUniformBuffer::Private {
public:
	Private(FrameUniformBuffer* parent) : q(parent) {}
	FrameUniformBuffer* q;

	UBOData ubo;
	bool dirty = false;
};

FrameUniformBuffer::FrameUniformBuffer()
	: d(std::make_unique<Private>(this)) {
	std::memset(&d->ubo, 0, kUBOSize);
}

FrameUniformBuffer::~FrameUniformBuffer() = default;

void FrameUniformBuffer::setZLevel(float value) {
	d->ubo.zLevel = value;
	d->dirty = true;
}

void FrameUniformBuffer::setFlags(uint32_t flags) {
	d->ubo.flags = flags;
	d->dirty = true;
}

uint32_t FrameUniformBuffer::flags() const {
	return d->ubo.flags;
}

void FrameUniformBuffer::setTime(float value) {
	d->ubo.time = value;
	d->dirty = true;
}

void FrameUniformBuffer::setTileAnumTime(float value) {
	d->ubo.tileAnimFrame = value;
	d->dirty = true;
}

void FrameUniformBuffer::setUVOffset(const QVector2D& value) {
	d->ubo.uvOffset = value;
	d->dirty = true;
}

void FrameUniformBuffer::setTileSize(const QVector2D& value) {
	d->ubo.tileSize = value;
	d->dirty = true;
}

void FrameUniformBuffer::setBorderColor(const QVector4D& value) {
	d->ubo.borderColor = value;
	d->dirty = true;
}

void FrameUniformBuffer::setWetParams(const QVector4D& value) {
	d->ubo.wetParams = value;
	d->dirty = true;
}

void FrameUniformBuffer::setFireParams(const QVector4D& value) {
	d->ubo.fireParams = value;
	d->dirty = true;
}

void FrameUniformBuffer::flush() {
	if (!d->dirty || !isCreated()) {
		return;
	}

	d->dirty = false;
	write(0, &d->ubo, kUBOSize);
}
