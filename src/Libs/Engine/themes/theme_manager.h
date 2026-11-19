#pragma once
#include <QString>

class ThemeManager {
public:
	explicit ThemeManager(const QString& themePath);

	// Возвращает объединенный stylesheet
	QByteArray loadStylesheet();

	// Возвращает путь к файлу палитры внутри темы
	QString getPaletteFilePath() const;

	Qt::ColorScheme getColorScheme(const QString& themeName);

private:
	QString _themeDir;
};
