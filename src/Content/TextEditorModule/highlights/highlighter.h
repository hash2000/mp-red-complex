#pragma once
#include <QSyntaxHighlighter>

#include <memory>

class HighlightingPluginManager;
class HighlightingPlugin;

/// Подсветка синтаксиса по правилам темы.
///
/// Для небольших документов подсветка считается целиком (штатное поведение
/// QSyntaxHighlighter). Для больших включается ленивый режим: форматы
/// получают только видимые строки, потому что полный проход по документу
/// в сотни килобайт занимает секунды и блокирует интерфейс.
class Highlighter : public QSyntaxHighlighter {
	Q_OBJECT

public:
	/// Строки длиннее этого предела не подсвечиваются: регулярные выражения
	/// на них растут квадратично. Если такая строка есть в документе,
	/// подсветка отключается целиком (см. setEnabled).
	static constexpr int MaxHighlightedBlockLength = 2048;

	explicit Highlighter(HighlightingPluginManager* pluginManager, QTextDocument* parent = nullptr);
	~Highlighter() override;

	const HighlightingPlugin* plugin() const;
	bool setLanguage(const QString& language);
	bool setContentType(const QString& type);

	/// Подсветка выключена (вручную или из-за слишком длинной строки):
	/// форматы сняты, регулярные выражения не выполняются. При включении
	/// подсветка восстанавливается (полностью или для видимых строк).
	bool isEnabled() const;
	void setEnabled(bool enabled);

	/// Ленивый режим: подсветка только видимого диапазона строк.
	/// Переключает режим, не пересчитывая подсветку: после включения
	/// вызывайте refreshVisibleBlocks(), после выключения — rehighlight().
	bool isLazyMode() const;
	void setLazyMode(bool lazy);

	/// Видимый диапазон строк (номера блоков включительно).
	void setVisibleBlockRange(int first, int last);

	/// Пересчитать подсветку видимого диапазона (после прокрутки или
	/// изменения размера). Вызывается с задержкой, чтобы прокрутка не
	/// выполняла регулярные выражения на каждом шаге.
	void refreshVisibleBlocks();

	/// Снять форматы со всех строк (смена языка в ленивом режиме:
	/// старые цвета не должны оставаться на невидимых строках).
	void clearFormats();

protected:
	void highlightBlock(const QString& text) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
