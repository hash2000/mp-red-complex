#pragma once
#include <QTextEdit>

class CodeEditorWidget : public QTextEdit {
	Q_OBJECT
public:
	explicit CodeEditorWidget(QWidget* parent = nullptr);
	~CodeEditorWidget() override;

	void setPath(const QString& path);
	void reloadFile();

	void setText(const QString& text);

private slots:
	void onAappendPlainText(const QString& line);

private:
	class Private;
	std::unique_ptr<Private> d;
};
