#pragma once
#include "Content/CodeEditorWidget/highlights/highlighter_plugin.h"
#include "Content/CodeEditorWidget/language_info.h"

#include <QRegularExpression>
#include <QTextCharFormat>

#include <memory>
#include <vector>

/// Плагин подсветки, полностью описанный данными темы:
/// `assets/themes/{theme}/highlighters/{language}.yaml`.
///
/// В коде не остаётся ни правил, ни цветов: файл темы задаёт язык
/// (имя, mime-тип, расширения, иконку), правила и вложенные блоки.
class ThemeHighlighterPlugin : public HighlightingPlugin {
public:
	explicit ThemeHighlighterPlugin(const QString& themeFile);
	~ThemeHighlighterPlugin() override;

	LanguageInfo languageInfo() const override;
	void install() override;

	/// Загружает все `*.yaml` из каталога темы. Файлы обходятся
	/// в алфавитном порядке, чтобы регистрация была детерминированной.
	static std::vector<std::unique_ptr<HighlightingPlugin>> loadDirectory(const QString& directory);

private:
	struct RuleSpec {
		QRegularExpression pattern;
		QTextCharFormat format;
		int priority = 0;
	};

	struct EmbeddedSpec {
		QRegularExpression start;
		QRegularExpression end;
		QString language;
		QTextCharFormat format;
		int priority = 0;
	};

	LanguageInfo _info;
	std::vector<RuleSpec> _rules;
	std::vector<EmbeddedSpec> _embeddedRules;
	/// install() идемпотентен: правила добавляются ровно один раз,
	/// даже если плагин загружен через loadDirectory().
	bool _installed = false;
};
