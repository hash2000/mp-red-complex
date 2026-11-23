#pragma once
#include <QPlainTextEdit>
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
///
/// Наследник QPlainTextEdit, а не QTextEdit: для больших текстов важны
/// ленивая раскладка (QPlainTextDocumentLayout) и отсутствие полноценной
/// rich-text модели. QTextEdit раскладывает документ в сотни килобайт
/// секундами, из-за чего страдают изменение размера и прокрутка.
class CodeEditorWidget : public QPlainTextEdit {
	Q_OBJECT
public:
	explicit CodeEditorWidget(
		HighlightingPluginManager* pluginManager,
		FormatterPluginManager* formatterManager,
		QWidget* parent = nullptr);
	~CodeEditorWidget() override;

	void setHoghlighterByPath(const QString& path);
	void openFile(const QString& path);

	/// Полная замена текста без создания записи undo: используется при
	/// загрузке данных и применении результата форматирования.
	void setText(const QString& text);

	bool setContentType(const QString& type);
	bool setLanguage(const QString& lang);

	/// Форматирует документ. false — для текущего языка нет форматтера.
	bool formatDocument();

	/// Идёт ли сейчас форматирование (кнопка блокируется на это время).
	bool isFormatting() const;

	/// Включён ли ленивый режим подсветки (большой документ).
	bool isHighlightingLazy() const;

	/// Автоперенос длинных строк. QPlainTextEdit без переноса пересчитывает
	/// и перерисовывает всю строку целиком, поэтому строки в сотни и тысячи
	/// символов дают секунды на кадр; с переносом стоимость не зависит от
	/// длины строки. Включается автоматически, когда такая строка появляется.
	bool isAutoWrapEnabled() const;
	void setAutoWrapEnabled(bool enabled);

	/// Длина строки (в символах), начиная с которой включается автоперенос.
	static int autoWrapLineLimit();

	int fontSize() const;
	void setFontSize(int size);

	/// Плагин подсветки, выбранный для текущего содержимого.
	const HighlightingPlugin* highlightingPlugin() const;

protected:
	void wheelEvent(QWheelEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

signals:
	void formattedFailed(const QString& errorMessage);
	/// Файл прочитан (success) либо не может быть открыт.
	void fileOpened(const QString& path, bool success);
	/// Размер шрифта изменён (в том числе колесом мыши с Ctrl).
	void fontSizeChanged(int size);
	/// Форматирование началось: можно показать занятость и заблокировать кнопку.
	void formattingStarted();
	/// Форматирование завершилось (успешно или с ошибкой).
	void formattingCompleted();
	/// Изменился режим переноса строк (длинная строка появилась/исчезла).
	void lineWrappingChanged(bool wrapped);

private slots:
	void onBlockRead(const QStringList& lines);
	void onFormattingFailed(const QString& errorMessage);
	void onFormattingFinished(const QString& formattedText);
	void onTextChanged();

private:
	class Private;
	std::unique_ptr<Private> d;
};
