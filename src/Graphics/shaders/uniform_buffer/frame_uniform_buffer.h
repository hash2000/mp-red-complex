#pragma once
#include "Graphics/shaders/uniform_buffer.h"
#include <QVector2D>
#include <QVector4D>

class FrameUniformBuffer : public UniformBuffer {
public:
	FrameUniformBuffer();
	~FrameUniformBuffer() override;

	void setZLevel(float value);
	void setTime(float value);
	void setTileAnumTime(float value);
	void setUVOffset(const QVector2D& value);
	void setTileSize(const QVector2D& value);
	void setBorderColor(const QVector4D& value);

	bool initialize() override;
	void flush() override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
