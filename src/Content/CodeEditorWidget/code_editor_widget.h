#pragma once
#include <QTextEdit>

class HighlightingPluginManager;
class FormatterPluginManager;

class CodeEditorWidget : public QTextEdit {
	Q_OBJECT
public:
	explicit CodeEditorWidget(
		HighlightingPluginManager* pluginManager,
		FormatterPluginManager* formatterManager,
		QWidget* parent = nullptr);
	~CodeEditorWidget() override;

	void setPath(const QString& path);
	void reloadFile();

	void setText(const QString& text);
	bool setLanguage(const QString& lang);
	void formatDocument();

private slots:
	void onBlockRead(const QStringList& lines);

private:
	class Private;
	std::unique_ptr<Private> d;
};
