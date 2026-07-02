#include "Content/CodeEditorWidget/highlights/plugins/css_plugin.h"
#include "Content/CodeEditorWidget/language_info.h"

#include <QRegularExpression>

CssHighlighterPlugin::~CssHighlighterPlugin() = default;
LanguageInfo CssHighlighterPlugin::languageInfo() const {
	return { "CSS", {
		".css", ".scss", ".sass", ".less" },
		"text/css" };
}

void CssHighlighterPlugin::install() {
	// ==========================================
	// 1. Комментарии
	// ==========================================
	QTextCharFormat commentFormat;
	commentFormat.setForeground(QColor("#6A9955"));  // зеленый
	commentFormat.setFontItalic(true);

	addRule({ QRegularExpression(R"(/\*[\s\S]*?\*/)"),
		commentFormat, 5 });

	// ==========================================
	// 2. Селекторы (теги, классы, ID)
	// ==========================================
	QTextCharFormat selectorFormat;
	selectorFormat.setForeground(QColor("#D7BA7D"));  // золотой
	selectorFormat.setFontWeight(QFont::Bold);

	// Теги
	addRule({ QRegularExpression(R"(^[\s]*[a-zA-Z][\w-]*(?=\s*[{,]))"),
		selectorFormat, 10 });

	// Классы
	addRule({ QRegularExpression(R"(\.[\w-]+)"),
		selectorFormat, 11 });

	// ID
	addRule({ QRegularExpression(R"(#[\w-]+)"),
		selectorFormat, 11 });

	// Псевдоклассы и псевдоэлементы
	addRule({ QRegularExpression(R"(::?[\w-]+)"),
		selectorFormat, 12 });

	// Атрибутные селекторы
	addRule({ QRegularExpression(R"(\[[^\]]+\])"),
		selectorFormat, 13 });

	// ==========================================
	// 3. Свойства
	// ==========================================
	QTextCharFormat propertyFormat;
	propertyFormat.setForeground(QColor("#9CDCFE"));  // голубой

	addRule({ QRegularExpression(R"([\w-]+(?=\s*:))"),
		propertyFormat, 15 });

	// ==========================================
	// 4. Значения
	// ==========================================
	QTextCharFormat valueFormat;
	valueFormat.setForeground(QColor("#CE9178"));  // оранжевый

	// Числовые значения с единицами
	addRule({ QRegularExpression(R"(\b[0-9]+\.?[0-9]*(?:px|em|rem|%|vh|vw|vmin|vmax|cm|mm|in|pt|pc|ex|ch|deg|rad|grad|turn|s|ms|Hz|kHz|dpi|dpcm|dppx)?\b)"),
		valueFormat, 16 });

	// Цвета (hex)
	addRule({ QRegularExpression(R"(#[0-9a-fA-F]{3,8}\b)"),
		valueFormat, 17 });

	// Цвета (именованные)
	addRule({ QRegularExpression(R"(\b(aliceblue|antiquewhite|aqua|aquamarine|azure|beige|bisque|black|blanchedalmond|blue|blueviolet|brown|burlywood|cadetblue|chartreuse|chocolate|coral|cornflowerblue|cornsilk|crimson|cyan|darkblue|darkcyan|darkgoldenrod|darkgray|darkgreen|darkgrey|darkkhaki|darkmagenta|darkolivegreen|darkorange|darkorchid|darkred|darksalmon|darkseagreen|darkslateblue|darkslategray|darkslategrey|darkturquoise|darkviolet|deeppink|deepskyblue|dimgray|dimgrey|dodgerblue|firebrick|floralwhite|forestgreen|fuchsia|gainsboro|ghostwhite|gold|goldenrod|gray|green|greenyellow|grey|honeydew|hotpink|indianred|indigo|ivory|khaki|lavender|lavenderblush|lawngreen|lemonchiffon|lightblue|lightcoral|lightcyan|lightgoldenrodyellow|lightgray|lightgreen|lightgrey|lightpink|lightsalmon|lightseagreen|lightskyblue|lightslategray|lightslategrey|lightsteelblue|lightyellow|lime|limegreen|linen|magenta|maroon|mediumaquamarine|mediumblue|mediumorchid|mediumpurple|mediumseagreen|mediumslateblue|mediumspringgreen|mediumturquoise|mediumvioletred|midnightblue|mintcream|mistyrose|moccasin|navajowhite|navy|oldlace|olive|olivedrab|orange|orangered|orchid|palegoldenrod|palegreen|paleturquoise|palevioletred|papayawhip|peachpuff|peru|pink|plum|powderblue|purple|red|rosybrown|royalblue|saddlebrown|salmon|sandybrown|seagreen|seashell|sienna|silver|skyblue|slateblue|slategray|slategrey|snow|springgreen|steelblue|tan|teal|thistle|tomato|turquoise|violet|wheat|white|whitesmoke|yellow|yellowgreen)\b)"),
		valueFormat, 17 });

	// Цвета (rgb, rgba, hsl, hsla)
	addRule({ QRegularExpression(R"(\b(rgb|rgba|hsl|hsla|hwb|lab|lch|oklab|oklch|color)\s*\([^)]+\))"),
		valueFormat, 17 });

	// Функции
	addRule({ QRegularExpression(R"(\b(url|calc|var|min|max|clamp|attr|cubic-bezier|linear-gradient|radial-gradient|conic-gradient|repeating-linear-gradient|repeating-radial-gradient|repeating-conic-gradient|matrix|matrix3d|perspective|rotate|rotate3d|rotateX|rotateY|rotateZ|scale|scale3d|scaleX|scaleY|scaleZ|skew|skewX|skewY|translate|translate3d|translateX|translateY|translateZ)\s*\()"),
		valueFormat, 17 });

	// Строки
	addRule({ QRegularExpression(R"("[^"]*")"),
		valueFormat, 18 });

	addRule({ QRegularExpression(R"('[^']*')"),
		valueFormat, 18 });

	// ==========================================
	// 5. Важные объявления (!important)
	// ==========================================
	QTextCharFormat importantFormat;
	importantFormat.setForeground(QColor("#FF6B6B"));  // красный
	importantFormat.setFontWeight(QFont::Bold);

	addRule({ QRegularExpression(R"(!important\b)"),
		importantFormat, 20 });

	// ==========================================
	// 6. @-правила (at-rules)
	// ==========================================
	QTextCharFormat atRuleFormat;
	atRuleFormat.setForeground(QColor("#C586C0"));  // фиолетовый
	atRuleFormat.setFontWeight(QFont::Bold);

	addRule({ QRegularExpression(R"(@[\w-]+\b)"),
		atRuleFormat, 8 });

	// ==========================================
	// 7. Медиа-запросы
	// ==========================================
	QTextCharFormat mediaFormat;
	mediaFormat.setForeground(QColor("#4EC9B0"));  // бирюзовый

	addRule({ QRegularExpression(R"(\([^)]*\))"),
		mediaFormat, 25 });

	addRule({ QRegularExpression(R"(\b(and|or|not|only|screen|print|all|speech)\b)"),
		mediaFormat, 25 });

	// ==========================================
	// 8. CSS переменные (custom properties)
	// ==========================================
	QTextCharFormat variableFormat;
	variableFormat.setForeground(QColor("#9CDCFE"));  // голубой
	variableFormat.setFontWeight(QFont::Bold);

	addRule({ QRegularExpression(R"(--[\w-]+)"),
		variableFormat, 14 });

	// Использование переменных var()
	addRule({ QRegularExpression(R"(var\s*\([^)]+\))"),
		variableFormat, 17 });
}
