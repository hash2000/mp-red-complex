#include "Libs/Engine/themes/theme_manager.h"
#include <yaml-cpp/yaml.h>
#include <QFile>
#include <QDir>
#include <QDebug>

ThemeManager::ThemeManager(const QString& themePath)
	: _themeDir(themePath) {
	QDir dir(_themeDir);
	if (!dir.exists()) {
		throw std::runtime_error("Theme directory does not exist: " + themePath.toStdString());
	}
}

QString ThemeManager::getPaletteFilePath() const {
	return _themeDir + "/palette.yaml";
}

QByteArray ThemeManager::loadStylesheet() {
	QString indexPath = _themeDir + "/stylesheets/index.yaml";

	QFile indexFile(indexPath);
	if (!indexFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qWarning() << "No stylesheet index found at:" << indexPath;
		return { };
	}

	YAML::Node root = YAML::Load(indexFile.readAll().toStdString());

	if (!root["items"]) {
		return { };
	}

	QByteArray fullSheet;
	QTextStream out(&fullSheet);

	for (const auto& item : root["items"]) {
		std::string styleName = item.as<std::string>();
		QString fileName = QString::fromStdString(styleName) + ".qss";
		QString fullPath = _themeDir + "/stylesheets/" + fileName;

		QFile qssFile(fullPath);
		if (qssFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
			out << "\n/* Included: " << fileName << " */\n";
			out << qssFile.readAll();
			out << "\n";
		}
		else {
			qWarning() << "Could not open stylesheet file:" << fullPath;
		}
	}

	out.flush();
	return fullSheet;
}

Qt::ColorScheme ThemeManager::getColorScheme(const QString& themeName) {
	if (themeName == "dark") return Qt::ColorScheme::Dark;
	if (themeName == "light") return Qt::ColorScheme::Light;

	return Qt::ColorScheme::Dark;
}
