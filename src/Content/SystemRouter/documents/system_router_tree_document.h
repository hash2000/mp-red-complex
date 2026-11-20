#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_node.h"
#include "Libs/Engine/ui/doc_widget.h"

#include <memory>

class LazyTreeWidget;
class SystemRouterCommandService;
class SystemRouterService;
struct SystemRouterScriptPreview;

/// Документ «Системный маршрутизатор».
///
/// Основной виджет — LazyTreeWidget: дерево узлов с поиском и управляющими
/// кнопками. Документ владеет виджетом, хранит выбранный узел в переменной
/// контекста `selectedNodeId`, выполняет скрипты по двойному щелчку и отдаёт
/// операции панели свойств.
class SystemRouterTreeDocument final : public DocumentWidget {
	Q_OBJECT
public:
	explicit SystemRouterTreeDocument(SystemRouterService* service,
		SystemRouterCommandService* commandService = nullptr, QWidget* parent = nullptr);
	~SystemRouterTreeDocument() override;

	LazyTreeWidget* tree() const;

	/// Открыть документ параметров узла из контекстного меню.
	void openNodeProperties(const LazyTreeNodePtr& node);

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
	void onNodeActivated(const LazyTreeNodePtr& node);
	void executePreview(const SystemRouterScriptPreview& preview);

	struct Private;
	std::unique_ptr<Private> d;
};
