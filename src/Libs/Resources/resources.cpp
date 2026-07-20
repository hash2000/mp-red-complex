#include "Libs/Resources/resources.h"
#include "Libs/DataStream/data_stream/dat/dat_file.h"
#include "Libs/DataStream/data_stream/raw/raw_directory.h"
#include "Libs/DataStream/data_write_stream/data_write_stream_file.h"
#include "Libs/Base/scoped_timer.h"

Resources::Resources() {
}

void Resources::configure(const Config* config) {
	const auto resources_path = QDir(config->resources_path);
	_defaultsVariables.set("Resources.Path", resources_path.absolutePath());
	_defaultsVariables.set("Resources.Tools.Formatters.Path", QDir(config->resources_formatters_path).absolutePath());
	_defaultsVariables.set("Resources.Tools.Formatter.Executable", QDir(config->resources_formatter_executable).absolutePath());
	_defaultsDirectories.set(DirectoryPath::ResourcesPath, resources_path);
}

bool Resources::load() {
	//loadDatResources();
	loadRawResources();
	return false;
}

QString Resources::defaultUseerName() const {
	return "Deafult";
}

void Resources::loadDatFile(const QString& fileName) {
	ScopedTimer watch(QString("Load %1 file.").arg(fileName));
	auto resources_path = _defaultsDirectories.get(DirectoryPath::ResourcesPath);
	if (!resources_path) {
		qCritical() << "Resources.Path is not set.";
		return;
	}

	QFileInfo file(resources_path.value().filePath(fileName));
	if(!file.exists()) {
		qWarning() << file.absoluteFilePath() << "not found";
		return;
	}

	for(const auto& res : _resources) {
		if (res->name() == fileName) {
			watch.error(QString("%1 allready loaded").arg(fileName));
			return;
		}
	}

	try {
		auto item = std::make_unique<DatFile>();
		item->name(fileName);
		item->loadFromFile(file.absoluteFilePath());
		_resources.push_back(std::move(item));
	}
	catch (std::exception &ex) {
		watch.error(ex.what());
		return;
	}
}

void Resources::loadDatResources() {
	auto resources_path = _defaultsDirectories.get(DirectoryPath::ResourcesPath);
	if (!resources_path) {
		qCritical() << "Resources.Path is not set.";
		return;
	}

	qDebug() << "Resources::loadDatResources from" << resources_path.value().absolutePath();
	const auto entries = resources_path.value().entryInfoList(QDir::Files);

	for(const auto &entry: entries) {
		const auto fileName = entry.fileName();
		const auto suffix = entry.suffix().toLower();

		if (suffix == "dat") {
			loadDatFile(fileName);
		}
	}
}

void Resources::loadRawResources() {
	auto resources_path = _defaultsDirectories.get(DirectoryPath::ResourcesPath);
	if (!resources_path) {
		qCritical() << "Resources.Path is not set.";
		return;
	}

	qDebug() << "Resources::loadRawResources from" << resources_path.value().absolutePath();
	const auto entries = resources_path.value().entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
	for(const auto &entry: entries) {
		const auto path = entry.absoluteFilePath();
		const auto fileName = entry.fileName();

		ScopedTimer watch("Load raw resources.");

		try {
			auto item = std::make_unique<RawDirectory>();
			item->name(fileName);
			item->loadFromPath(path);
			_resources.push_back(std::move(item));
		}
		catch (std::exception &ex) {
			watch.error(QString("%1 exception: %2")
				.arg(fileName)
				.arg(ex.what()));
			return;
		}
	}
}

auto Resources::getStream(const QString &container, const QString &path) const
	-> std::optional<std::shared_ptr<DataStream>> {
	for (const auto& res : _resources) {
		if (res->name() == container) {
			return res->find(path);
		}
	}

	return std::nullopt;
}

auto Resources::getWriteStream(const QString& container, const QString& path) const
	-> std::optional<std::shared_ptr<DataWriteStream>> {
	//const auto dataPath = QString("")
	//	.arg(_resources_path.absolutePath())
	//	.arg(container)
	//	.arg(path);
	auto stream = std::make_shared<DataWriteStreamFile>("");


	return stream;
}

bool Resources::loadDefaults() {
	Directories.apply(_defaultsDirectories);
	Variables.apply(_defaultsVariables);

	auto resources_path = _defaultsDirectories.get(DirectoryPath::ResourcesPath);
	if (!resources_path) {
		qCritical() << "Resources.Path is not set.";
		return false;
	}

	Directories.set(DirectoryPath::AssetsPath, resources_path.value().filePath("assets"));
	Directories.set(DirectoryPath::DataPath, resources_path.value().filePath("data"));
	Directories.set(DirectoryPath::RecoveryWordsFile, resources_path.value().filePath("data/recovery/english.txt"));
	Directories.set(DirectoryPath::DatabasesConfigFile, resources_path.value().filePath("data/databases.json"));
	Directories.set(DirectoryPath::UsersPath, resources_path.value().filePath("users"));
	Directories.set(DirectoryPath::AccountsDbFile, resources_path.value().filePath("data/accounts.db"));
	return true;
}

bool Resources::loadProfile(const QString& userHash) {

	Directories.clear();
	Variables.clear();

	if (!loadDefaults()) {
		return false;
	}

	auto user_base_pathOpt = createProfilePath(userHash);
	if (!user_base_pathOpt) {
		return false;
	}

	auto user_base_path = user_base_pathOpt.value();
	Directories.set(DirectoryPath::CurrentUserPath, user_base_path);
	Directories.set(DirectoryPath::AutchFile, user_base_path.filePath("auth.enc"));
	Directories.set(DirectoryPath::UsersDbFile, user_base_path.filePath("users.db"));
	Directories.set(DirectoryPath::GameDbFile, user_base_path.filePath("game.db"));
	Directories.set(DirectoryPath::MessangerDbFile, user_base_path.filePath("messanger.db"));

	return true;
}

std::optional<QDir> Resources::createProfilePath(const QString& userHash, bool checkOnly) {
	auto users_path = Directories.get(DirectoryPath::UsersPath);
	if (!users_path) {
		qCritical() << "Users.Path is not set.";
		return std::nullopt;
	}

	auto user = userHash;
	if (user.isEmpty()) {
		qCritical() << "User hash is empty.";
		return std::nullopt;
	}

	QDir user_base_path = users_path.value().filePath(userHash);
	if (checkOnly) {
		if (!QDir().exists(user_base_path.absolutePath())) {
			qCritical() << "Can't search user profile path.";
			return std::nullopt;
		}
	}
	else {
		if (!QDir().mkpath(user_base_path.absolutePath())) {
			qCritical() << "Can't create user profile path.";
			return std::nullopt;
		}
	}

	return user_base_path;
}
