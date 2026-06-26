#include "Content/CodeEditorWidget/highlights/plugins/markdown_plugin.h"
#include <QRegularExpression>

MarkdownPlugin::~MarkdownPlugin() = default;

MarkdownPlugin::LanguageInfo MarkdownPlugin::languageInfo() const {
	return {
		"Markdown",
		{ ".md" },
		"text/markdown",
		QIcon(":/icons/md.png")
	};
}

void MarkdownPlugin::install()  {
	// ==========================================
	// Заголовки (Headers)
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
	addRule(Rule{ QRegularExpression("^# [^\n]+$"), h1Format, 10 });
	addRule(Rule{ QRegularExpression("^## [^\n]+$"), h2Format, 10 });
	addRule(Rule{ QRegularExpression("^### [^\n]+$"), h3Format, 10 });
	addRule(Rule{ QRegularExpression("^#### [^\n]+$"), h4Format, 10 });
	addRule(Rule{ QRegularExpression("^##### [^\n]+$"), h5Format, 10 });
	addRule(Rule{ QRegularExpression("^###### [^\n]+$"), h6Format, 10 });

	// Альтернативные заголовки (Setext-style)
	QTextCharFormat altH1Format;
	altH1Format.setForeground(QColor("#569CD6"));
	altH1Format.setFontWeight(QFont::Bold);
	altH1Format.setFontUnderline(true);

	addRule(Rule{ QRegularExpression("^=+$"), altH1Format, 11 });
	addRule(Rule{ QRegularExpression("^-+$"), altH1Format, 11 });

	// ==========================================
	// Жирный текст (Bold)
	// ==========================================
	QTextCharFormat boldFormat;
	boldFormat.setFontWeight(QFont::Bold);

	addRule(Rule{ QRegularExpression("\\*\\*[^\\*]+\\*\\*"), boldFormat, 20 });
	addRule(Rule{ QRegularExpression("__[^_]+__"), boldFormat, 20 });

	// ==========================================
	// Курсив (Italic)
	// ==========================================
	QTextCharFormat italicFormat;
	italicFormat.setFontItalic(true);

	addRule(Rule{ QRegularExpression("(?<!\\*)\\*(?!\\*)[^\\*]+\\*(?!\\*)"), italicFormat, 21 });
	addRule(Rule{ QRegularExpression("(?<!_)_(?!_)[^_]+_(?!_)"), italicFormat, 21 });

	// ==========================================
	// Жирный курсив (Bold + Italic)
	// ==========================================
	QTextCharFormat boldItalicFormat;
	boldItalicFormat.setFontWeight(QFont::Bold);
	boldItalicFormat.setFontItalic(true);

	addRule(Rule{ QRegularExpression("\\*\\*\\*[^\\*]+\\*\\*\\*"), boldItalicFormat, 19 });
	addRule(Rule{ QRegularExpression("___[^_]+___"), boldItalicFormat, 19 });
	addRule(Rule{ QRegularExpression("\\*\\*_[^_]+_\\*\\*"), boldItalicFormat, 19 });

	// ==========================================
	// Зачеркнутый текст (Strikethrough)
	// ==========================================
	QTextCharFormat strikeFormat;
	strikeFormat.setFontStrikeOut(true);
	strikeFormat.setForeground(QColor("#808080"));  // серый

	addRule(Rule{ QRegularExpression("~~[^~]+~~"), strikeFormat, 22 });

	// ==========================================
	// Ссылки (Links)
	// ==========================================
	QTextCharFormat linkFormat;
	linkFormat.setForeground(QColor("#4EA1FE"));  // голубой
	linkFormat.setUnderlineStyle(QTextCharFormat::SingleUnderline);

	// Полные ссылки [text](url)
	addRule(Rule{ QRegularExpression("\\[([^\\]]+)\\]\\(([^\\)]+)\\)"), linkFormat, 25 });

	// Простые URL
	QTextCharFormat urlFormat;
	urlFormat.setForeground(QColor("#4EA1FE"));
	urlFormat.setUnderlineStyle(QTextCharFormat::SingleUnderline);

	addRule(Rule{ QRegularExpression(
		"https?://[^\\s\\)]+|ftp://[^\\s\\)]+|www\\.[^\\s\\)]+"),
		urlFormat, 26 });

	// Ссылки-сноски [text][id]
	addRule(Rule{ QRegularExpression("\\[([^\\]]+)\\]\\[([^\\]]*)\\]"), linkFormat, 25 });

	// ==========================================
	// Изображения (Images)
	// ==========================================
	QTextCharFormat imageFormat;
	imageFormat.setForeground(QColor("#CE9178"));  // оранжевый
	imageFormat.setFontItalic(true);

	addRule(Rule{ QRegularExpression("!\\[([^\\]]*)\\]\\(([^\\)]+)\\)"), imageFormat, 27 });

	// ==========================================
	// Цитаты (Blockquotes)
	// ==========================================
	QTextCharFormat blockquoteFormat;
	blockquoteFormat.setForeground(QColor("#6A9955"));  // зеленый
	blockquoteFormat.setFontItalic(true);
	blockquoteFormat.setBackground(QColor("#2D2D2D"));  // темно-серый фон

	addRule(Rule{ QRegularExpression("^> .*$"), blockquoteFormat, 30 });

	// ==========================================
	// Списки (Lists)
	// ==========================================
	QTextCharFormat unorderedListFormat;
	unorderedListFormat.setForeground(QColor("#FFE66D"));  // желтый
	unorderedListFormat.setFontWeight(QFont::Bold);

	// Маркированные списки
	addRule(Rule{ QRegularExpression("^[\\s]*[-*+] .*$"), unorderedListFormat, 35 });

	QTextCharFormat orderedListFormat;
	orderedListFormat.setForeground(QColor("#FFE66D"));
	orderedListFormat.setFontWeight(QFont::Bold);

	// Нумерованные списки
	addRule(Rule{ QRegularExpression("^[\\s]*\\d+\\. .*$"), orderedListFormat, 35 });

	// ==========================================
	// Горизонтальные линии (Horizontal Rules)
	// ==========================================
	QTextCharFormat hrFormat;
	hrFormat.setForeground(QColor("#808080"));

	addRule(Rule{ QRegularExpression("^[-*_]{3,}$"), hrFormat, 40 });

	// ==========================================
	// Специальные символы (Emphasis markers)
	// ==========================================
	QTextCharFormat markerFormat;
	markerFormat.setForeground(QColor("#808080"));  // серый

	addRule(Rule{ QRegularExpression("^[\\s]*[-*+] "), markerFormat, 36 });
	addRule(Rule{ QRegularExpression("^[\\s]*\\d+\\. "), markerFormat, 36 });
	addRule(Rule{ QRegularExpression("^> "), markerFormat, 31 });

	// ==========================================
	// Escape-последовательности
	// ==========================================
	QTextCharFormat escapeFormat;
	escapeFormat.setForeground(QColor("#D7BA7D"));  // золотой
	escapeFormat.setFontWeight(QFont::Bold);

	addRule(Rule{ QRegularExpression("\\\\[\\*_\\[\\]()#\\+\\-\\.!|~`<>]"), escapeFormat, 5 });

	// ==========================================
	// HTML теги (опционально, если поддерживается в вашем MD)
	// ==========================================
	QTextCharFormat htmlTagFormat;
	htmlTagFormat.setForeground(QColor("#808080"));
	htmlTagFormat.setFontFamily("Courier New");

	addRule(Rule{ QRegularExpression("</?\\w+[^>]*>"), htmlTagFormat, 45 });

	// ==========================================
	// Таблицы (Table separators)
	// ==========================================
	QTextCharFormat tableFormat;
	tableFormat.setForeground(QColor("#569CD6"));

	addRule(Rule{ QRegularExpression("^\\|[\\s\\-:]+\\|"), tableFormat, 40 });
	addRule(Rule{ QRegularExpression("^\\|[^\\n]+\\|$"), tableFormat, 41 });

	QTextCharFormat codeBlockFormat;
	codeBlockFormat.setForeground(QColor("#CE9178"));
	codeBlockFormat.setBackground(QColor("#1E1E1E"));

	addEmbeddedBlockRule(EmbeddedBlockRule{
		QRegularExpression(R"(^```(\w+)[\r\n]*$)"), // Начало: ```язык + перевод строки
		QRegularExpression(R"(^```$)"), // Конец: ``` на отдельной строке
		"$1", // Идентификатор языка - захватывается из начала
		codeBlockFormat,
		14 });

	// Блок кода без языка: ``` ... ```
	addEmbeddedBlockRule(EmbeddedBlockRule{
		QRegularExpression(R"(```)"), // Начало: ``` + перевод строки
		QRegularExpression(R"(^```$)"), // Конец: ``` на отдельной строке
		"", // Без указания языка
		codeBlockFormat,
		14 });
}
