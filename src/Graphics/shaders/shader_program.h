#pragma once
#include "Graphics/shaders/material_params.h"

class UniformBuffer;

class ShaderProgram {
public:
	ShaderProgram();
	~ShaderProgram();

public:
	bool initialize(const QString& vertex, const QString& fragment);
	bool isInitialized() const;

	void bind();
	void release();

	// Настройка привязки UBO к именованному блоку
	// blockName — имя uniform-блока в шейдере (например "FrameUBO")
	// bindingPoint — слот, куда привязываем (0, 1, 2...)
	void bindUniformBlock(const char* blockName, uint32_t bindingPoint);

	// Или проще: связать блок с конкретным UniformBuffer
	// buffer должен быть уже привязан к слоту через buffer->bindToSlot(slot)
	void linkBlockToSlot(const char* blockName, uint32_t slot);

	bool validateBlockSize(const char* blockName, size_t expectedSize);

	uint32_t programId() const;

	// Установка uniform-значений
	void setUniformValue(const char* name, int value);
	void setUniformValue(const char* name, float value);
	void setUniformValue(const char* name, const QMatrix4x4& value);
	void setUniformValue(const char* name, const QVector4D& value);

private:
	class Private;
	std::unique_ptr<Private> d;
};
