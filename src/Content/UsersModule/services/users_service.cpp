#include "Content/UsersModule/services/users_service.h"
#include "Content/UsersModule/data_providers/users_registry/i_users_registry_data_provider.h"
#include "Content/UsersModule/models/user_view.h"
#include "Content/DatabaseModule/services/databases_service.h"
#include "Content/TexturesModule/services/images_service.h"
#include "Content/UsersModule/extensions/crypto_extensions.h"

#include "Libs/Base/crypto/bip_39.h"
#include "Libs/Resources/resources.h"

#include <QCryptographicHash>
#include <QByteArray>
#include <QFile>
#include <QCryptographicHash>

#include <sodium.h>

class UsersService::Private {
public:
	Private(UsersService* parent) : q(parent) { }
	UsersService* q;

	IUsersRegistryDataProvider* usersRegistryDataProvider = nullptr;
	ImagesService* imagesService = nullptr;
	DatabasesService* databasesService = nullptr;
	Resources* resources;
	QString currentUserHash;
	QByteArray encryptedSeed;

	// Читаем зашифрованную seed-фразу из файла
	static bool loadEncryptedSeed(
		const QDir& profilePath,
		const QString& userHash,
		QByteArray& outSalt,
		QByteArray& outEncryptedSeed);
};

UsersService::UsersService(
	Resources* resources,
	DatabasesService* databasesService,
	IUsersRegistryDataProvider* usersRegistryDataProvider,
	ImagesService* imagesService,
	QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->resources = resources;
	d->usersRegistryDataProvider = usersRegistryDataProvider;
	d->databasesService = databasesService;
	d->imagesService = imagesService;
}

UsersService::~UsersService() = default;

bool UsersService::login(const QString& username, const QString& password) {
	using namespace Extensions::Users::Crypto;

	if (isAuthenticated()) {
		logout();
	}

	try {
		const QString userHash = d->usersRegistryDataProvider->getUserHashByUsername(username);
		if (userHash.isEmpty()) {
			throw std::runtime_error("Username is not registered.");
		}

		auto profile_path = d->resources->createProfilePath(userHash, true);
		if (!profile_path) {
			throw std::runtime_error("Can't open user profile path.");
		}

		// 1. Читаем зашифрованную seed-фразу из файла
		QByteArray salt, encryptedSeed;
		if (!Private::loadEncryptedSeed(profile_path.value(), userHash, salt, encryptedSeed)) {
			throw std::runtime_error("User not found or auth file corrupted");
		}

		// 2. Выводим ключ шифрования из пароля
		QByteArray encKey = deriveEncryptionKey(password, salt);

		// 3. Расшифровываем seed-фразу
		QString seedPhraseStr;
		try {
			seedPhraseStr = decryptSeedPhrase(encryptedSeed, encKey);
		}
		catch (const std::exception& e) {
			sodium_memzero(encKey.data(), encKey.size());
			throw std::runtime_error("Invalid password");
		}

		// 4. Парсим seed-фразу
		QStringList seedPhrase = seedPhraseStr.split(' ', Qt::SkipEmptyParts);

		// 5. Получаем мастер-сид из seed-фразы
		Bip39 bip39(d->resources);
		QByteArray masterSeed = bip39.mnemonicToSeed(seedPhrase);

		// 6. Проверяем, что user_hash совпадает
		QString computedHash = computeUserHash(masterSeed);
		if (computedHash != userHash) {
			sodium_memzero(masterSeed.data(), masterSeed.size());
			sodium_memzero(encKey.data(), encKey.size());
			throw std::runtime_error("User hash mismatch (data corruption?)");
		}

		QByteArray dbKey;
		try {
			// 7. Выводим ключ для БД
			dbKey = deriveDatabaseKey(masterSeed);
		}
		catch (const std::exception& e) {
			sodium_memzero(masterSeed.data(), masterSeed.size());
			sodium_memzero(encKey.data(), encKey.size());
			throw std::runtime_error("Invalid master seed");
		}

		// 8. Устанавливаем контекст пользователя
		d->currentUserHash = userHash;
		d->resources->loadProfile(userHash);
		d->databasesService->setEncryptionKey(dbKey);
		d->databasesService->reloadAliases();
		d->encryptedSeed = encryptedSeed;

		// 9. Очищаем чувствительные данные
		sodium_memzero(masterSeed.data(), masterSeed.size());
		sodium_memzero(encKey.data(), encKey.size());
		sodium_memzero(dbKey.data(), dbKey.size());

		qDebug() << "User logged in successfully. Hash:" << userHash;
	}
	catch (const std::exception& e) {
		qCritical() << "Login failed:" << e.what();
		return false;
	}

	return true;
}

void UsersService::logout() {
	d->encryptedSeed.clear();
	d->currentUserHash.clear();
	d->resources->loadProfile();
	d->databasesService->cleadEncryptionKey();
	d->databasesService->reloadAliases();

	emit loggedOut();
}

bool UsersService::isAuthenticated() const {
	return !d->currentUserHash.isEmpty();
}

// Читаем зашифрованную seed-фразу из файла
bool UsersService::Private::loadEncryptedSeed(
	const QDir& profilePath,
	const QString& userHash,
	QByteArray& outSalt,
	QByteArray& outEncryptedSeed) {

	const auto path = profilePath.filePath("auth.enc");
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		return false;
	}

	QByteArray fileData = file.readAll();
	file.close();

	int saltSize = crypto_pwhash_SALTBYTES;
	if (fileData.size() < saltSize) {
		return false;
	}

	outSalt = fileData.left(saltSize);
	outEncryptedSeed = fileData.mid(saltSize);

	return true;
}

QString UsersService::currentUserHash() {
	return d->currentUserHash;
}

QByteArray UsersService::currentUserEncryptedSeed() {
	return d->encryptedSeed;
}
