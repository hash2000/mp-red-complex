#pragma once
#include <QMap>
#include <QString>
#include <QIcon>
#include <QRegularExpression>
#include <QTextCharFormat>

#include <memory>

enum class HighlighterRuleType;

class HighlightingPlugin {
public:
	HighlightingPlugin();
	virtual ~HighlightingPlugin();

	struct LanguageInfo {
		QString name;
		QStringList extensions;
		QString mimeType;
		QIcon icon; // для UI
	};

	struct Rule {
		QRegularExpression pattern;
		QTextCharFormat format;
		int priority = 0; // 0 = высокий, 100 = низкий
	};

	struct EmbeddedBlockRule {
		QRegularExpression startPattern;
		QRegularExpression endPattern;
		QString languageId;
		QTextCharFormat format;
		int priority = 0;
	};

	struct HighlightingMatch {
		qsizetype start;
		qsizetype length;
		QTextCharFormat format;
		int priority = 0;
		enum class Type {
			Start,
			End,
		} type;
		QString language;
	};

	virtual LanguageInfo languageInfo() const = 0;
	virtual void install() = 0;

	// Для анализа переменных
	virtual QStringList extractVariables(const QString& code) const {
		return { };
	}

	virtual QStringList extractFunctions(const QString& code) const {
		return { };
	}

	std::vector<HighlightingMatch> matches(const QString& text) const;
	std::vector<HighlightingMatch> matchesEmbeddedBlock(const QString& text) const;

protected:
	void addRule(const Rule& rule);
	void addEmbeddedBlockRule(const EmbeddedBlockRule& rule);

private:
	class Private;
	std::unique_ptr<Private> d;
};
