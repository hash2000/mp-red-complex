#include "Content/UsersModule/extensions/crypto_extensions.h"

#include <QCryptographicHash>

#include <stdexcept>
#include <sodium.h>

namespace Extensions::Users::Crypto {

// Вычисляем user_hash из мастер-сида (SHA-256)
QString computeUserHash(const QByteArray& masterSeed) {
	QByteArray hash = QCryptographicHash::hash(masterSeed, QCryptographicHash::Sha256);
	return hash.toHex();
}

QByteArray deriveEncryptionKey(const QString& password, const QByteArray& salt) {
	std::vector<uint8_t> enc_key(crypto_box_SEEDBYTES);
	QByteArray pwdUtf8 = password.toUtf8();

	if (crypto_pwhash(
		enc_key.data(), enc_key.size(),
		pwdUtf8.constData(), pwdUtf8.length(),
		reinterpret_cast<const uint8_t*>(salt.constData()),
		crypto_pwhash_OPSLIMIT_SENSITIVE,  // Максимальная защита для seed
		crypto_pwhash_MEMLIMIT_SENSITIVE,
		crypto_pwhash_ALG_ARGON2ID13) != 0) {
		throw std::runtime_error("Key derivation failed (out of memory?)");
	}

	return QByteArray::fromRawData(reinterpret_cast<const char*>(enc_key.data()), enc_key.size());
}

QByteArray encryptSeedPhrase(const QString& seedPhrase, const QByteArray& encKey) {
	QByteArray seedBytes = seedPhrase.toUtf8();

	std::vector<uint8_t> nonce(crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
	randombytes_buf(nonce.data(), nonce.size());

	std::vector<uint8_t> ciphertext(
		seedBytes.size() + crypto_aead_xchacha20poly1305_ietf_ABYTES
	);
	unsigned long long ciphertext_len;

	crypto_aead_xchacha20poly1305_ietf_encrypt(
		ciphertext.data(), &ciphertext_len,
		reinterpret_cast<const uint8_t*>(seedBytes.constData()), seedBytes.size(),
		nullptr, 0, nullptr,
		nonce.data(),
		reinterpret_cast<const uint8_t*>(encKey.constData())
	);

	// Формат: [nonce (24 байта)] [ciphertext]
	QByteArray result;
	result.append(QByteArray::fromRawData(reinterpret_cast<const char*>(nonce.data()), nonce.size()));
	result.append(QByteArray::fromRawData(reinterpret_cast<const char*>(ciphertext.data()), ciphertext_len));

	return result;
}

// Выводим ключ для SQLCipher из мастер-сида (детерминированно)
QByteArray deriveDatabaseKey(const QByteArray& masterSeed) {
	std::vector<uint8_t> db_key(crypto_box_SEEDBYTES); // 32 байта

	// crypto_kdf_derive_from_key требует контекст ровно 8 байт
	crypto_kdf_derive_from_key(
		db_key.data(), db_key.size(),
		1,              // subkey index (1 = ключ для БД)
		"appdbkey ",    // context (ровно 8 байт, с пробелом на конце)
		reinterpret_cast<const uint8_t*>(masterSeed.constData())
	);

	return QByteArray::fromRawData(reinterpret_cast<const char*>(db_key.data()), db_key.size());
}

// Расшифровываем seed-фразу паролем
QString decryptSeedPhrase(const QByteArray& encryptedData, const QByteArray& encKey) {
	int nonceSize = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;

	if (encryptedData.size() < nonceSize + crypto_aead_xchacha20poly1305_ietf_ABYTES) {
		throw std::runtime_error("Encrypted data too short");
	}

	QByteArray nonce = encryptedData.left(nonceSize);
	QByteArray ciphertext = encryptedData.mid(nonceSize);

	std::vector<uint8_t> plaintext(ciphertext.size());
	unsigned long long plaintext_len;

	int result = crypto_aead_xchacha20poly1305_ietf_decrypt(
		plaintext.data(), &plaintext_len, nullptr,
		reinterpret_cast<const uint8_t*>(ciphertext.constData()), ciphertext.size(),
		nullptr, 0,
		reinterpret_cast<const uint8_t*>(nonce.constData()),
		reinterpret_cast<const uint8_t*>(encKey.constData())
	);

	if (result != 0) {
		throw std::runtime_error("Decryption failed (wrong password or corrupted data)");
	}

	return QString::fromUtf8(reinterpret_cast<const char*>(plaintext.data()), plaintext_len);
}
}
