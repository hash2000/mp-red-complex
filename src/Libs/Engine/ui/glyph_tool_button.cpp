#include "Libs/Engine/ui/glyph_tool_button.h"

GlyphToolButton::GlyphToolButton(const QString& glyph, const QString& toolTip,
	QWidget* parent, const char* objectName)
	: QToolButton(parent) {
	setObjectName(QString::fromLatin1(objectName));
	setText(glyph);
	setToolTip(toolTip);
	setAutoRaise(true);
	setCursor(Qt::PointingHandCursor);
	setFocusPolicy(Qt::NoFocus);
}

GlyphToolButton::~GlyphToolButton() = default;
