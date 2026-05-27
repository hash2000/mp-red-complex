#pragma once
#include <QTextEdit>

class HighlightingPluginManager;

class CodeEditorWidget : public QTextEdit {
	Q_OBJECT
public:
	explicit CodeEditorWidget(HighlightingPluginManager* pluginManager, QWidget* parent = nullptr);
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
