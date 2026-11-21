#pragma once
#include <QColor>
#include <QString>
#include <QWidget>

class QToolButton;

/// Редактор цвета вкладки: образец цвета, стандартный диалог выбора и сброс
/// к цвету темы.
///
/// Виджет не хранит цвет в контексте: он показывает выбранное значение
/// и сообщает о выборе сигналом `colorChanged`, а запись выполняет владелец
/// (документ через `DocumentWidget::setColor`, диалог создания — в переменные
/// нового документа).
class TabColorEditor : public QWidget {
	Q_OBJECT
public:
	explicit TabColorEditor(QWidget* parent = nullptr);
	~TabColorEditor() override;

	QColor color() const;
	/// Показывает цвет, не испуская `colorChanged`.
	void setColor(const QColor& color);

	/// Заголовок диалога выбора цвета.
	QString dialogTitle() const;
	void setDialogTitle(const QString& title);

signals:
	/// Пользователь выбрал цвет; невалидный цвет — сброс к цвету темы.
	void colorChanged(const QColor& color);

private:
	void chooseColor();
	void updateView();

	QToolButton* _button = nullptr;
	QToolButton* _reset = nullptr;
	QColor _color;
	QString _dialogTitle;
};
