#include "Libs/Base/extensions/text_edit_extensions.h"

namespace Extensions::TextEdit {
QTextEdit* setTabDistance(QTextEdit* obj, int distance) {
	if (!obj) {
		return nullptr;
	}

	const auto font = obj->font();
	QFontMetrics fm(font);
	const auto avgCharWidth = fm.averageCharWidth();
	const auto tabWidth = distance * avgCharWidth;
	obj->setTabStopDistance(tabWidth);
	return obj;
}
}
