#include "Content/SystemMenu/documents/system_menu_tree_document.h"
#include "Content/SystemMenu/services/system_menu_service.h"
#include "Content/SystemMenu/widgets/system_menu_tree_widget.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_view.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_widget.h"

#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace {
const QString kSelectedNodeVariable = QStringLiteral("selectedNodeId");
}

struct SystemMenuTreeDocument::Private {
	SystemMenuService* service = nullptr;
	SystemMenuTreeWidget* tree = nullptr;
	bool updatingVariable = false;
};

SystemMenuTreeDocument::SystemMenuTreeDocument(SystemMenuService* service, QWidget* parent)
	: DocumentWidget(parent)
	, d(std::make_unique<Private>()) {
	d->service = service;
	d->tree = new SystemMenuTreeWidget(service, this);

	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(d->tree);

	setGlyph(QStringLiteral("🏢"));
	setTitle(tr("Системное меню"));

	connect(d->tree, &LazyTreeWidget::nodeActivated, this, &SystemMenuTreeDocument::nodeActivated);
	connect(d->tree, &LazyTreeWidget::errorOccurred, this, &SystemMenuTreeDocument::errorOccurred);
	connect(d->tree, &LazyTreeWidget::selectionChanged, this,
		[this](const LazyTreeNodePtr& node) {
			d->updatingVariable = true;
			setVariable(kSelectedNodeVariable, node ? node->id() : QVariant());
			d->updatingVariable = false;
			emit propertiesChanged();
		});
}

SystemMenuTreeDocument::~SystemMenuTreeDocument() = default;

LazyTreeWidget* SystemMenuTreeDocument::tree() const {
	return d->tree;
}

void SystemMenuTreeDocument::refresh() {
	if (d->tree) {
		d->tree->refreshAll();
	}
}

bool SystemMenuTreeDocument::save() {
	// Все операции дерева сохраняются сразу при завершении редактирования.
	return true;
}

QWidget* SystemMenuTreeDocument::createPropertiesEditor(QWidget* parent) {
	auto* editor = DocumentWidget::createPropertiesEditor(parent);
	auto* form = qobject_cast<QFormLayout*>(editor->layout());
	if (!form) {
		return editor;
	}

	auto* nodeLabel = new QLabel(this);
	nodeLabel->setObjectName(QStringLiteral("SystemMenuSelectedNode"));
	nodeLabel->setText(hasVariable(kSelectedNodeVariable)
		? variable(kSelectedNodeVariable).toString()
		: tr("не выбран"));

	form->addRow(tr("Пункт меню:"), nodeLabel);
	return editor;
}

void SystemMenuTreeDocument::onContextAttached() {
	if (!d->tree || !hasVariable(kSelectedNodeVariable)) {
		return;
	}

	const auto id = variable(kSelectedNodeVariable);
	if (id.isValid() && !id.isNull()) {
		d->tree->treeView()->expandAndSelectNode(id);
	}
}

void SystemMenuTreeDocument::onContextVariableChanged(const QString& name, const QVariant& value) {
	if (name != kSelectedNodeVariable || !d->tree || d->updatingVariable) {
		return;
	}

	if (value.isValid() && !value.isNull()) {
		d->tree->treeView()->expandAndSelectNode(value);
	}
}
