#include "Libs/Base/crypto/security_buffer.h"

#include <sodium.h>

class SecureBuffer::Private {
public:
	Private(SecureBuffer* parent) : q(parent) { }
	SecureBuffer* q;
	unsigned char* data = nullptr;
	int size = 0;
};

SecureBuffer::SecureBuffer(int size)
	: d(std::make_unique<Private>(this)) {
	// sodium_malloc выделяет память, которую:
	// 1. Нельзя выгрузить в swap (автоматический mlock)
	// 2. Автоматически затирается при освобождении
	// 3. Имеет guard pages вокруг (защита от buffer overflow)
	d->data = static_cast<unsigned char*>(sodium_malloc(size));
	d->size = size;
}

SecureBuffer::SecureBuffer(SecureBuffer&& other) noexcept
	: d(std::make_unique<Private>(this)) {
	d->data = other.d->data;
	d->size = other.d->size;
	other.d->data = nullptr;
	other.d->size = 0;
}

SecureBuffer::~SecureBuffer() {
	clear();
}

unsigned char* SecureBuffer::data() {
	return d->data;
}

const unsigned char* SecureBuffer::data() const {
	return d->data;
}

int SecureBuffer::size() const {
	return d->size;
}

void SecureBuffer::clear() {
	// sodium_free автоматически вызывает sodium_memzero
	if (d->data) {
		sodium_free(d->data);
	}

	d->data = nullptr;
	d->size = 0;
}


QByteArray SecureBuffer::toQByteArray() const {
	return QByteArray::fromRawData(
		reinterpret_cast<const char*>(d->data), d->size);
}
