#pragma once
#include <QString>
#include <QByteArray>

class ThemeManager {
public:
	explicit ThemeManager(const QString& themePath);

	// Возвращает объединенный stylesheet
	QByteArray loadStylesheet();

	// Возвращает путь к файлу палитры внутри темы
	QString getPaletteFilePath() const;

private:
	QString _themeDir;
};
