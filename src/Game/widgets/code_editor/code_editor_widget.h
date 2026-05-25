#pragma once
#include <QWidget>

class CodeEditorWidget : public QWidget {
	Q_OBJECT
public:
	explicit CodeEditorWidget(QWidget* parent = nullptr);
	~CodeEditorWidget() override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
