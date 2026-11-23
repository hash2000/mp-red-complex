#pragma once
#include <QWidget>

#include <memory>

class QHBoxLayout;
class RibbonButton;

/// Лента управления документом: группы функциональных кнопок с подписями.
///
/// Лента не хранит состояние документа и не знает о панели свойств. Кнопки
/// с параметрами создаются с идентификатором группы свойств и сообщают о
/// нажатии значка свойств сигналом `propertiesRequested(groupId)`.
class DocumentRibbon : public QWidget {
	Q_OBJECT
public:
	explicit DocumentRibbon(QWidget* parent = nullptr);
	~DocumentRibbon() override;

	/// Создаёт группу с подписью и возвращает layout для её кнопок.
	QHBoxLayout* addGroup(const QString& title);

	/// Добавляет кнопку в группу. propertiesGroup — идентификатор группы
	/// свойств действия (пустая строка — у действия нет параметров).
	RibbonButton* addButton(QHBoxLayout* group, const QString& label,
		const QString& tooltip, const QString& propertiesGroup = QString());

	void setCollapsed(bool collapsed);
	bool isCollapsed() const;

signals:
	/// Нажат значок свойств у одной из кнопок ленты.
	void propertiesRequested(const QString& groupId);
	/// Лента свёрнута или развёрнута.
	void collapsedChanged(bool collapsed);

private:
	struct Private;
	std::unique_ptr<Private> d;
};
