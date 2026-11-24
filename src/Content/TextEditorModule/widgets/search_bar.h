#pragma once
#include <QWidget>

class QLabel;
class QLineEdit;

/// Общая строка поиска для документов редактора кода и просмотрщика
/// больших файлов: поле запроса, счётчик «текущее/всего», переходы и
/// закрытие. Внешний вид задаётся темой по objectName `TextEditorSearchBar`.
class SearchBar : public QWidget {
	Q_OBJECT
public:
	explicit SearchBar(QWidget* parent = nullptr);

	QLineEdit* input() const;
	QLabel* counter() const;

	void focusSearch();
	/// currentIndex — с нуля; -1 означает «позиция не выбрана».
	void setCounter(int currentIndex, int total);
	void setBusy();
	void setMessage(const QString& text);

signals:
	void queryChanged(const QString& text);
	void nextRequested();
	void previousRequested();
	void closeRequested();

protected:
	bool eventFilter(QObject* watched, QEvent* event) override;

private:
	QLineEdit* _input = nullptr;
	QLabel* _counter = nullptr;
};
