#include "Libs/Base/extensions/text_edit_extensions.h"

namespace {
template <typename TextWidget>
TextWidget* applyTabDistance(TextWidget* obj, int distance) {
	if (!obj) {
		return nullptr;
	}

	const QFontMetrics fm(obj->font());

	const int charWidth = fm.horizontalAdvance(QLatin1Char(' '));
	const int tabWidth = distance * charWidth;

	obj->setTabStopDistance(tabWidth);
	return obj;
}
}

namespace Extensions::TextEdit {
QTextEdit* setTabDistance(QTextEdit* obj, int distance) {
	return applyTabDistance(obj, distance);
}

QPlainTextEdit* setTabDistance(QPlainTextEdit* obj, int distance) {
	return applyTabDistance(obj, distance);
}
};
