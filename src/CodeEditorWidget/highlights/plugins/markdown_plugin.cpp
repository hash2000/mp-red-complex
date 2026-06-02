#include "CodeEditorWidget/highlights/plugins/markdown_plugin.h"
#include "CodeEditorWidget/highlights/highlighter.h"
#include <QRegularExpression>

MarkdownPlugin::LanguageInfo MarkdownPlugin::languageInfo() const {
	return {
		"Markdown",
		{ ".md" },
		"text/markdown",
		QIcon(":/icons/md.png")
	};
}

void MarkdownPlugin::install(Highlighter& highlighter, HighlighterRuleType type) const {
	// ==========================================
	// 1. Заголовки (Headers)
	// ==========================================
	QTextCharFormat h1Format;
	h1Format.setForeground(QColor("#569CD6"));  // синий
	h1Format.setFontWeight(QFont::Bold);
	h1Format.setFontPointSize(24);  // можно настроить размер

	QTextCharFormat h2Format;
	h2Format.setForeground(QColor("#569CD6"));
	h2Format.setFontWeight(QFont::Bold);
	h2Format.setFontPointSize(20);

	QTextCharFormat h3Format;
	h3Format.setForeground(QColor("#569CD6"));
	h3Format.setFontWeight(QFont::Bold);
	h3Format.setFontPointSize(16);

	QTextCharFormat h4Format;
	h4Format.setForeground(QColor("#569CD6"));
	h4Format.setFontWeight(QFont::Bold);
	h4Format.setFontPointSize(14);

	QTextCharFormat h5Format;
	h5Format.setForeground(QColor("#569CD6"));
	h4Format.setFontWeight(QFont::Bold);

	QTextCharFormat h6Format;
	h6Format.setForeground(QColor("#569CD6"));
	h6Format.setFontWeight(QFont::Bold);

	// Заголовки с # (ATX-style)
	highlighter.addRule("md_h1", "^# [^\n]+$", h1Format, 10, type);
	highlighter.addRule("md_h2", "^## [^\n]+$", h2Format, 10, type);
	highlighter.addRule("md_h3", "^### [^\n]+$", h3Format, 10, type);
	highlighter.addRule("md_h4", "^#### [^\n]+$", h4Format, 10, type);
	highlighter.addRule("md_h5", "^##### [^\n]+$", h5Format, 10, type);
	highlighter.addRule("md_h6", "^###### [^\n]+$", h6Format, 10, type);

	// Альтернативные заголовки (Setext-style)
	QTextCharFormat altH1Format;
	altH1Format.setForeground(QColor("#569CD6"));
	altH1Format.setFontWeight(QFont::Bold);
	altH1Format.setFontUnderline(true);

	highlighter.addRule("md_alt_h1_underline", "^=+$", altH1Format, 11, type);
	highlighter.addRule("md_alt_h2_underline", "^-+$", altH1Format, 11, type);

	// ==========================================
	// 2. Жирный текст (Bold)
	// ==========================================
	QTextCharFormat boldFormat;
	boldFormat.setFontWeight(QFont::Bold);

	highlighter.addRule("md_bold_double_asterisk", "\\*\\*[^\\*]+\\*\\*", boldFormat, 20, type);
	highlighter.addRule("md_bold_double_underscore", "__[^_]+__", boldFormat, 20, type);

	// ==========================================
	// 3. Курсив (Italic)
	// ==========================================
	QTextCharFormat italicFormat;
	italicFormat.setFontItalic(true);

	highlighter.addRule("md_italic_asterisk", "(?<!\\*)\\*(?!\\*)[^\\*]+\\*(?!\\*)", italicFormat, 21, type);
	highlighter.addRule("md_italic_underscore", "(?<!_)_(?!_)[^_]+_(?!_)", italicFormat, 21, type);

	// ==========================================
	// 4. Жирный курсив (Bold + Italic)
	// ==========================================
	QTextCharFormat boldItalicFormat;
	boldItalicFormat.setFontWeight(QFont::Bold);
	boldItalicFormat.setFontItalic(true);

	highlighter.addRule("md_bold_italic_triple", "\\*\\*\\*[^\\*]+\\*\\*\\*", boldItalicFormat, 19, type);
	highlighter.addRule("md_bold_italic_mixed1", "___[^_]+___", boldItalicFormat, 19, type);
	highlighter.addRule("md_bold_italic_mixed2", "\\*\\*_[^_]+_\\*\\*", boldItalicFormat, 19, type);

	// ==========================================
	// 5. Зачеркнутый текст (Strikethrough)
	// ==========================================
	QTextCharFormat strikeFormat;
	strikeFormat.setFontStrikeOut(true);
	strikeFormat.setForeground(QColor("#808080"));  // серый

	highlighter.addRule("md_strikethrough", "~~[^~]+~~", strikeFormat, 22, type);

	// ==========================================
	// 6. Ссылки (Links)
	// ==========================================
	QTextCharFormat linkFormat;
	linkFormat.setForeground(QColor("#4EA1FE"));  // голубой
	linkFormat.setUnderlineStyle(QTextCharFormat::SingleUnderline);

	// Полные ссылки [text](url)
	highlighter.addRule("md_inline_link", "\\[([^\\]]+)\\]\\(([^\\)]+)\\)", linkFormat, 25, type);

	// Простые URL
	QTextCharFormat urlFormat;
	urlFormat.setForeground(QColor("#4EA1FE"));
	urlFormat.setUnderlineStyle(QTextCharFormat::SingleUnderline);

	highlighter.addRule("md_url",
		"https?://[^\\s\\)]+|ftp://[^\\s\\)]+|www\\.[^\\s\\)]+",
		urlFormat, 26, type);

	// Ссылки-сноски [text][id]
	highlighter.addRule("md_reference_link", "\\[([^\\]]+)\\]\\[([^\\]]*)\\]", linkFormat, 25, type);

	// ==========================================
	// 7. Изображения (Images)
	// ==========================================
	QTextCharFormat imageFormat;
	imageFormat.setForeground(QColor("#CE9178"));  // оранжевый
	imageFormat.setFontItalic(true);

	highlighter.addRule("md_image", "!\\[([^\\]]*)\\]\\(([^\\)]+)\\)", imageFormat, 27, type);

	// ==========================================
	// 8. Цитаты (Blockquotes)
	// ==========================================
	QTextCharFormat blockquoteFormat;
	blockquoteFormat.setForeground(QColor("#6A9955"));  // зеленый
	blockquoteFormat.setFontItalic(true);
	blockquoteFormat.setBackground(QColor("#2D2D2D"));  // темно-серый фон

	highlighter.addRule("md_blockquote", "^> .*$", blockquoteFormat, 30, type);

	// ==========================================
	// 9. Списки (Lists)
	// ==========================================
	QTextCharFormat unorderedListFormat;
	unorderedListFormat.setForeground(QColor("#FFE66D"));  // желтый
	unorderedListFormat.setFontWeight(QFont::Bold);

	// Маркированные списки
	highlighter.addRule("md_unordered_list", "^[\\s]*[-*+] .*$", unorderedListFormat, 35, type);

	QTextCharFormat orderedListFormat;
	orderedListFormat.setForeground(QColor("#FFE66D"));
	orderedListFormat.setFontWeight(QFont::Bold);

	// Нумерованные списки
	highlighter.addRule("md_ordered_list", "^[\\s]*\\d+\\. .*$", orderedListFormat, 35, type);

	// ==========================================
	// 10. Горизонтальные линии (Horizontal Rules)
	// ==========================================
	QTextCharFormat hrFormat;
	hrFormat.setForeground(QColor("#808080"));

	highlighter.addRule("md_horizontal_rule", "^[-*_]{3,}$", hrFormat, 40, type);

	// ==========================================
	// 11. Специальные символы (Emphasis markers)
	// ==========================================
	QTextCharFormat markerFormat;
	markerFormat.setForeground(QColor("#808080"));  // серый

	highlighter.addRule("md_list_marker", "^[\\s]*[-*+] ", markerFormat, 36, type);
	highlighter.addRule("md_ordered_list_marker", "^[\\s]*\\d+\\. ", markerFormat, 36, type);
	highlighter.addRule("md_blockquote_marker", "^> ", markerFormat, 31, type);

	// ==========================================
	// 12. Escape-последовательности
	// ==========================================
	QTextCharFormat escapeFormat;
	escapeFormat.setForeground(QColor("#D7BA7D"));  // золотой
	escapeFormat.setFontWeight(QFont::Bold);

	highlighter.addRule("md_escape", "\\\\[\\*_\\[\\]()#\\+\\-\\.!|~`<>]", escapeFormat, 5, type);

	// ==========================================
	// 13. HTML теги (опционально, если поддерживается в вашем MD)
	// ==========================================
	QTextCharFormat htmlTagFormat;
	htmlTagFormat.setForeground(QColor("#808080"));
	htmlTagFormat.setFontFamily("Courier New");

	highlighter.addRule("md_html_tag", "</?\\w+[^>]*>", htmlTagFormat, 45, type);

	// ==========================================
	// 14. Таблицы (Table separators)
	// ==========================================
	QTextCharFormat tableFormat;
	tableFormat.setForeground(QColor("#569CD6"));

	highlighter.addRule("md_table_separator", "^\\|[\\s\\-:]+\\|", tableFormat, 40, type);
	highlighter.addRule("md_table_row", "^\\|[^\\n]+\\|$", tableFormat, 41, type);

	QTextCharFormat codeBlockFormat;
	codeBlockFormat.setForeground(QColor("#CE9178"));
	codeBlockFormat.setBackground(QColor("#1E1E1E"));

	highlighter.addEmbeddedCodeRule(
		"md_embedded_code_block",
		R"(^```(\w+)[\r\n]*$)", // Начало: ```язык + перевод строки
		R"(^```$)", // Конец: ``` на отдельной строке
		"$1", // Идентификатор языка - захватывается из начала
		codeBlockFormat,
		14
	);

	// Блок кода без языка: ``` ... ```
	highlighter.addEmbeddedCodeRule(
		"md_embedded_code_block_plain",
		R"(```)", // Начало: ``` + перевод строки
		R"(^```$)", // Конец: ``` на отдельной строке
		"", // Без указания языка
		codeBlockFormat,
		14
	);
}

