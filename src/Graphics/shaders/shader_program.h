#pragma once
#include "Graphics/shaders/material_params.h"

class SharedShaderProgram {
public:
	SharedShaderProgram();
	~SharedShaderProgram();

public:
	bool initialize(const QString& vertex, const QString& fragment);
	void bind();
	void release();

	/// Установить матрицы проекции и вида
	void setMatrices(const QMatrix4x4& projection, const QMatrix4x4& view);

	/// Установить глобальное время
	void setTime(float time);

	/// Обновить параметры UBO
	void updateParams(const MaterialParams& params);

	bool isInitialized() const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
