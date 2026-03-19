#include "DataLayer/users/users_data_provider_json_impl.h"
#include "DataStream/format/json/data_reader.h"
#include "Resources/resources.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QUuid>

class UsersDataProviderJsonImpl::Private {
public:
	Private(UsersDataProviderJsonImpl* parent)
		: q(parent) {
		// Путь к данным пользователей
		dataPath = QDir::fromNativeSeparators(
			QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/users");
	}

	UsersDataProviderJsonImpl* q;
	QString dataPath;

	// Загрузить пользователя из JSON файла
	std::optional<UserData> loadUserFromFile(const QString& filePath) const {
		QFile file(filePath);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
			return std::nullopt;
		}

		QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
		if (doc.isNull()) {
			return std::nullopt;
		}

		return userFromJson(doc.object());
	}

	// Парсинг UserData из JSON
	UserData userFromJson(const QJsonObject& json) const {
		UserData user;
		user.id = json["id"].toString();
		user.login = json["login"].toString();
		user.passwordHash = json["passwordHash"].toString();
		user.displayName = json["displayName"].toString();

		// Метаданные
		if (json.contains("metadata")) {
			QJsonObject metaObj = json["metadata"].toObject();
			for (auto it = metaObj.begin(); it != metaObj.end(); ++it) {
				user.metadata[it.key()] = it.value().toString();
			}
		}

		return user;
	}

	// Сериализация UserData в JSON
	QJsonObject userToJson(const UserData& user) const {
		QJsonObject json;
		json["id"] = user.id;
		json["login"] = user.login;
		json["passwordHash"] = user.passwordHash;
		json["displayName"] = user.displayName;

		// Метаданные
		QJsonObject metaObj;
		for (auto it = user.metadata.begin(); it != user.metadata.end(); ++it) {
			metaObj[it.key()] = it.value();
		}
		json["metadata"] = metaObj;

		return json;
	}

	// Получить путь к файлу пользователя
	QString getUserFilePath(const QString& userId) const {
		return dataPath + "/" + userId + ".json";
	}
};

UsersDataProviderJsonImpl::UsersDataProviderJsonImpl(Resources* resources)
	: d(std::make_unique<Private>(this)) {
	Q_UNUSED(resources);
	// Создаём директорию, если не существует
	QDir().mkpath(d->dataPath);
}

UsersDataProviderJsonImpl::~UsersDataProviderJsonImpl() = default;

bool UsersDataProviderJsonImpl::loadAllUsers(QHash<QString, UserData>& users) const {
	users.clear();

	QDir dir(d->dataPath);
	if (!dir.exists()) {
		return true; // Нет директории - нет пользователей
	}

	dir.setNameFilters(QStringList() << "*.json");
	const auto files = dir.entryInfoList();

	for (const auto& fileInfo : files) {
		auto userOpt = d->loadUserFromFile(fileInfo.absoluteFilePath());
		if (userOpt.has_value()) {
			users[userOpt->id] = userOpt.value();
		}
	}

	return true;
}

std::optional<UserData> UsersDataProviderJsonImpl::loadUser(const QString& userId) const {
	QString filePath = d->getUserFilePath(userId);
	return d->loadUserFromFile(filePath);
}

bool UsersDataProviderJsonImpl::saveUser(const UserData& user) {
	QString filePath = d->getUserFilePath(user.id);

	QJsonObject json = d->userToJson(user);
	QJsonDocument doc(json);

	QFile file(filePath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		return false;
	}

	file.write(doc.toJson());
	return true;
}

bool UsersDataProviderJsonImpl::deleteUser(const QString& userId) {
	QString filePath = d->getUserFilePath(userId);
	return QFile::remove(filePath);
}

std::optional<UserData> UsersDataProviderJsonImpl::findUserByLogin(const QString& login) const {
	QHash<QString, UserData> users;
	if (!loadAllUsers(users)) {
		return std::nullopt;
	}

	for (const auto& user : users) {
		if (user.login == login) {
			return user;
		}
	}

	return std::nullopt;
}
