#include "Libs/Resources/resources.h"
#include "Libs/Resources/profiles/profile_name.h"

#include <QStandardPaths>

class Resources::Private {
public:
	Private(Resources* parent) : q(parent) { }
	Resources* q;

	VariablesContext defaultsVariables;
	DirectoriesContext defaultsDirectories;
	std::list<std::unique_ptr<DataStreamContainer>> resources;
	QString currentProfileName;
	bool allowEncryptedStores = true;

	bool loadDefaults();
};

Resources::Resources()
	: d(std::make_unique<Private>(this)) {
}

Resources::~Resources() = default;

Resources::ItemsCollection Resources::items() const {
	return make_deref_view(d->resources);
}

bool Resources::load() {
	qDebug() << "Load resources";
	const auto bootstrapDir = QDir::current();
	const auto bootstrapPath = bootstrapDir.path();
	const auto configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);

	qDebug() << "Bootstrap path:" << bootstrapPath;
	d->defaultsDirectories.set("path.bootstrap", bootstrapPath);

	qDebug() << "Profile path:" << configPath;
	d->defaultsDirectories.set("path.profile", configPath);

	if (!d->loadDefaults()) {
		qDebug() << "Load resources Failed";
		return false;
	}

	// Неавторизованный пользователь работает с профилем default: его каталог
	// создаётся при первом запуске, а настройки заполняет BootstrapManager.
	if (!selectProfile(Profiles::defaultName())) {
		qDebug() << "Load resources Failed";
		return false;
	}

	qDebug() << "Load resources Success";
	return true;
}

QString Resources::currentProfileName() const {
	return d->currentProfileName;
}

std::optional<QDir> Resources::currentProfilePath() const {
	return Directories.get("path.profiles.current");
}

bool Resources::allowsEncryptedStores() const {
	return d->allowEncryptedStores;
}

bool Resources::selectProfile(const QString& name) {
	const auto profilesPath = Directories.get("path.profiles");
	if (!profilesPath) {
		qCritical() << "Profiles path is not set.";
		return false;
	}

	const auto normalized = name.trimmed().toLower();
	if (!Profiles::isValidName(normalized)) {
		qCritical() << "Invalid profile name:" << name;
		return false;
	}

	const QDir profilePath = profilesPath->filePath(normalized);
	if (!QDir().mkpath(profilePath.absolutePath())) {
		qCritical() << "Can't create profile path:" << profilePath.absolutePath();
		return false;
	}

	d->currentProfileName = normalized;
	d->allowEncryptedStores = normalized != Profiles::defaultName();
	Directories.set("path.profiles.current", profilePath);
	qDebug() << "Current profile:" << normalized << "path:" << profilePath.absolutePath()
		<< "encrypted stores:" << d->allowEncryptedStores;
	return true;
}

QString Resources::defaultUseerName() const {
	return "Deafult";
}

bool Resources::Private::loadDefaults() {
	q->Directories.apply(defaultsDirectories);
	q->Variables.apply(defaultsVariables);

	auto resourcesPath = defaultsDirectories.get("path.profile");
	if (!resourcesPath) {
		qCritical() << "Resources.Path is not set.";
		return false;
	}

	q->Directories.set("path.tools", resourcesPath.value().filePath("tools"));
	q->Directories.set("path.databases", resourcesPath.value().filePath("data"));
	q->Directories.set("path.auth.words", resourcesPath.value().filePath("data/recovery/english.txt"));
	q->Directories.set("path.users", resourcesPath.value().filePath("users"));
	q->Directories.set("path.profiles", resourcesPath.value().filePath("profiles"));
	q->Directories.set("path.databases.accounts.file", resourcesPath.value().filePath("data/accounts.db"));
	q->Directories.set("path.databases.fetch_api.file", resourcesPath.value().filePath("data/fetch_api.db"));
	return true;
}

bool Resources::loadProfile(const QString& userHash) {

	Directories.clear();
	Variables.clear();

	if (!d->loadDefaults()) {
		return false;
	}

	// Каждому пользователю соответствует профиль с настройками: вход в систему
	// переключает его на профиль с именем user hash, выход — на default.
	const auto profileName = userHash.isEmpty() ? Profiles::defaultName() : userHash;
	if (!selectProfile(profileName)) {
		return false;
	}

	if (userHash.isEmpty()) {
		return true;
	}

	auto userBasePathOpt = createProfilePath(userHash);
	return userBasePathOpt.has_value();
}

std::optional<QDir> Resources::createProfilePath(const QString& userHash, bool checkOnly) {
	auto usersPath = Directories.get("path.users");
	if (!usersPath) {
		qCritical() << "Users.Path is not set.";
		return std::nullopt;
	}

	auto user = userHash;
	if (user.isEmpty()) {
		qCritical() << "User hash is empty.";
		return std::nullopt;
	}

	QDir userBasePath = usersPath.value().filePath(userHash);
	if (checkOnly) {
		if (!QDir().exists(userBasePath.absolutePath())) {
			qCritical() << "Can't search user profile path.";
			return std::nullopt;
		}
	}
	else {
		if (!QDir().mkpath(userBasePath.absolutePath()) ||
			!QDir().mkpath(userBasePath.filePath("data"))) {
			qCritical() << "Can't create user profile path.";
			return std::nullopt;
		}
	}

	return userBasePath;
}
