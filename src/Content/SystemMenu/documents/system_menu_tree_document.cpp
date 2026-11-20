#include "Content/SystemMenu/documents/system_menu_tree_document.h"
#include "Content/SystemMenu/documents/system_menu_node_properties_document.h"
#include "Content/SystemMenu/services/system_menu_service.h"
#include "Content/SystemMenu/widgets/system_menu_tree_widget.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_view.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_widget.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/ui/main_window.h"

#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QVariantMap>

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
	connect(d->tree, &SystemMenuTreeWidget::propertiesRequested, this,
		&SystemMenuTreeDocument::openNodeProperties);
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

void SystemMenuTreeDocument::openNodeProperties(const LazyTreeNodePtr& node) {
	if (!d->service || !node) {
		return;
	}

	const auto nodeId = node->id();
	if (!nodeId.isValid() || nodeId.isNull()) {
		return;
	}

	auto context = this->context();
	auto* controller = context ? context->commandController() : nullptr;
	auto* window = controller ? controller->mainWindow() : nullptr;
	if (!window) {
		emit errorOccurred(tr("Не удалось открыть параметры узла: окно приложения недоступно"));
		return;
	}

	auto* service = d->service;
	const auto title = tr("Параметры: %1").arg(node->name());
	const auto documentId = QStringLiteral("system-menu-node-%1").arg(nodeId.toString());
	const QVariantMap variables = {
		{ QStringLiteral("nodeId"), nodeId },
		{ QStringLiteral("nodeName"), node->name() }
	};

	auto* document = window->createWidget(documentId, MainWindow::WidgetKind::Document,
		title, QStringLiteral("🛠"),
		[service, nodeId]() {
			return new SystemMenuNodePropertiesDocument(service, nodeId);
		},
		tr("Параметры узла системного меню"),
		QStringLiteral("system-menu-node-properties"),
		context, variables);

	if (!document) {
		emit errorOccurred(tr("Не удалось открыть документ параметров узла «%1»").arg(node->name()));
	}
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
