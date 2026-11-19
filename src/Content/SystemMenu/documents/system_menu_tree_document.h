#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_node.h"
#include "Libs/Engine/ui/doc_widget.h"

#include <memory>

class LazyTreeWidget;
class SystemMenuService;

/// Документ «Системное меню».
///
/// Основной виджет — LazyTreeWidget: дерево пунктов меню с поиском и
/// управляющими кнопками. Документ владеет виджетом, хранит выбранный узел
/// в переменной контекста `selectedNodeId` и отдаёт операции панели свойств.
class SystemMenuTreeDocument final : public DocumentWidget {
	Q_OBJECT
public:
	explicit SystemMenuTreeDocument(SystemMenuService* service, QWidget* parent = nullptr);
	~SystemMenuTreeDocument() override;

	LazyTreeWidget* tree() const;

	void refresh() override;
	bool save() override;
	QWidget* createPropertiesEditor(QWidget* parent) override;

signals:
	void nodeActivated(const LazyTreeNodePtr& node);
	void errorOccurred(const QString& message);

protected:
	void onContextAttached() override;
	void onContextVariableChanged(const QString& name, const QVariant& value) override;

private:
	struct Private;
	std::unique_ptr<Private> d;
};
