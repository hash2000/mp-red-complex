#pragma once
#include <QMap>
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

	// Новые методы для вложенных языков, например ```cpp {code}``` - вложенный код в marcdown разметке
	struct EmbeddedRegion {
		int start; // начало региона в текущем блоке
		int length; // длина региона
		QString languageId; // идентификатор языка (например, "glsl", "cpp", "python")
		QMap<QString, QString> metadata; // дополнительная информация
	};

	virtual LanguageInfo languageInfo() const = 0;
	virtual void install(Highlighter& highlighter) const = 0;
	virtual void uninstall(Highlighter& highlighter) const = 0;

	// Для анализа переменных
	virtual QStringList extractVariables(const QString& code) const {
		return { };
	}

	virtual QStringList extractFunctions(const QString& code) const {
		return { };
	}

	// Находит вложенные регионы в строке текста
	virtual QList<EmbeddedRegion> findEmbeddedRegions(const QString& text) const {
		return { };
	}

	// Может ли плагин обрабатывать вложенный регион?
	virtual bool canHandleEmbeddedRegion(const QString& languageId) const {
		return false;
	}

	// Создает временный highlighter для вложенного региона
	virtual void setupEmbeddedHighlighting(Highlighter& highlighter,
		const QString& languageId,
		const QMap<QString, QString>& metadata) const {
	}
};
