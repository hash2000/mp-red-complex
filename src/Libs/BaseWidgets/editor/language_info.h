#pragma once
#include <QString>
#include <QIcon>

struct LanguageInfo {
	QString name;
	QStringList extensions;
	QString mimeType;
	QIcon icon;
};
