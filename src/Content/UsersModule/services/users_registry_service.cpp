#include "Content/UsersModule/services/users_registry_service.h"
#include "Content/UsersModule/data_providers/users_registry/i_users_registry_data_provider.h"
#include "Content/UsersModule/extensions/crypto_extensions.h"

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

	IUsersRegistryDataProvider* usersRegistryDataProvider;
	Resources* resources;

	static bool saveEncryptedSeed(const QDir& profilePath, const QString& userHash, const QByteArray& salt, const QByteArray& encryptedSeed);
};

UsersRegistryService::UsersRegistryService(
	Resources* resources,
	IUsersRegistryDataProvider* usersRegistryDataProvider,
	QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->usersRegistryDataProvider = usersRegistryDataProvider;
	d->resources = resources;
}

UsersRegistryService::~UsersRegistryService() = default;

bool UsersRegistryService::registerUser(const QString& username, const QString& password, QStringList& outSeedPhrase) {
	using namespace Extensions::Users::Crypto;

	try {
		const QString existingHash = d->usersRegistryDataProvider->getUserHashByUsername(username);
		if (!existingHash.isEmpty()) {
			throw std::runtime_error("Username already taken.");
		}

		Bip39 bip39(d->resources);

		// 1. Генерируем seed-фразу
		outSeedPhrase = bip39.generateMnemonic(128);

		// 2. Получаем мастер - сид
		QByteArray masterSeed = bip39.mnemonicToSeed(outSeedPhrase);

		// 3. Вычисляем user_hash
		QString userHash = computeUserHash(masterSeed);

		auto profile_path = d->resources->createProfilePath(userHash, false);
		if (!profile_path) {
			throw std::runtime_error("Can't create user profile path.");
		}

		// 4. Шифруем и сохраняем seed-фразу
		std::vector<uint8_t> salt(crypto_pwhash_SALTBYTES);
		randombytes_buf(salt.data(), salt.size());
		QByteArray saltBA = QByteArray::fromRawData(reinterpret_cast<const char*>(salt.data()), salt.size());

		QByteArray encKey = deriveEncryptionKey(password, saltBA);
		QString seedPhraseStr = outSeedPhrase.join(' ');
		QByteArray encryptedSeed = encryptSeedPhrase(seedPhraseStr, encKey);

		if (!Private::saveEncryptedSeed(profile_path.value().absolutePath(), userHash, saltBA, encryptedSeed)) {
			throw std::runtime_error("Can't save auser secrets.");
		}

		// 5. Добавляем в accounts.db (открытый реестр)
		d->usersRegistryDataProvider->addUser(userHash, username);

		// 6. Очищаем
		sodium_memzero(masterSeed.data(), masterSeed.size());
		sodium_memzero(encKey.data(), encKey.size());
	}
	catch (const std::exception& e) {
		qCritical() << "Registration failed:" << e.what();
		return false;
	}

	return true;
}

bool UsersRegistryService::restoreFromSeed(const QStringList& seedPhrase, const QString& newPassword) {
	using namespace Extensions::Users::Crypto;

	try {
		Bip39 bip39(d->resources);

		// 1. Валидируем seed-фразу и получаем мастер-сид
		QByteArray masterSeed = bip39.mnemonicToSeed(seedPhrase);

		// 2. Вычисляем user_hash
		QString userHash = computeUserHash(masterSeed);

		auto profile_path = d->resources->createProfilePath(userHash, false);
		if (!profile_path) {
			throw std::runtime_error("Can't create user profile path.");
		}

		// 3. Генерируем новую соль
		std::vector<uint8_t> salt(crypto_pwhash_SALTBYTES);
		randombytes_buf(salt.data(), salt.size());
		QByteArray saltBA = QByteArray::fromRawData(reinterpret_cast<const char*>(salt.data()), salt.size());

		// 4. Выводим ключ шифрования из нового пароля
		QByteArray encKey = deriveEncryptionKey(newPassword, saltBA);

		// 5. Шифруем seed-фразу
		QString seedPhraseStr = seedPhrase.join(' ');
		QByteArray encryptedSeed = encryptSeedPhrase(seedPhraseStr, encKey);

		// 6. Сохраняем (перезаписываем auth.enc)
		if (!Private::saveEncryptedSeed(profile_path.value().absolutePath(), userHash, saltBA, encryptedSeed)) {
			sodium_memzero(masterSeed.data(), masterSeed.size());
			sodium_memzero(encKey.data(), encKey.size());
			throw std::runtime_error("Failed to save auth file");
		}

		// 7. Очищаем
		sodium_memzero(masterSeed.data(), masterSeed.size());
		sodium_memzero(encKey.data(), encKey.size());

		qDebug() << "User restored successfully from seed. Hash:" << userHash;
	}
	catch (const std::exception& e) {
		qCritical() << "Restore failed:" << e.what();
		return false;
	}

	return true;
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

