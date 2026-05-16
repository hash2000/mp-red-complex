#pragma once
#include <memory>

class UniformBuffer {
public:
	explicit UniformBuffer();
	virtual ~UniformBuffer();

	// Запрет копирования
	UniformBuffer(const UniformBuffer&) = delete;
	UniformBuffer& operator=(const UniformBuffer&) = delete;

	// Инициализация: выделяет память нужного размера
	bool create(size_t size);
	bool isCreated() const;
	void destroy();

	// Запись данных в буфер
	void write(size_t offset, const void* data, size_t size);

	// Привязка к глобальному слоту (один раз при инициализации)
	void bindToSlot(uint32_t bindingPoint);

	// Обновить данные (каждый наследник переопределяет)
	virtual void flush() = 0;

	// Получить ID буфера
	uint32_t bufferId() const;

	// Размер буфера
	size_t size() const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
