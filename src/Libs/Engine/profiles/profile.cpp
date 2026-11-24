#include "Libs/Engine/profiles/profile.h"
#include "Libs/Resources/profiles/profile_name.h"

#include <yaml-cpp/yaml.h>

#include <QDebug>
#include <QFile>
#include <QSaveFile>

#include <stdexcept>
#include <utility>

namespace {

constexpr const char* kSettingsFileName = "appearance.yaml";
constexpr const char* kDefaultTheme = "dark";

} // namespace

class Profile::Private {
public:
	Private(QString name, QDir directory)
		: name(std::move(name))
		, directory(std::move(directory)) {
	}

	QString name;
	QDir directory;
	BootstrapConfig::Appearance appearance;
};

Profile::Profile(QString name, const QDir& directory)
	: d(std::make_unique<Private>(std::move(name), directory)) {
}

Profile::~Profile() = default;

QString Profile::name() const {
	return d->name;
}

QDir Profile::directory() const {
	return d->directory;
}

QString Profile::settingsPath() const {
	return d->directory.filePath(QString::fromLatin1(kSettingsFileName));
}

bool Profile::isDefault() const {
	return d->name == Profiles::defaultName();
}

bool Profile::allowsEncryptedStores() const {
	return !isDefault();
}

const BootstrapConfig::Appearance& Profile::appearance() const {
	return d->appearance;
}

bool Profile::load(const BootstrapConfig::Appearance& defaults) {
	if (!Profiles::isValidName(d->name)) {
		qCritical() << "Profile: invalid profile name:" << d->name;
		return false;
	}

	if (!QDir().mkpath(d->directory.absolutePath())) {
		qCritical() << "Profile: cannot create profile directory:" << d->directory.absolutePath();
		return false;
	}

	d->appearance = defaults;

	const auto path = settingsPath();
	QFile file(path);
	if (!file.exists()) {
		qInfo() << "Profile: initializing empty profile from bootstrap defaults:" << path;
		return save(d->appearance);
	}

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qCritical() << "Profile: cannot open settings:" << path;
		return false;
	}

	const auto contents = file.readAll();
	file.close();
	if (file.error() != QFileDevice::NoError) {
		qCritical() << "Profile: cannot read settings:" << path;
		return false;
	}

	// Пустой файл (или только комментарии/пробелы) — профиль ещё не заполнен.
	if (contents.trimmed().isEmpty()) {
		qInfo() << "Profile: empty settings file, initializing from bootstrap defaults:" << path;
		return save(d->appearance);
	}

	YAML::Node root;
	try {
		root = YAML::Load(contents.toStdString());
	}
	catch (const YAML::Exception& error) {
		qCritical() << "Profile: invalid YAML in" << path << ":" << error.what();
		return false;
	}

	if (root.IsNull()) {
		return save(d->appearance);
	}
	if (!root.IsMap()) {
		qCritical() << "Profile: settings must be a map:" << path;
		return false;
	}

	try {
		const auto theme = root["theme"];
		if (theme) {
			if (!theme.IsScalar()) {
				throw std::runtime_error("theme must be a string");
			}
			const auto value = QString::fromStdString(theme.as<std::string>());
			if (!value.isEmpty()) {
				d->appearance.themeName = value;
			}
		}

		const auto application = root["application"];
		if (application) {
			if (!application.IsMap()) {
				throw std::runtime_error("application must be a map");
			}

			const auto name = application["name"];
			if (name) {
				if (!name.IsScalar()) {
					throw std::runtime_error("application.name must be a string");
				}
				const auto value = QString::fromStdString(name.as<std::string>());
				if (!value.isEmpty()) {
					d->appearance.application.name = value;
				}
			}

			const auto viewport = application["viewport"];
			if (viewport) {
				if (!viewport.IsMap()) {
					throw std::runtime_error("application.viewport must be a map");
				}

				const auto xtype = viewport["xtype"];
				if (xtype) {
					if (!xtype.IsScalar()) {
						throw std::runtime_error("application.viewport.xtype must be a string");
					}
					const auto value = QString::fromStdString(xtype.as<std::string>());
					if (!value.isEmpty()) {
						d->appearance.application.viewport = value;
					}
				}
			}
		}
	}
	catch (const std::exception& error) {
		qCritical() << "Profile: invalid settings in" << path << ":" << error.what();
		return false;
	}

	if (d->appearance.themeName.isEmpty()) {
		d->appearance.themeName = defaults.themeName;
	}
	if (d->appearance.themeName.isEmpty()) {
		d->appearance.themeName = QString::fromLatin1(kDefaultTheme);
	}
	if (d->appearance.application.name.isEmpty()) {
		d->appearance.application.name = defaults.application.name;
	}
	if (d->appearance.application.viewport.isEmpty()) {
		d->appearance.application.viewport = defaults.application.viewport;
	}

	if (d->appearance.application.name.isEmpty() || d->appearance.application.viewport.isEmpty()) {
		qCritical() << "Profile: settings do not define appearance.application:" << path;
		return false;
	}

	qInfo() << "Profile loaded:" << d->name << "theme:" << d->appearance.themeName
		<< "application:" << d->appearance.application.name;
	return true;
}

bool Profile::save(const BootstrapConfig::Appearance& appearance) {
	if (!QDir().mkpath(d->directory.absolutePath())) {
		qCritical() << "Profile: cannot create profile directory:" << d->directory.absolutePath();
		return false;
	}

	YAML::Emitter emitter;
	emitter.SetIndent(2);
	emitter << YAML::BeginMap;
	emitter << YAML::Key << "theme" << YAML::Value << appearance.themeName.toStdString();
	emitter << YAML::Key << "application" << YAML::Value << YAML::BeginMap;
	emitter << YAML::Key << "name" << YAML::Value << appearance.application.name.toStdString();
	emitter << YAML::Key << "viewport" << YAML::Value << YAML::BeginMap;
	emitter << YAML::Key << "xtype" << YAML::Value << appearance.application.viewport.toStdString();
	emitter << YAML::EndMap;
	emitter << YAML::EndMap;
	emitter << YAML::EndMap;
	if (!emitter.good()) {
		qCritical() << "Profile: cannot serialize settings:" << emitter.GetLastError().c_str();
		return false;
	}

	QSaveFile file(settingsPath());
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		qCritical() << "Profile: cannot write settings:" << settingsPath();
		return false;
	}

	const auto header = QStringLiteral(
		"# Настройки профиля «%1». Созданы при первом открытии профиля\n"
		"# из значений по умолчанию в bootstrap.yaml (appearance).\n"
		"# Файл можно менять вручную: приложение читает его при старте.\n")
		.arg(d->name);
	file.write(header.toUtf8());
	file.write(QByteArray(emitter.c_str(), static_cast<int>(emitter.size())));
	file.write("\n");

	if (!file.commit()) {
		qCritical() << "Profile: cannot save settings:" << settingsPath();
		return false;
	}

	d->appearance = appearance;
	qInfo() << "Profile settings saved:" << settingsPath();
	return true;
}
