#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_node.h"

#include <QStringList>
#include <QVariant>
#include <QWidget>

#include <memory>

class SystemRouterCommandService;
class SystemRouterService;

/// Виджет просмотра параметров узла (только чтение).
///
/// Показывает `ILazyTreeNode::properties()` узла, отсортированные по имени.
/// Вид виджета значения выбирается по mime-типу параметра: текст, число,
/// флажок, цвет, изображение, JSON/XML, многострочный текст или ссылка на
/// команду. У каждой строки есть две кнопки — копировать имя параметра и
/// копировать его значение.
///
/// Параметры с `application/x-mp-secret` показываются без значения, а с
/// `application/x-mp-hidden` не показываются совсем. Служебное свойство глифа
/// узла в список не попадает.
///
/// Виджет знает сервисы маршрутизатора, но не зависит от документа и его
/// контекста: документ оборачивает виджет и ведёт состояние.
class SystemRouterNodeViewWidget final : public QWidget {
	Q_OBJECT
public:
	explicit SystemRouterNodeViewWidget(SystemRouterService* service,
		SystemRouterCommandService* commandService = nullptr, QWidget* parent = nullptr);
	~SystemRouterNodeViewWidget() override;

	/// Узел для просмотра; пустой идентификатор очищает список параметров.
	void setNodeId(const QVariant& nodeId);
	QVariant nodeId() const;
	LazyTreeNodePtr node() const;

	/// Перечитать узел из сервиса и перестроить список параметров.
	void refresh();

	/// Имена показанных параметров в порядке отображения.
	QStringList propertyNames() const;

signals:
	/// Значение или имя параметра скопировано в буфер обмена.
	void copied(const QString& text);

	/// Узел не найден или не удалось прочитать его параметры.
	void errorOccurred(const QString& message);

private:
	class Private;
	std::unique_ptr<Private> d;
};
