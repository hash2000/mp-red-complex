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

	void setHoghlighterByPath(const QString& path);
	void openFile(const QString& path);

	QString getText() const;
	void setText(const QString& text);
	bool setLanguage(const QString& lang);
	void formatDocument();

signals:
	void formattedFailed(const QString& errorMessage);

private slots:
	void onBlockRead(const QStringList& lines);
	void onFormattingFailed(const QString& errorMessage);
	void onFormattingFinished(const QString& formattedText);
	void onTextChanged();

private:
	class Private;
	std::unique_ptr<Private> d;
};
