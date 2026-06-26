#pragma once
#include <memory>

class UniformBuffer {
public:
	explicit UniformBuffer();
	virtual ~UniformBuffer();

	// Запрет копирования
	UniformBuffer(const UniformBuffer&) = delete;
	UniformBuffer& operator=(const UniformBuffer&) = delete;

	bool isCreated() const;
	void destroy();

	// Привязка к глобальному слоту (один раз при инициализации)
	void bindToSlot(uint32_t bindingPoint);

	// инициализация буфера
	virtual bool initialize() = 0;

	// Обновить данные (каждый наследник переопределяет)
	virtual void flush() = 0;

	// Получить ID буфера
	uint32_t bufferId() const;

	// Размер буфера
	size_t size() const;

protected:
	// Инициализация: выделяет память нужного размера
	bool create(size_t size);

	// Запись данных в буфер
	void write(size_t offset, const void* data, size_t size);

private:
	class Private;
	std::unique_ptr<Private> d;
};
