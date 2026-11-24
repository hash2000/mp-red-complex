#pragma once
#include <QAbstractScrollArea>
#include <QByteArray>
#include <QString>
#include <QVector>

#include <memory>

/// Прототип виртуального редактора для очень больших файлов.
///
/// В памяти держится только индекс начал строк и несколько страниц текста;
/// остальное читается с диска по мере прокрутки. Пока режим только для
/// просмотра: правка, выделение и поиск появятся следующими шагами.
class PagedTextEditor : public QAbstractScrollArea {
	Q_OBJECT
public:
	explicit PagedTextEditor(QWidget* parent = nullptr);
	~PagedTextEditor() override;

	/// Открыть файл. Индексация строк идёт в фоновом потоке.
	/// Поддерживаются UTF-8 и однобайтовые кодировки (QTextCodec);
	/// UTF-16/UTF-32 в прототипе не поддерживаются.
	void openFile(const QString& path, const QString& encoding = QStringLiteral("UTF-8"));
	void closeFile();

	QString filePath() const;
	bool isIndexing() const;
	int lineCount() const;
	/// Длина самой длинной строки (в байтах; для UTF-8/ASCII совпадает
	/// с числом символов). Считается при индексации.
	int maxLineLength() const;

	int firstVisibleLine() const;
	/// Текст строки (читается из страницы; для тестов и копирования).
	QString lineText(int line) const;

signals:
	/// Индексация завершилась; false — файл не открыт или кодировка не та.
	void indexingFinished(bool success, const QString& errorMessage);
	void lineCountChanged(int lines);

protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void scrollContentsBy(int dx, int dy) override;
	void keyPressEvent(QKeyEvent* event) override;

private:
	struct Private;
	std::unique_ptr<Private> d;
};
