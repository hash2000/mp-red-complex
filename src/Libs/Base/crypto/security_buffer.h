#pragma once
#include <QByteArray>
#include <memory>

class SecureBuffer {
public:
	// Выделяем защищённую память заданного размера
	explicit SecureBuffer(size_t size);
	~SecureBuffer();

	// ЗАПРЕЩАЕМ копирование, чтобы ключи не размножались в памяти
	SecureBuffer(const SecureBuffer&) = delete;
	SecureBuffer& operator=(const SecureBuffer&) = delete;

	// РАЗРЕШАЕМ перемещение (move semantics)
	SecureBuffer(SecureBuffer&& other) noexcept;
	SecureBuffer& operator=(SecureBuffer&& other) noexcept;

	unsigned char* data();
	const unsigned char* data() const;
	size_t size() const;
	bool isEmpty() const;

	// Безопасно затирает содержимое, но не освобождает память (буфер можно переиспользовать)
	void clear();

	// Копирует данные в буфер, предварительно затерев старое содержимое
	void setData(const unsigned char* newData, size_t newSize);
	void setData(const QByteArray& data);

	// Более безопасная альтернатива: сразу получаем HEX-строку для PRAGMA key
	QByteArray toHex() const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
