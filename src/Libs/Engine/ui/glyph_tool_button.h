#pragma once

#include <QToolButton>

class GlyphToolButton : public QToolButton {
	Q_OBJECT;
public:
	explicit GlyphToolButton(const QString& glyph, const QString& toolTip,
		QWidget* parent = nullptr, const char* objectName = "HeaderButton");
	~GlyphToolButton() override;

};
