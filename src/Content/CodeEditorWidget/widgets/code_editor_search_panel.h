#pragma once
#include <QWidget>

#include <memory>

class QPlainTextEdit;

/// Панель поиска над редактором кода.
///
/// Показывает поле запроса, счётчик совпадений и переходы по ним,
/// подсвечивает вхождения в документе через ExtraSelections
/// (текущее — ярче; при большом числе совпадений подсвечивается окно
/// вокруг текущего). При скрытии подсветка снимается.
class CodeEditorSearchPanel : public QWidget {
	Q_OBJECT
public:
	enum class Option {
		RegularExpression,     ///< поиск по регулярному выражению
		SimplifiedExpression,  ///< обычный текст с \n, \t, \r
		CaseSensitive,         ///< учитывать регистр
		WholeWord,             ///< слово целиком
	};

	explicit CodeEditorSearchPanel(QPlainTextEdit* editor, QWidget* parent = nullptr);
	~CodeEditorSearchPanel() override;

	void setOption(Option option, bool enabled);
	bool option(Option option) const;

	/// Показать панель и поставить фокус в поле запроса.
	void focusSearch();
	/// Снять подсветку и очистить состояние поиска (панель не скрывает).
	void clearSearch();

	void activateNext();
	void activatePrevious();

	/// Запрос поиска (используется панелью и тестами).
	void setQuery(const QString& text);
	QString query() const;

	/// Пересчитать совпадения (например, после правки текста).
	void refresh();

	int matchCount() const;
	/// Сколько вхождений сейчас подсвечено в редакторе.
	int highlightedCount() const;

signals:
	/// Пользователь закрыл панель (Escape или кнопка закрытия).
	void closed();

protected:
	void keyPressEvent(QKeyEvent* event) override;

private:
	struct Match {
		int start = 0;
		int length = 0;
	};

	void setupUi();
	void updateCounter();
	void applyHighlights();
	void moveToCurrent();
	QString buildPattern(bool& valid) const;

	class Private;
	std::unique_ptr<Private> d;
};