void MarkdownPlugin::uninstall(Highlighter& highlighter) const {
	highlighter.deleteRule("md_h1");
	highlighter.deleteRule("md_h2");
	highlighter.deleteRule("md_h3");
	highlighter.deleteRule("md_h4");
	highlighter.deleteRule("md_h5");
	highlighter.deleteRule("md_h6");
	highlighter.deleteRule("md_alt_h1_underline");
	highlighter.deleteRule("md_alt_h2_underline");
	highlighter.deleteRule("md_bold_double_asterisk");
	highlighter.deleteRule("md_bold_double_underscore");
	highlighter.deleteRule("md_italic_asterisk");
	highlighter.deleteRule("md_italic_underscore");
	highlighter.deleteRule("md_bold_italic_triple");
	highlighter.deleteRule("md_bold_italic_mixed1");
	highlighter.deleteRule("md_bold_italic_mixed2");
	highlighter.deleteRule("md_strikethrough");
	highlighter.deleteRule("md_inline_link");
	highlighter.deleteRule("md_url");
	highlighter.deleteRule("md_image");
	highlighter.deleteRule("md_inline_code");
	highlighter.deleteRule("md_code_block_marker");
	highlighter.deleteRule("md_blockquote");
	highlighter.deleteRule("md_unordered_list");
	highlighter.deleteRule("md_ordered_list");
	highlighter.deleteRule("md_horizontal_rule");
	highlighter.deleteRule("md_list_marker");
	highlighter.deleteRule("md_ordered_list_marker");
	highlighter.deleteRule("md_blockquote_marker");
	highlighter.deleteRule("md_escape");
	highlighter.deleteRule("md_html_tag");
	highlighter.deleteRule("md_table_separator");
	highlighter.deleteRule("md_table_row");
	highlighter.deleteEmbeddedCodeRule("md_embedded_code_block");
	highlighter.deleteEmbeddedCodeRule("md_embedded_code_block_plain");
}
