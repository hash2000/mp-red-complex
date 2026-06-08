#include "Content/DatabaseModule/data_providers/databases_settings_data_provider_json_impl.h"
#include "Libs/DataStream/format/json/data_reader.h"
#include "Libs/Resources/resources.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QReadWriteLock>

class DatabaseSettingsDataProviderJsonImpl::Private {
public:
	Private(DatabaseSettingsDataProviderJsonImpl* parent) : q(parent) {}
	DatabaseSettingsDataProviderJsonImpl* q;

	Resources* resources;
	mutable QJsonObject cachedJson;
	mutable QDateTime lastModified;
	mutable QReadWriteLock cacheLock;

	bool loadFromFile() const;
	Settings parseSettings(const QJsonObject& json) const;
	QJsonValue findInObject(const QJsonObject& obj, const QStringList& path, int depth = 0) const;
	std::optional<Settings> findSettings(const QString& name) const;
};

DatabaseSettingsDataProviderJsonImpl::DatabaseSettingsDataProviderJsonImpl(Resources* resources)
	: d(std::make_unique<Private>(this)) {
	d->resources = resources;
}

DatabaseSettingsDataProviderJsonImpl::~DatabaseSettingsDataProviderJsonImpl() = default;

bool DatabaseSettingsDataProviderJsonImpl::Private::loadFromFile() const {
	const QString path = "assets/databases.json";
	QWriteLocker locker(&cacheLock);
	QFileInfo fileInfo(path);

	// Проверяем, изменился ли файл
	if (lastModified.isValid() && fileInfo.lastModified() <= lastModified && !cachedJson.isEmpty()) {
		qDebug() << "Config file allready in cache:" << path;
		return true;
	}

	QJsonObject json;
	Format::Json::DataReader reader(resources, "data", path);
	if (!reader.read(json)) {
		return false;
	}

	cachedJson = json;
	lastModified = fileInfo.lastModified();
	qDebug() << "Config file loaded and cached:" << path;
	return true;
}

DatabaseSettingsDataProviderJsonImpl::Settings DatabaseSettingsDataProviderJsonImpl::Private::parseSettings(const QJsonObject& json) const {
	Settings settings;

	// Безопасное чтение с проверкой типов
	auto readInt = [&json](const QString& key, int defaultValue) -> int {
		auto it = json.constFind(key);
		if (it != json.constEnd()) {
			QJsonValue val = it.value();
			if (val.isDouble()) {
				return val.toInt();
			}
			else if (val.isString()) {
				// Поддержка строковых чисел (на всякий случай)
				bool ok;
				int result = val.toString().toInt(&ok);
				if (ok) {
					return result;
				}
			}
		}
		return defaultValue;
	};

	settings.walCheckInterval = readInt("checkInterval", settings.walCheckInterval);
	settings.walMaxSizeMb = readInt("maxSizeMb", settings.walMaxSizeMb);

	return settings;
}

QJsonValue DatabaseSettingsDataProviderJsonImpl::Private::findInObject(const QJsonObject& obj, const QStringList& path, int depth) const {
	if (depth >= path.size()) {
		return QJsonValue::Undefined;
	}

	const QString& key = path[depth];

	// Итеративный поиск (быстрее чем рекурсия для простых путей)
	if (depth == path.size() - 1) {
		// Последний элемент пути - ищем ключ напрямую
		auto it = obj.constFind(key);
		if (it != obj.constEnd()) {
			return it.value();
		}
		return QJsonValue::Undefined;
	}

	// Промежуточный элемент - ищем во вложенном объекте
	auto it = obj.constFind(key);
	if (it != obj.constEnd() && it.value().isObject()) {
		return findInObject(it.value().toObject(), path, depth + 1);
	}

	return QJsonValue::Undefined;
}

std::optional<DatabaseSettingsDataProviderJsonImpl::Settings> DatabaseSettingsDataProviderJsonImpl::Private::findSettings(const QString& name) const {

	QReadLocker locker(&cacheLock);
	// Путь к настройкам: connectionStrings -> {name} -> wal
	QStringList path = { "connectionStrings", name, "wal" };
	QJsonValue walValue = findInObject(cachedJson, path);

	if (walValue.isUndefined() || !walValue.isObject()) {
		qWarning() << "Settings not found for connection:" << name;
		// Пробуем найти default настройки
		if (name != "default") {
			qInfo() << "Trying default settings for:" << name;
			return findSettings("default");
		}

		return std::nullopt;
	}

	return parseSettings(walValue.toObject());
}


std::optional<DatabaseSettingsDataProviderJsonImpl::Settings> DatabaseSettingsDataProviderJsonImpl::get(const QString& name) const {
	if (!d->loadFromFile()) {
		qWarning() << "Failed to load config file, using defaults";
		return Settings();
	}

	return d->findSettings(name);
}
