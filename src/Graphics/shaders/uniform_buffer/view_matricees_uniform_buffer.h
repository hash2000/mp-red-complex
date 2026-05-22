#pragma once
#include "Graphics/shaders/uniform_buffer.h"
#include <QMatrix4x4>
#include <memory>

class ViewMatricesUniformBuffer : public UniformBuffer {
public:
	ViewMatricesUniformBuffer();
	~ViewMatricesUniformBuffer() override;

	void setProjection(const QMatrix4x4& m);
	void setView(const QMatrix4x4& m);

	bool initialize() override;
	void flush() override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
