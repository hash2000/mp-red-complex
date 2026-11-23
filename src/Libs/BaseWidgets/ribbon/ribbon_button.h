#pragma once
#include <QString>
#include <QWidget>

#include <memory>

class QToolButton;

/// Кнопка ленты документа: большая часть — само действие, маленький значок
/// в правом нижнем углу (если у действия есть параметры) открывает свойства
/// действия в панели «Свойства».
///
/// Кнопка не знает, где находится панель свойств: она сообщает только
/// идентификатор группы сигналом `propertiesRequested`, а показывает панель
/// документ-владелец.
class RibbonButton : public QWidget {
	Q_OBJECT
public:
	explicit RibbonButton(const QString& label = QString(),
		const QString& tooltip = QString(), QWidget* parent = nullptr);
	~RibbonButton() override;

	QString label() const;
	void setLabel(const QString& label);

	/// Подсказка основной кнопки (и самой кнопки ленты).
	void setToolTip(const QString& tooltip);

	void setCheckable(bool checkable);
	bool isCheckable() const;
	void setChecked(bool checked);
	bool isChecked() const;

	/// Идентификатор группы свойств; пустая строка — значка свойств нет.
	QString propertiesGroup() const;
	void setPropertiesGroup(const QString& groupId);

	QToolButton* mainButton() const;
	QToolButton* propertiesButton() const;

signals:
	void clicked();
	void toggled(bool checked);
	/// Пользователь нажал значок свойств в правом нижнем углу.
	void propertiesRequested(const QString& groupId);

protected:
	void resizeEvent(QResizeEvent* event) override;

private:
	struct Private;
	std::unique_ptr<Private> d;
};
