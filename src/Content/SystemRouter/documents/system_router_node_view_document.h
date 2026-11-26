#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_node.h"
#include "Libs/Engine/ui/doc_widget.h"

#include <QVariant>
#include <memory>

class SystemRouterCommandService;
class SystemRouterNodeViewWidget;
class SystemRouterService;

/// Документ просмотра узла SystemRouter.
///
/// Показывает параметры узла только для чтения в SystemRouterNodeViewWidget:
/// вид виджета значения выбирается по mime-типу, у каждого параметра есть
/// кнопки копирования имени и значения. Секретные параметры
/// (`application/x-mp-secret`) показываются без значения, скрытые
/// (`application/x-mp-hidden`) не показываются.
///
/// Документ открывается при активации узла с параметрами, кнопкой «Просмотр»
/// в диалоге скриптов и пунктом контекстного меню дерева. Идентификатор узла
/// хранится в переменной контекста `nodeId`; при изменении узла в документе
/// свойств просмотр обновляется.
class SystemRouterNodeViewDocument final : public DocumentWidget {
	Q_OBJECT
public:
	/// @param nodeId идентификатор узла; пустой — узел будет прочитан из
	///        переменной контекста документа `nodeId` при привязке контекста.
	explicit SystemRouterNodeViewDocument(SystemRouterService* service,
		SystemRouterCommandService* commandService = nullptr, const QVariant& nodeId = {},
		QWidget* parent = nullptr);
	~SystemRouterNodeViewDocument() override;

	QVariant nodeId() const;
	LazyTreeNodePtr node() const;

	/// Виджет просмотра параметров (для панели свойств и тестов).
	SystemRouterNodeViewWidget* view() const;

	void refresh() override;
	bool save() override;

signals:
	void errorOccurred(const QString& message);

protected:
	void onContextAttached() override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
