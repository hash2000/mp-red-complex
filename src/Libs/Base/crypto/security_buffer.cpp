#include "Libs/Base/crypto/security_buffer.h"

#include <stdexcept>
#include <sodium.h>

class SecureBuffer::Private {
public:
	Private(SecureBuffer* parent) : q(parent) { }
	SecureBuffer* q;
	bool empty = true;
	unsigned char* data = nullptr;
	size_t size = 0;
};

SecureBuffer::SecureBuffer(size_t size)
	: d(std::make_unique<Private>(this)) {
	if (size == 0) {
		throw std::invalid_argument("SecureBuffer size must be greater than 0");
	}

	// sodium_malloc гарантирует:
	// 1. Выделение памяти, защищённой от выгрузки в swap (mlock)
	// 2. Наличие guard pages для защиты от переполнения буфера
	// 3. Автоматическое обнуление при вызове sodium_free
	d->data = static_cast<unsigned char*>(sodium_malloc(size));
	if (!d->data) {
		throw std::runtime_error("Failed to allocate secure memory (sodium_malloc)");
	}

	d->size = size;
	d->empty = true;
}

SecureBuffer::SecureBuffer(SecureBuffer&& other) noexcept
	: d(std::move(other.d)) {
}

SecureBuffer::~SecureBuffer() {
	if (d->data) {
		sodium_free(d->data);
	}
}
SecureBuffer& SecureBuffer::operator=(SecureBuffer&& other) noexcept {
	if (this != &other) {
		d = std::move(other.d);
	}
	return *this;
}

unsigned char* SecureBuffer::data() {
	return d->empty ? nullptr : d->data;
}

const unsigned char* SecureBuffer::data() const {
	return d->empty ? nullptr : d->data;
}

size_t SecureBuffer::size() const {
	return d->size;
}

bool SecureBuffer::isEmpty() const {
	return d->empty;
}

void SecureBuffer::clear() {
	if (d->data && !d->empty) {
		sodium_memzero(d->data, d->size);
		d->empty = true;
	}
}

void SecureBuffer::setData(const unsigned char* newData, size_t newSize) {
	if (!d->data) {
		return;
	}

	sodium_memzero(d->data, d->size);

	size_t copySize = std::min(newSize, d->size);
	if (copySize > 0) {
		std::memcpy(d->data, newData, copySize);
		d->empty = false;
	}
	else {
		d->empty = true;
	}
}

void SecureBuffer::setData(const QByteArray& data) {
	setData(reinterpret_cast<const unsigned char*>(data.constData()), data.size());
}

QByteArray SecureBuffer::toHex() const {
	if (d->empty || !d->data) {
		return { };
	}

	return QByteArray::fromRawData(reinterpret_cast<const char*>(d->data), d->size).toHex();
}
