#include "Libs/Base/crypto/bip_39.h"
#include "Libs/Resources/resources.h"

#include <QByteArray>
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QTextStream>

#include <sodium.h>


class Bip39::Private {
public:
	Private(Bip39* parent) : q(parent) {}
	Bip39* q;

	Resources* resources;
	QStringList wordlist;
	bool loadWordlist();

	static QByteArray pbkdf2_hmac_sha512(const QByteArray& password,
		const QByteArray& salt,
		int iterations,
		int keyLength);
};

Bip39::Bip39(Resources* resources)
	: d(std::make_unique<Private>(this)) {
	d->resources = resources;

	if (sodium_init() < 0) {
		throw std::runtime_error("Failed to initialize libsodium");
	}

	if (!d->loadWordlist()) {
		throw std::runtime_error("Failed to load BIP39 wordlist from resources");
	}
}

Bip39::~Bip39() = default;

bool Bip39::Private::loadWordlist() {
	auto recoveryFile = resources->Directories.get(DirectoryPath::RecoveryWordsFile);
	if (!recoveryFile) {
		throw std::invalid_argument("Recovery file is not set.");
	}

	QFile file(recoveryFile.value().absolutePath());
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return false;
	}

	QTextStream in(&file);
	while (!in.atEnd()) {
		QString line = in.readLine().trimmed();
		if (line.isEmpty()) {
			continue;
		}

		// Ищем двоеточие, отделяющее метаданные от слов
		int colonPos = line.indexOf(':');
		if (colonPos < 0) {
			continue;
		}

		// Берем часть после двоеточия: "abandon,ability,able,about,above"
		QString wordsPart = line.mid(colonPos + 1);

		// Разделяем слова по запятым и добавляем в список
		const QStringList words = wordsPart.split(',', Qt::SkipEmptyParts);
		for (const QString& word : words) {
			wordlist.append(word.trimmed());
		}
	}

	return wordlist.size() == 2048;
}

QStringList Bip39::generateMnemonic(int strengthBits) {
	if (strengthBits != 128 && strengthBits != 256) {
		throw std::invalid_argument("Strength must be 128 or 256 bits");
	}

	int entropyBytes = strengthBits / 8;
	std::vector<uint8_t> entropy(entropyBytes);
	randombytes_buf(entropy.data(), entropy.size());

	// SHA-256 от энтропии
	QByteArray entropyBA = QByteArray::fromRawData(
		reinterpret_cast<const char*>(entropy.data()), entropy.size());
	QByteArray hash = QCryptographicHash::hash(entropyBA, QCryptographicHash::Sha256);

	// Checksum: для 128 бит энтропии = 4 бита, для 256 бит = 8 бит
	int checksumBits = strengthBits / 32;  // 4 или 8

	// Собираем все биты в один поток: энтропия + checksum
	// Используем вектор битов для точности
	std::vector<bool> bits;
	bits.reserve(strengthBits + checksumBits);

	// Добавляем биты энтропии
	for (int i = 0; i < entropyBytes; ++i) {
		for (int bit = 7; bit >= 0; --bit) {
			bits.push_back((entropy[i] >> bit) & 1);
		}
	}

	// Добавляем биты checksum (старшие биты hash[0])
	for (int i = 0; i < checksumBits; ++i) {
		bits.push_back((hash[0] >> (7 - i)) & 1);
	}

	// Разбиваем на 11-битные индексы
	QStringList words;
	for (size_t i = 0; i < bits.size(); i += 11) {
		int index = 0;
		for (int j = 0; j < 11; ++j) {
			index = (index << 1) | (bits[i + j] ? 1 : 0);
		}
		words.append(d->wordlist.at(index));
	}

	sodium_memzero(entropy.data(), entropy.size());
	return words;
}

