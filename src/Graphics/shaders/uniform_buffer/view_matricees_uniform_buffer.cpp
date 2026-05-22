#include "Graphics/shaders/uniform_buffer/view_matricees_uniform_buffer.h"

namespace {
	// Заполняем данные UBO
	struct alignas(16) UBOData {
		QMatrix4x4 projection;
		QMatrix4x4 view;
	};
	constexpr int kUBOSize = sizeof(UBOData);
	static_assert(kUBOSize % 16 == 0, "ViewMatricesUniformBuffer UBOData size mismatch");
	static_assert(alignof(UBOData) == 16, "ViewMatricesUniformBuffer UBOData must be 16-byte aligned");
}

class ViewMatricesUniformBuffer::Private {
public:
	Private(ViewMatricesUniformBuffer* parent) : q(parent) {}
	ViewMatricesUniformBuffer* q;

	UBOData ubo;
	bool dirty = false;
};

ViewMatricesUniformBuffer::ViewMatricesUniformBuffer()
	: d(std::make_unique<Private>(this)) {
}

ViewMatricesUniformBuffer::~ViewMatricesUniformBuffer() = default;

void ViewMatricesUniformBuffer::setProjection(const QMatrix4x4& m) {
	d->ubo.projection = m;
	d->dirty = true;
}

void ViewMatricesUniformBuffer::setView(const QMatrix4x4& m) {
	d->ubo.view = m;
	d->dirty = true;
}

void ViewMatricesUniformBuffer::flush() {
	if (!d->dirty || !isCreated()) {
		return;
	}

	d->dirty = false;
	write(0, &d->ubo, kUBOSize);
}

bool ViewMatricesUniformBuffer::initialize() {
	return create(kUBOSize);
}
