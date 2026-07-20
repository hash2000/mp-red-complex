#pragma once
#include <QByteArray>
#include <memory>

class SecureBuffer {
public:
	explicit SecureBuffer(int size);
	~SecureBuffer();

	// Запрещаем копирование (чтобы ключ не размножался в памяти)
	SecureBuffer(const SecureBuffer&) = delete;
	SecureBuffer(SecureBuffer&& other) noexcept;
	SecureBuffer& operator=(const SecureBuffer&) = delete;

	unsigned char* data();
	const unsigned char* data() const;
	int size() const;

	void clear();

	QByteArray toQByteArray() const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
