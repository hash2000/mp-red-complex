#include "Libs/Resources/resources.h"

#include <QStandardPaths>

class Resources::Private {
public:
	Private(Resources* parent) : q(parent) { }
	Resources* q;

	VariablesContext defaultsVariables;
	DirectoriesContext defaultsDirectories;
	std::list<std::unique_ptr<DataStreamContainer>> resources;

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

	qDebug() << "Load resources Success";
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
	q->Directories.set("path.databases.settings.file", resourcesPath.value().filePath("data/databases.json"));
	q->Directories.set("path.auth.words", resourcesPath.value().filePath("data/recovery/english.txt"));
	q->Directories.set("path.users", resourcesPath.value().filePath("users"));
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

	auto userBasePathOpt = createProfilePath(userHash);
	if (!userBasePathOpt) {
		return false;
	}

	auto userBasePath = userBasePathOpt.value();
	//Directories.set("path.users.current.file", userBasePath);
	//Directories.set("path.auth.current.file", userBasePath.filePath("auth.enc"));
	//Directories.set(DirectoryPath::MessangerDbFile, userBasePath.filePath("data/messanger.db"));
	//Directories.set(DirectoryPath::GameDbFile, userBasePath.filePath("data/game.db"));

	return true;
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
