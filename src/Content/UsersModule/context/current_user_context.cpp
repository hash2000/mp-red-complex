#include "Content/UsersModule/context/current_user_context.h"
#include "Libs/Resources/resources.h"

#include <QMutex>
#include <QDir>

//#include <sodium.h>

class CurrentUserContext::Private {
public:
	Private(CurrentUserContext* parent) : q(parent) {}
	CurrentUserContext* q;

	Resources* resources;
	mutable QMutex mutex;
	QString currentUserHash;
	QByteArray encryptionKey;  // Ключ для SQLCipher (32 байта)
	QString baseDataDir;       // ~/.yourapp/users
};

CurrentUserContext::CurrentUserContext(Resources* resources)
	: d(std::make_unique<Private>(this)) {
	d->resources = resources;
}

CurrentUserContext::~CurrentUserContext() = default;

std::pair<QString, QString> CurrentUserContext::generateUserPath(Resources* resources, const QString& userHash) {
	const auto path = resources->Variables.get("Resources.Path", "").toString();
		// Создаём директорию пользователя, если её нет
	const auto baseDataDir = path + "/users";
	QString userDir = baseDataDir + "/" + userHash;
	return { baseDataDir, userDir };
}

void CurrentUserContext::setCurrentUser(const QString& userHash, const QByteArray& encryptionKey) {

	QMutexLocker lock(&d->mutex);
	d->currentUserHash = userHash;
	d->encryptionKey = encryptionKey;

	const auto [baseDir, userDir] = generateUserPath(d->resources, userHash);

	d->baseDataDir = baseDir;
	d->resources->Variables.set("Users.CurrentUser.Identity.Name", "guest");
	d->resources->Variables.set("Users.CurrentUser.Profile.Path", userDir);
	QDir().mkpath(userDir);
}

void CurrentUserContext::clearCurrentUser() {
	QMutexLocker lock(&d->mutex);
	// Затираем ключ из памяти
	//sodium_memzero(d->encryptionKey.data(), d->encryptionKey.size());
	d->currentUserHash.clear();
	d->encryptionKey.clear();
	d->resources->Variables.set("Users.CurrentUser.Identity.Name", "");
	d->resources->Variables.set("Users.CurrentUser.Profile.Path", "");

}

QString CurrentUserContext::currentUserHash() const {
	QMutexLocker lock(&d->mutex);
	return d->currentUserHash;
}

QByteArray CurrentUserContext::currentUserEncryptionKey() const {
	QMutexLocker lock(&d->mutex);
	return d->encryptionKey;
}

QString CurrentUserContext::currentUserDataDir() const {
	QMutexLocker lock(&d->mutex);
	if (d->currentUserHash.isEmpty()) {
		return { };
	}
	return d->baseDataDir + "/" + d->currentUserHash;
}


bool CurrentUserContext::isLoggedIn() const {
	QMutexLocker lock(&d->mutex);
	return !d->currentUserHash.isEmpty();
}
