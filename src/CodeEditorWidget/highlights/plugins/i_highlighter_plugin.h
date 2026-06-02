#pragma once
#include <QMap>
#include <QString>
#include <QIcon>

class Highlighter;
enum class HighlighterRuleType;

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
	virtual void install(Highlighter& highlighter, HighlighterRuleType type) const = 0;
	virtual void uninstall(Highlighter& highlighter) const = 0;

	// Для анализа переменных
	virtual QStringList extractVariables(const QString& code) const {
		return { };
	}

	virtual QStringList extractFunctions(const QString& code) const {
		return { };
	}
};
