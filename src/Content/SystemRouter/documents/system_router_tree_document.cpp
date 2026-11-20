#include "Content/SystemRouter/documents/system_router_tree_document.h"
#include "Content/SystemRouter/documents/system_router_node_properties_document.h"
#include "Content/SystemRouter/services/system_router_command_service.h"
#include "Content/SystemRouter/services/system_router_service.h"
#include "Content/SystemRouter/widgets/system_router_script_dialog.h"
#include "Content/SystemRouter/widgets/system_router_tree_widget.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_view.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_widget.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/ui/main_window.h"

#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QVariantMap>

namespace {
const QString kSelectedNodeVariable = QStringLiteral("selectedNodeId");
}

struct SystemRouterTreeDocument::Private {
	SystemRouterService* service = nullptr;
	SystemRouterCommandService* commandService = nullptr;
	SystemRouterTreeWidget* tree = nullptr;
	bool updatingVariable = false;
};

SystemRouterTreeDocument::SystemRouterTreeDocument(SystemRouterService* service,
	SystemRouterCommandService* commandService, QWidget* parent)
	: DocumentWidget(parent)
	, d(std::make_unique<Private>()) {
	d->service = service;
	d->commandService = commandService;
	d->tree = new SystemRouterTreeWidget(service, this);

	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(d->tree);

	setGlyph(QStringLiteral("🧭"));
	setTitle(tr("Системный маршрутизатор"));

	connect(d->tree, &LazyTreeWidget::nodeActivated, this, &SystemRouterTreeDocument::nodeActivated);
	connect(d->tree, &LazyTreeWidget::nodeActivated, this, &SystemRouterTreeDocument::onNodeActivated);
	connect(d->tree, &LazyTreeWidget::errorOccurred, this, &SystemRouterTreeDocument::errorOccurred);
	connect(d->tree, &SystemRouterTreeWidget::propertiesRequested, this,
		&SystemRouterTreeDocument::openNodeProperties);
	connect(d->tree, &LazyTreeWidget::selectionChanged, this,
		[this](const LazyTreeNodePtr& node) {
			d->updatingVariable = true;
			setVariable(kSelectedNodeVariable, node ? node->id() : QVariant());
			d->updatingVariable = false;
			emit propertiesChanged();
		});
}

SystemRouterTreeDocument::~SystemRouterTreeDocument() = default;

LazyTreeWidget* SystemRouterTreeDocument::tree() const {
	return d->tree;
}

void SystemRouterTreeDocument::openNodeProperties(const LazyTreeNodePtr& node) {
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
	const auto documentId = QStringLiteral("system-router-node-%1").arg(nodeId.toString());
	const QVariantMap variables = {
		{ QStringLiteral("nodeId"), nodeId },
		{ QStringLiteral("nodeName"), node->name() }
	};

	auto* document = window->createWidget(documentId, MainWindow::WidgetKind::Document,
		title, QStringLiteral("🛠"),
		[service, nodeId]() {
			return new SystemRouterNodePropertiesDocument(service, nodeId);
		},
		tr("Параметры узла системного маршрутизатора"),
		QStringLiteral("system-router-node-properties"),
		context, variables);

	if (!document) {
		emit errorOccurred(tr("Не удалось открыть документ параметров узла «%1»").arg(node->name()));
	}
}

void SystemRouterTreeDocument::onNodeActivated(const LazyTreeNodePtr& node) {
	if (!node) {
		return;
	}

	// Без сервиса команд или без привязанных скриптов сохраняем прежнее
	// поведение: двойной щелчок открывает параметры узла.
	if (!d->commandService) {
		openNodeProperties(node);
		return;
	}

	const auto previews = d->commandService->previewsForNode(node);
	if (previews.isEmpty()) {
		openNodeProperties(node);
		return;
	}

	if (previews.size() == 1) {
		executePreview(previews.first());
		return;
	}

	SystemRouterScriptDialog dialog(previews, this);
	if (dialog.exec() == QDialog::Accepted) {
		executePreview(dialog.selectedPreview());
	}
}

void SystemRouterTreeDocument::executePreview(const SystemRouterScriptPreview& preview) {
	if (!preview.commandFound) {
		QMessageBox::warning(this, tr("Скрипт"),
			preview.error.isEmpty() ? tr("Команда не найдена") : preview.error);
		return;
	}

	auto context = this->context();
	auto* controller = context ? context->commandController() : nullptr;
	if (!controller) {
		emit errorOccurred(tr("Не удалось выполнить скрипт: контроллер команд недоступен"));
		return;
	}

	if (!controller->execute(preview.renderedScript, this)) {
		QMessageBox::warning(this, tr("Скрипт"),
			tr("Не удалось выполнить скрипт «%1». Подробности — в консоли.")
				.arg(preview.propertyName));
	}
}

void SystemRouterTreeDocument::refresh() {
	if (d->tree) {
		d->tree->refreshAll();
	}
}

bool SystemRouterTreeDocument::save() {
	// Все операции дерева сохраняются сразу при завершении редактирования.
	return true;
}

QWidget* SystemRouterTreeDocument::createPropertiesEditor(QWidget* parent) {
	auto* editor = DocumentWidget::createPropertiesEditor(parent);
	auto* form = qobject_cast<QFormLayout*>(editor->layout());
	if (!form) {
		return editor;
	}

	auto* nodeLabel = new QLabel(this);
	nodeLabel->setObjectName(QStringLiteral("SystemRouterSelectedNode"));
	nodeLabel->setText(hasVariable(kSelectedNodeVariable)
		? variable(kSelectedNodeVariable).toString()
		: tr("не выбран"));

	form->addRow(tr("Узел:"), nodeLabel);
	return editor;
}

void SystemRouterTreeDocument::onContextAttached() {
	if (!d->tree || !hasVariable(kSelectedNodeVariable)) {
		return;
	}

	const auto id = variable(kSelectedNodeVariable);
	if (id.isValid() && !id.isNull()) {
		d->tree->treeView()->expandAndSelectNode(id);
	}
}

void SystemRouterTreeDocument::onContextVariableChanged(const QString& name, const QVariant& value) {
	if (name != kSelectedNodeVariable || !d->tree || d->updatingVariable) {
		return;
	}

	if (value.isValid() && !value.isNull()) {
		d->tree->treeView()->expandAndSelectNode(value);
	}
}
