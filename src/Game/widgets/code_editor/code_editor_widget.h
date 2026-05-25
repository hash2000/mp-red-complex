#pragma once
#include <QTextEdit>

class CodeEditorWidget : public QTextEdit {
	Q_OBJECT
public:
	explicit CodeEditorWidget(QWidget* parent = nullptr);
	~CodeEditorWidget() override;

	void setPath(const QString& path);

private:
	class Private;
	std::unique_ptr<Private> d;
};
