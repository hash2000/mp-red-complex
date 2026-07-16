#include "Content/UsersModule/crypto/bip_39.h"
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
	QFile file(":/bip39/english.txt");
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return false;
	}
	QTextStream in(&file);
	while (!in.atEnd()) {
		QString word = in.readLine().trimmed();
		if (!word.isEmpty()) {
			wordlist.append(word);
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

	QByteArray entropyBA = QByteArray::fromRawData(
		reinterpret_cast<const char*>(entropy.data()), entropy.size());
	QByteArray hash = QCryptographicHash::hash(entropyBA, QCryptographicHash::Sha256);

	std::vector<uint8_t> data = entropy;
	data.push_back(static_cast<uint8_t>(hash[0]));

	QStringList words;
	int bits = 0;
	uint32_t accumulator = 0;

	for (uint8_t byte : data) {
		accumulator = (accumulator << 8) | byte;
		bits += 8;
		while (bits >= 11) {
			bits -= 11;
			int index = (accumulator >> bits) & 0x7FF;
			words.append(d->wordlist.at(index));
		}
	}

	sodium_memzero(entropy.data(), entropy.size());
	return words;
}

std::vector<uint8_t> Bip39::validateAndGetEntropy(const QStringList& words) const {
	if (words.size() != 12 && words.size() != 24) {
		throw std::invalid_argument("Mnemonic must have 12 or 24 words");
	}

	std::vector<int> indices;
	indices.reserve(words.size());
	for (const QString& w : words) {
		int idx = d->wordlist.indexOf(w.toLower().trimmed());
		if (idx < 0) {
			throw std::runtime_error("Unknown word in mnemonic: " + w.toStdString());
		}
		indices.push_back(idx);
	}

	std::vector<uint8_t> data;
	int bits = 0;
	uint32_t accumulator = 0;
	for (int idx : indices) {
		accumulator = (accumulator << 11) | idx;
		bits += 11;
		while (bits >= 8) {
			bits -= 8;
			data.push_back(static_cast<uint8_t>((accumulator >> bits) & 0xFF));
		}
	}

	int entropyBytes = data.size() - 1;
	std::vector<uint8_t> entropy(data.begin(), data.begin() + entropyBytes);

	QByteArray entropyBA = QByteArray::fromRawData(
		reinterpret_cast<const char*>(entropy.data()), entropy.size());
	QByteArray hash = QCryptographicHash::hash(entropyBA, QCryptographicHash::Sha256);

	uint8_t expectedChecksum = (hash[0] >> 4) & 0x0F;
	uint8_t actualChecksum = data.back() >> 4;

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

