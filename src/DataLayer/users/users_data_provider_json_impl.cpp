#include "DataLayer/users/users_data_provider_json_impl.h"
#include "DataLayer/users/user.h"
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
	}

	UsersDataProviderJsonImpl* q;
	Resources* resources;

	// Загрузить пользователя из JSON файла
	std::optional<UserData> loadUserFromFile(const QString& filePath) const {
		QJsonObject json;
		Format::Json::DataReader reader(resources, "data", filePath);
		if (!reader.read(json)) {
			return std::nullopt;
		}

		return userFromJson(json);
	}

	// Парсинг UserData из JSON
	UserData userFromJson(const QJsonObject& json) const {
		UserData user;
		user.loginHash = json["login"].toString();
		user.passwordHash = json["passwordHash"].toString();
		user.displayName = json["displayName"].toString();
		user.chestId = QUuid::fromString(json["chestId"].toString());

		// Метаданные
		if (json.contains("metadata")) {
			QJsonObject metaObj = json["metadata"].toObject();
			for (auto it = metaObj.begin(); it != metaObj.end(); it++) {
				user.metadata[it.key()] = it.value().toString();
			}
		}

		if (json.contains("characters")) {
			QJsonArray chars = json["characters"].toArray();
			for (auto it = chars.begin(); it != chars.end(); it++) {
				user.characters.push_back(QUuid::fromString(it->toString()));
			}
		}

		return user;
	}

	// Сериализация UserData в JSON
	QJsonObject userToJson(const UserData& user) const {
		QJsonObject json;
		json["login"] = user.loginHash;
		json["passwordHash"] = user.passwordHash;
		json["displayName"] = user.displayName;
		json["chestId"] = user.chestId
			.toString(QUuid::StringFormat::WithoutBraces);

		// Метаданные
		QJsonObject metaObj;
		for (auto it = user.metadata.begin(); it != user.metadata.end(); it++) {
			metaObj[it.key()] = it.value();
		}
		json["metadata"] = metaObj;

		QJsonArray chars;
		for (auto it = user.characters.begin(); it != user.characters.end(); it++) {
			chars.push_back(it->toString(QUuid::StringFormat::WithoutBraces));
		}
		json["characters"] = chars;

		return json;
	}

	// Получить путь к файлу пользователя
	QString getUserFilePath(const QString& loginHash) const {
		const auto path = QString("users/%1.json")
			.arg(loginHash.toLower());

		return path;
	}
};

UsersDataProviderJsonImpl::UsersDataProviderJsonImpl(Resources* resources)
: d(std::make_unique<Private>(this)) {
	d->resources = resources;
}

UsersDataProviderJsonImpl::~UsersDataProviderJsonImpl() = default;

std::optional<UserData> UsersDataProviderJsonImpl::loadUser(const QString& loginHash) const {
	QString filePath = d->getUserFilePath(loginHash);
	return d->loadUserFromFile(filePath);
}

bool UsersDataProviderJsonImpl::saveUser(const UserData& user) {
	QString path = d->getUserFilePath(user.loginHash);
	auto dataDir = d->resources->Variables.get("Resources.Path", "")
		.toString();
	QDir dir(dataDir);
	if (!dir.exists(path)) {
		dir.mkpath(path);
	}

	QJsonObject json = d->userToJson(user);
	QJsonDocument doc(json);

	const auto filePath = dir.filePath("data/" + path);
	QFile file(filePath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		return false;
	}

	file.write(doc.toJson());
	return true;
}

bool UsersDataProviderJsonImpl::deleteUser(const QString& loginHash) {
	QString filePath = d->getUserFilePath(loginHash);
	return QFile::remove(filePath);
}
