#pragma once
#include <QString>
#include <QWidget>

#include <functional>
#include <memory>

/// Сворачиваемый раздел панели «Свойства».
///
/// Заголовок группы — кнопка с треугольником и подписью; содержимое можно
/// передать готовым (`setContent`) или фабрикой (`setContentFactory`), которая
/// вызывается при первом раскрытии. Тяжёлые списки (например, переменные
/// контекста документа) не строятся, пока пользователь не откроет группу.
class PropertiesGroupWidget : public QWidget {
	Q_OBJECT
public:
	explicit PropertiesGroupWidget(const QString& title, QWidget* parent = nullptr);
	~PropertiesGroupWidget() override;

	/// Идентификатор группы в панели свойств (например, "search").
	QString groupId() const;
	void setGroupId(const QString& id);

	QString title() const;
	void setTitle(const QString& title);

	/// Готовое содержимое группы (группа становится его владельцем).
	void setContent(QWidget* content);

	/// Ленивое содержимое: создаётся при первом раскрытии группы.
	void setContentFactory(std::function<QWidget*()> factory);

	/// Созданное содержимое; nullptr, пока группа не раскрыта.
	QWidget* content() const;

	bool isExpanded() const;
	void setExpanded(bool expanded);

signals:
	/// Группа раскрыта или свёрнута (в том числе программно).
	void expandedChanged(bool expanded);

private:
	void updateHeader();

	struct Private;
	std::unique_ptr<Private> d;
};
