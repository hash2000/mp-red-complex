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
	enum class LineWrapPolicy {
		Auto,  ///< перенос только для длинных строк (по умолчанию)
		On,    ///< переносить всегда
		Off,   ///< не переносить; на очень длинных строках редактор тормозит
	};

	LineWrapPolicy lineWrapPolicy() const;
	void setLineWrapPolicy(LineWrapPolicy policy);

	/// Есть ли в документе строка длиннее предела автопереноса.
	bool hasLongLines() const;

	/// Режим подсветки: автоматический (с отключением на длинных строках),
	/// принудительно включённый или выключенный вручную.
	enum class HighlightingPolicy {
		Auto,  ///< включена, но отключается на слишком длинных строках
		On,    ///< включена всегда, даже при длинных строках
		Off,   ///< выключена всегда
	};

	HighlightingPolicy highlightingPolicy() const;
	void setHighlightingPolicy(HighlightingPolicy policy);

	/// Подсветка реально работает сейчас (режим не Off и нет авто-запрета).
	bool isHighlightingEnabled() const;

	/// Длина строки (в символах), начиная с которой включается автоперенос.
	static int autoWrapLineLimit();

	/// Длина строки, после которой подсветка отключается (и сама строка
	/// не подсвечивается): без этого регулярные выражения подвешивают редактор.
	static int maxHighlightedLineLength();

	int fontSize() const;
	void setFontSize(int size);

	/// Кодировка файла для чтения и сохранения.
	struct EncodingInfo {
		/// Имя кодека для QTextCodec (например, "Windows-1251").
		QString name;
		/// Подпись для пользователя.
		QString title;
	};

	/// Доступные кодировки (Qt5Compat: QTextCodec).
	static QList<EncodingInfo> availableEncodings();

	QString encoding() const;
	void setEncoding(const QString& encoding);

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
	/// Изменился режим переноса строк (длинная строка появилась/исчезла
	/// или режим выбран пользователем).
	void lineWrappingChanged(bool wrapped);
	/// В документе появились/исчезли строки длиннее autoWrapLineLimit().
	void longLinesChanged(bool hasLongLines);
	/// Подсветка включена (enabled=true) или выключена: вручную либо
	/// автоматически из-за слишком длинной строки.
	void highlightingEnabledChanged(bool enabled);
	/// Кодировка документа изменена.
	void encodingChanged(const QString& encoding);

private slots:
	void onBlockRead(const QStringList& lines);
	void onFormattingFailed(const QString& errorMessage);
	void onFormattingFinished(const QString& formattedText);
	void onTextChanged();

private:
	class Private;
	std::unique_ptr<Private> d;
};