std::vector<uint8_t> Bip39::validateAndGetEntropy(const QStringList& words) const {
	if (words.size() != 12 && words.size() != 24) {
		throw std::invalid_argument("Mnemonic must have 12 or 24 words");
	}

	// Преобразуем слова в индексы
	std::vector<int> indices;
	indices.reserve(words.size());
	for (const QString& w : words) {
		int idx = d->wordlist.indexOf(w.toLower().trimmed());
		if (idx < 0) {
			throw std::runtime_error("Unknown word in mnemonic: " + w.toStdString());
		}
		indices.push_back(idx);
	}

	// Собираем все биты из 11-битных индексов
	std::vector<bool> bits;
	bits.reserve(words.size() * 11);
	for (int idx : indices) {
		for (int bit = 10; bit >= 0; --bit) {
			bits.push_back((idx >> bit) & 1);
		}
	}

	// Определяем размер энтропии и checksum
	int totalBits = bits.size();  // 132 для 12 слов, 264 для 24 слов
	int checksumBits = totalBits / 33;  // 4 для 12 слов, 8 для 24 слов
	int entropyBits = totalBits - checksumBits;
	int entropyBytes = entropyBits / 8;

	// Извлекаем энтропию
	std::vector<uint8_t> entropy(entropyBytes, 0);
	for (int i = 0; i < entropyBits; ++i) {
		if (bits[i]) {
			entropy[i / 8] |= (1 << (7 - (i % 8)));
		}
	}

	// Извлекаем checksum
	uint8_t actualChecksum = 0;
	for (int i = 0; i < checksumBits; ++i) {
		if (bits[entropyBits + i]) {
			actualChecksum |= (1 << (checksumBits - 1 - i));
		}
	}

	// Вычисляем ожидаемый checksum
	QByteArray entropyBA = QByteArray::fromRawData(
		reinterpret_cast<const char*>(entropy.data()), entropy.size());
	QByteArray hash = QCryptographicHash::hash(entropyBA, QCryptographicHash::Sha256);

	uint8_t expectedChecksum = (hash[0] >> (8 - checksumBits)) & ((1 << checksumBits) - 1);

	if (expectedChecksum != actualChecksum) {
		throw std::runtime_error("Invalid mnemonic checksum");
	}

	return entropy;
}

QByteArray Bip39::mnemonicToSeed(const QStringList& words, const QString& passphrase) const {
	validateAndGetEntropy(words);

	QString joined = words.join(' ');
	QByteArray mnemonicData = joined.toUtf8();
	QString saltStr = "mnemonic" + passphrase;
	QByteArray saltData = saltStr.toUtf8();
	QByteArray seed = Private::pbkdf2_hmac_sha512(mnemonicData, saltData, 2048, 64);

	return seed;
}

QByteArray Bip39::Private::pbkdf2_hmac_sha512(const QByteArray& password,
	const QByteArray& salt,
	int iterations,
	int keyLength) {
	int hashLength = 64; // SHA-512 дает 64 байта
	int blocks = (keyLength + hashLength - 1) / hashLength;

	QByteArray result;
	result.reserve(keyLength);

	for (int blockIndex = 1; blockIndex <= blocks; ++blockIndex) {
		// Формируем salt || INT_32_BE(blockIndex)
		QByteArray saltWithBlock = salt;
		saltWithBlock.append(static_cast<char>((blockIndex >> 24) & 0xFF));
		saltWithBlock.append(static_cast<char>((blockIndex >> 16) & 0xFF));
		saltWithBlock.append(static_cast<char>((blockIndex >> 8) & 0xFF));
		saltWithBlock.append(static_cast<char>(blockIndex & 0xFF));

		// U1 = HMAC-SHA512(password, salt || INT_32_BE(blockIndex))
		QMessageAuthenticationCode hmac1(QCryptographicHash::Sha512);
		hmac1.setKey(password);
		hmac1.addData(saltWithBlock);
		QByteArray u = hmac1.result();

		QByteArray t = u;

		// U2 ... Uc
		for (int i = 1; i < iterations; ++i) {
			QMessageAuthenticationCode hmacIter(QCryptographicHash::Sha512);
			hmacIter.setKey(password);
			hmacIter.addData(u);
			u = hmacIter.result();

			// t = t XOR u
			for (int j = 0; j < t.size(); ++j) {
				t[j] = t[j] ^ u[j];
			}
		}

		result.append(t);
	}

	return result.left(keyLength);
}

