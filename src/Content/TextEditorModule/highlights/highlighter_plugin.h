#pragma once
#include <QString>
#include <QIcon>
#include <QRegularExpression>
#include <QTextCharFormat>

#include <functional>
#include <memory>
#include <vector>

struct LanguageInfo;

class HighlightingPlugin {
public:
	HighlightingPlugin();
	virtual ~HighlightingPlugin();

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

	/// Готовый фрагмент подсветки: границы и формат правила.
	///
	/// Плагин отдаёт фрагменты через visitor, не собирая промежуточные
	/// векторы: подсветка выполняется для каждой строки документа, поэтому
	/// лишние аллокации и копии QTextCharFormat заметны на больших текстах.
	using MatchVisitor = std::function<void(qsizetype start, qsizetype length,
		const QTextCharFormat& format, int priority)>;

	/// Фрагмент вложенного блока: дополнительно сообщает тип (начало/конец)
	/// и язык вложенной подсветки.
	using EmbeddedMatchVisitor = std::function<void(qsizetype start, qsizetype length,
		const QTextCharFormat& format, int priority, HighlightingMatch::Type type,
		const QString& language)>;

	virtual LanguageInfo languageInfo() const = 0;
	virtual void install() = 0;

	// Для анализа переменных
	virtual QStringList extractVariables(const QString& code) const {
		return { };
	}

	virtual QStringList extractFunctions(const QString& code) const {
		return { };
	}

	/// Перебирает итоговые (не перекрывающиеся) фрагменты подсветки строки.
	void visitMatches(const QString& text, const MatchVisitor& visitor) const;

	/// Перебирает итоговые фрагменты вложенных блоков (например, ```).
	void visitEmbeddedMatches(const QString& text, const EmbeddedMatchVisitor& visitor) const;

	/// true — у плагина есть правила вложенных блоков: их не нужно
	/// проверять для каждого плагина (частый случай — обычный текст и JSON).
	bool hasEmbeddedRules() const;

	/// true — у плагина есть обычные правила подсветки.
	bool hasRules() const;

	/// Совместимый построчный API: результат в виде вектора.
	std::vector<HighlightingMatch> matches(const QString& text) const;
	std::vector<HighlightingMatch> matchesEmbeddedBlock(const QString& text) const;

protected:
	void addRule(const Rule& rule);
	void addEmbeddedBlockRule(const EmbeddedBlockRule& rule);

private:
	class Private;
	std::unique_ptr<Private> d;
};
