#include "Libs/Engine/scripting/bootstrap/bootstrap_manager.h"
#include "Libs/Engine/scripting/bootstrap/bootstrap_config.h"
#include "Libs/Engine/profiles/profile.h"
#include "Libs/Engine/themes/theme_manager.h"
#include "Libs/Engine/themes/palette_parser.h"
#include "Libs/Resources/resources.h"
#include <QApplication>
#include <QDir>
#include <QPalette>
#include <QStyleHints>
#include <QDebug>

class BootstrapManager::Private {
public:
	Private(BootstrapManager* parent) : q(parent) {}
	BootstrapManager* q;
	Resources* resources;
	BootstrapConfig::Config config;
	std::unique_ptr<Profile> profile;
};

BootstrapManager::BootstrapManager(Resources* resources)
	: d(std::make_unique<Private>(this)) {
	d->resources = resources;
}

BootstrapManager::~BootstrapManager() = default;

const BootstrapConfig::Config& BootstrapManager::config() const {
	return d->config;
}

QString BootstrapManager::profileName() const {
	return d->profile ? d->profile->name() : QString();
}

bool BootstrapManager::reload() {

	try {
		// 1. Загрузка bootstrap.yaml
		// Предполагаем, что исполняемый файл лежит рядом с папкой assets
		auto basePath = d->resources->Directories.get("path.profile");
		if (!basePath) {
			throw std::runtime_error("Bootstrap path is not set");
		}

		auto bootstrapPath = basePath->filePath("bootstrap.yaml");

		qDebug() << "Loading bootstrap from:" << bootstrapPath;
		d->config = BootstrapConfig::loadBootstrap(bootstrapPath);

		// 1.1. Профиль пользователя: его appearance.yaml перекрывает
		// значения по умолчанию из bootstrap.yaml. Пустой профиль
		// инициализируется этими значениями при первом открытии.
		const auto profilePath = d->resources->currentProfilePath();
		if (!profilePath) {
			throw std::runtime_error("Profile path is not selected");
		}

		const auto profileName = d->resources->currentProfileName();
		if (profileName.isEmpty()) {
			throw std::runtime_error("Current profile is not selected");
		}

		d->profile = std::make_unique<Profile>(profileName, profilePath.value());
		if (!d->profile->load(d->config.appearance)) {
			return false;
		}

		d->config.appearance = d->profile->appearance();

		qDebug() << "Using profile:" << d->profile->name()
			<< "settings:" << d->profile->settingsPath()
			<< "encrypted stores:" << d->profile->allowsEncryptedStores();

		if (d->config.appearance.themeName.isEmpty()) {
			throw std::runtime_error("Theme name not specified in bootstrap or profile");
		}

		// 2. Инициализация менеджера темы
		// Путь к теме: assets/themes/{theme_name}/
		QString themeDir = basePath->filePath("assets/themes/" + d->config.appearance.themeName);
		ThemeManager themeMgr(themeDir);

		qDebug() << "Using theme directory:" << themeDir;

		// Каталог темы доступен остальным частям приложения (например,
		// консоли для console-document.css).
		d->resources->Directories.set("path.theme", QDir(themeDir));

		// 3. Загрузка и применение Stylesheet
		QByteArray cssData = themeMgr.loadStylesheet();
		if (!cssData.isEmpty()) {
			qApp->setStyleSheet(QString::fromUtf8(cssData));
			qDebug() << "Stylesheet applied:" << cssData.size() << "bytes";
		}
		else {
			qWarning() << "Empty stylesheet loaded.";
		}

		// 4. Загрузка и применение Palette
		// Используем наш парсер. 
		// Примечание: В реальной системе здесь мог бы вызываться DataFlow граф 
		// (cfg.processorPath), который вернул бы готовый QPalette.
		// Сейчас мы делаем прямую загрузку для демонстрации механики.

		QString paletteFile = themeMgr.getPaletteFilePath();
		qDebug() << "Loading palette from:" << paletteFile;

		QPalette palette = PaletteParser::parseFromFile(paletteFile);
		qApp->setPalette(palette);
		qApp->styleHints()->setColorScheme(themeMgr.getColorScheme(d->config.appearance.themeName));
		qDebug() << "Palette applied: window" << palette.color(QPalette::Window).name()
			<< "base" << palette.color(QPalette::Base).name()
			<< "button" << palette.color(QPalette::Button).name()
			<< "text" << palette.color(QPalette::Text).name();
	}
	catch (const std::exception& e) {
		qCritical() << "Fatal error during startup:" << e.what();
		return false;
	}

	return true;
}
