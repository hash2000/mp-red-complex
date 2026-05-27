#pragma once
#include <QString>
#include <QIcon>

class Highlighter;

class IHighlightingPlugin {
public:
	virtual ~IHighlightingPlugin() = default;

	struct LanguageInfo {
		QString name;
		QStringList extensions;
		QString mimeType;
		QIcon icon; // для UI
	};

	virtual LanguageInfo languageInfo() const = 0;
	virtual void install(Highlighter& highlighter) = 0;
	virtual void uninstall(Highlighter& highlighter) = 0;

	// Для анализа переменных (ваш кейс)
	virtual QStringList extractVariables(const QString& code) const {
		return { };
	}
	virtual QStringList extractFunctions(const QString& code) const {
		return { };
	}
};
