#include "Libs/Engine/bootstrap/bootstrap_manager.h"
#include "Libs/Engine/bootstrap/bootstrap_config.h"
#include "Libs/Engine/themes/theme_manager.h"
#include "Libs/Engine/themes/palette_parser.h"
#include "Libs/Resources/resources.h"
#include <QApplication>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QDebug>

class BootstrapManager::Private {
public:
	Private(BootstrapManager* parent) : q(parent) {}
	BootstrapManager* q;
	Resources* resources;
};

BootstrapManager::BootstrapManager(Resources* resources)
	: d(std::make_unique<Private>(this)) {
	d->resources = resources;
}

BootstrapManager::~BootstrapManager() = default;

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
		BootstrapConfig::Config config = BootstrapConfig::loadBootstrap(bootstrapPath);

		if (config.themeName.isEmpty()) {
			throw std::runtime_error("Theme name not specified in bootstrap");
		}

		// 2. Инициализация менеджера темы
		// Путь к теме: assets/themes/{theme_name}/
		QString themeDir = basePath->filePath("assets/themes/" + config.themeName);
		ThemeManager themeMgr(themeDir);

		qDebug() << "Using theme directory:" << themeDir;

		// 3. Загрузка и применение Stylesheet
		QByteArray cssData = themeMgr.loadStylesheet();
		if (!cssData.isEmpty()) {
			qApp->setStyleSheet(QString::fromUtf8(cssData));
			qDebug() << "Stylesheet applied.";
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
		qDebug() << "Palette applied.";
	}
	catch (const std::exception& e) {
		qCritical() << "Fatal error during startup:" << e.what();
		return false;
	}

	return true;
}
