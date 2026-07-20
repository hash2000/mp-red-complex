#include "Content/UsersModule/services/users_registry_service.h"
#include "Content/UsersModule/data_providers/register_user/i_register_user_data_provider.h"

#include "Libs/Base/crypto/bip_39.h"
#include "Libs/Resources/resources.h"

#include <QJsonObject>
#include <QCryptographicHash>
#include <QDir>
#include <QString>

#include <sodium.h>

class UsersRegistryService::Private {
public:
	Private(UsersRegistryService* parent) : q(parent) {}
	UsersRegistryService* q;

	IRegisterUserDataProvider* registerUserDataProvider;
	Resources* resources;

	static QString computeUserHash(const QByteArray& masterSeed);
	static QByteArray deriveEncryptionKey(const QString& password, const QByteArray& salt);
	static QByteArray encryptSeedPhrase(const QString& seedPhrase, const QByteArray& encKey);
	static QByteArray deriveDatabaseKey(const QByteArray& masterSeed);

	static bool saveEncryptedSeed(const QDir& profilePath, const QString& userHash, const QByteArray& salt, const QByteArray& encryptedSeed);
};

UsersRegistryService::UsersRegistryService(
	Resources* resources,
	IRegisterUserDataProvider* registerUserDataProvider,
	QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->registerUserDataProvider = registerUserDataProvider;
	d->resources = resources;
}

UsersRegistryService::~UsersRegistryService() = default;

bool UsersRegistryService::registerUser(const QString& username, const QString& password, QStringList& outSeedPhrase) {
	try {
		const QString existingHash = d->registerUserDataProvider->getUserHashByUsername(username);
		if (!existingHash.isEmpty()) {
			throw std::runtime_error("Username already taken.");
		}

		Bip39 bip39(d->resources);

		// 1. Генерируем seed-фразу
		outSeedPhrase = bip39.generateMnemonic(128);

		// 2. Получаем мастер - сид
		QByteArray masterSeed = bip39.mnemonicToSeed(outSeedPhrase);

		// 3. Вычисляем user_hash
		QString userHash = Private::computeUserHash(masterSeed);

		auto profile_path = d->resources->createProfilePath(userHash);
		if (!profile_path) {
			throw std::runtime_error("Can't create user profile path.");
		}

		// 4. Шифруем и сохраняем seed-фразу
		std::vector<uint8_t> salt(crypto_pwhash_SALTBYTES);
		randombytes_buf(salt.data(), salt.size());
		QByteArray saltBA = QByteArray::fromRawData(reinterpret_cast<const char*>(salt.data()), salt.size());

		QByteArray encKey = Private::deriveEncryptionKey(password, saltBA);
		QString seedPhraseStr = outSeedPhrase.join(' ');
		QByteArray encryptedSeed = Private::encryptSeedPhrase(seedPhraseStr, encKey);

		if (!Private::saveEncryptedSeed(profile_path.value().absolutePath(), userHash, saltBA, encryptedSeed)) {
			throw std::runtime_error("Can't save auser secrets.");
		}

		// 5. Выводим ключ для БД
		QByteArray dbKey = Private::deriveDatabaseKey(masterSeed);

		// 6. Добавляем в accounts.db (открытый реестр)
		d->registerUserDataProvider->addUser(userHash, username);

		// 7. Очищаем
		sodium_memzero(masterSeed.data(), masterSeed.size());
		sodium_memzero(encKey.data(), encKey.size());
		sodium_memzero(dbKey.data(), dbKey.size());
	}
	catch (const std::exception& e) {
		qCritical() << "Registration failed:" << e.what();
		return false;
	}

	return true;
}

// Вычисляем user_hash из мастер-сида (SHA-256)
QString UsersRegistryService::Private::computeUserHash(const QByteArray& masterSeed) {
	QByteArray hash = QCryptographicHash::hash(masterSeed, QCryptographicHash::Sha256);
	return hash.toHex();
}

QByteArray UsersRegistryService::Private::deriveEncryptionKey(const QString& password, const QByteArray& salt) {
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

QByteArray  UsersRegistryService::Private::encryptSeedPhrase(const QString& seedPhrase, const QByteArray& encKey) {
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
QByteArray UsersRegistryService::Private::deriveDatabaseKey(const QByteArray& masterSeed) {
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

bool UsersRegistryService::Private::saveEncryptedSeed(
	const QDir& profilePath,
	const QString& userHash,
	const QByteArray& salt,
	const QByteArray& encryptedSeed) {
	QFile file(profilePath.filePath("auth.enc"));
	if (!file.open(QIODevice::WriteOnly)) {
		return false;
	}

	// Формат файла: [salt (16 байт)] [encrypted_seed]
	file.write(salt);
	file.write(encryptedSeed);
	file.close();

	return true;
}
