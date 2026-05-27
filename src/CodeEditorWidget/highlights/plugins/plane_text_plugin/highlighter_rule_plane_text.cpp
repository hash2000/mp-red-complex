#include "CodeEditorWidget/highlights/plugins/plane_text_plugin/highlighter_rule_plane_text.h"
#include "CodeEditorWidget/highlights/highlighter.h"

namespace highlighter::rule::plane_text {

void apply(Highlighter& highlighter) {
	QTextCharFormat numberFormat;
	numberFormat.setForeground(QColor("#B5CEA8"));
	highlighter.addRule("numbers", "\\b[0-9]+\\b", numberFormat, 100);

	QTextCharFormat stringFormat;
	stringFormat.setForeground(QColor("#CE9178"));  // оранжевый
	stringFormat.setFontWeight(QFont::Normal);
	highlighter.addRule("strings", "\"[^\"]*\"", stringFormat, 100);
}

void remove(Highlighter& highlighter) {
	highlighter.deleteRule("numbers");
	highlighter.deleteRule("strings");
}

}
