#pragma once
#include <QTextEdit>
#include <QWheelEvent>
#include <memory>

class HighlightingPlugin;
class HighlightingPluginManager;
class FormatterPluginManager;

/// Базовый редактор кода: подсветка текста по типу разметки и
/// форматирование внешней утилитой.
///
/// Виджет не является документом и не хранит путь к файлу: состояние
/// документа (путь, признак изменения, сохранение) ведёт обёртка
/// CodeEditorDocument.
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

	void setText(const QString& text);

	bool setContentType(const QString& type);
	bool setLanguage(const QString& lang);

	/// Форматирует документ. false — для текущего языка нет форматтера.
	bool formatDocument();

	int fontSize() const;
	void setFontSize(int size);

	/// Плагин подсветки, выбранный для текущего содержимого.
	const HighlightingPlugin* highlightingPlugin() const;

protected:
	void wheelEvent(QWheelEvent* event) override;

signals:
	void formattedFailed(const QString& errorMessage);
	/// Файл прочитан (success) либо не может быть открыт.
	void fileOpened(const QString& path, bool success);
	/// Размер шрифта изменён (в том числе колесом мыши с Ctrl).
	void fontSizeChanged(int size);

private slots:
	void onBlockRead(const QStringList& lines);
	void onFormattingFailed(const QString& errorMessage);
	void onFormattingFinished(const QString& formattedText);
	void onTextChanged();

private:
	class Private;
	std::unique_ptr<Private> d;
};
