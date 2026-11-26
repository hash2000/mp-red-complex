#include "Content/SystemRouter/documents/system_router_tree_document.h"
#include "Content/SystemRouter/documents/system_router_node_properties_document.h"
#include "Content/SystemRouter/documents/system_router_node_view_document.h"
#include "Content/SystemRouter/models/system_router_mime.h"
#include "Content/SystemRouter/services/system_router_blob_service.h"
#include "Content/SystemRouter/services/system_router_command_service.h"
#include "Content/SystemRouter/services/system_router_service.h"
#include "Content/SystemRouter/widgets/system_router_script_dialog.h"
#include "Content/SystemRouter/widgets/system_router_tree_widget.h"
#include "Libs/BaseWidgets/properties/property_mime.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_node_glyph.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_view.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_widget.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/command_processor.h"
#include "Libs/Engine/ui/main_window.h"

#include <QFormLayout>
#include <QLabel>
#include <QStandardItem>
#include <QUuid>
#include <QVBoxLayout>
#include <QVariantMap>

namespace {
const QString kSelectedNodeVariable = QStringLiteral("selectedNodeId");

bool hasMime(const QStringList& mimeTypes, const char* mimeType) {
	for (const auto& raw : mimeTypes) {
		if (raw.trimmed().compare(QLatin1String(mimeType), Qt::CaseInsensitive) == 0) {
			return true;
		}
	}

	return false;
}
}

struct SystemRouterTreeDocument::Private {
	SystemRouterService* service = nullptr;
	SystemRouterCommandService* commandService = nullptr;
	SystemRouterBlobService* blobService = nullptr;
	SystemRouterTreeWidget* tree = nullptr;
	bool updatingVariable = false;
};

SystemRouterTreeDocument::SystemRouterTreeDocument(SystemRouterService* service,
	SystemRouterCommandService* commandService, SystemRouterBlobService* blobService,
	QWidget* parent)
	: DocumentWidget(parent)
	, d(std::make_unique<Private>()) {
	d->service = service;
	d->commandService = commandService;
	d->blobService = blobService;
	d->tree = new SystemRouterTreeWidget(service, this);

	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(d->tree);

	setGlyph(QStringLiteral("🧭"));
	setTitle(tr("Системный маршрутизатор"));

	connect(d->tree, &LazyTreeWidget::nodeActivated, this, &SystemRouterTreeDocument::nodeActivated);
	connect(d->tree, &LazyTreeWidget::nodeActivated, this, &SystemRouterTreeDocument::onNodeActivated);
	connect(d->tree, &LazyTreeWidget::errorOccurred, this, [this](const QString& message) {
		if (auto* controller = context() ? context()->commandController() : nullptr) {
			controller->printError(message);
		}
		emit errorOccurred(message);
	});
	connect(d->tree, &SystemRouterTreeWidget::propertiesRequested, this,
		&SystemRouterTreeDocument::openNodeProperties);
	connect(d->tree, &SystemRouterTreeWidget::viewRequested, this,
		&SystemRouterTreeDocument::openNodeView);
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

	auto* service = d->service;
	auto* blobService = d->blobService;
	openNodeDocument(node,
		QStringLiteral("system-router-node-%1").arg(nodeId.toString()),
		tr("Параметры: %1").arg(node->name()),
		QStringLiteral("🛠"),
		QStringLiteral("system-router-node-properties"),
		[service, blobService, nodeId]() {
			return new SystemRouterNodePropertiesDocument(service, blobService, nodeId);
		});
}

void SystemRouterTreeDocument::openNodeView(const LazyTreeNodePtr& node) {
	if (!d->service || !node) {
		return;
	}

	const auto nodeId = node->id();
	if (!nodeId.isValid() || nodeId.isNull()) {
		return;
	}

	auto* service = d->service;
	auto* commandService = d->commandService;
	openNodeDocument(node,
		QStringLiteral("system-router-node-view-%1").arg(nodeId.toString()),
		tr("Просмотр: %1").arg(node->name()),
		QStringLiteral("🔍"),
		QStringLiteral("system-router-node-view"),
		[service, commandService, nodeId]() {
			return new SystemRouterNodeViewDocument(service, commandService, nodeId);
		});
}

void SystemRouterTreeDocument::openNodeDocument(const LazyTreeNodePtr& node,
	const QString& documentId, const QString& title, const QString& glyph,
	const QString& type, const std::function<QWidget*()>& factory) {
	auto context = this->context();
	auto* controller = context ? context->commandController() : nullptr;
	auto* window = controller ? controller->mainWindow() : nullptr;
	if (!window) {
		const auto message = tr("Не удалось открыть документ узла «%1»: окно приложения недоступно")
			.arg(node->name());
		if (controller) {
			controller->printError(message);
		}
		emit errorOccurred(message);
		return;
	}

	const QVariantMap variables = {
		{ QStringLiteral("nodeId"), node->id() },
		{ QStringLiteral("nodeName"), node->name() }
	};

	auto* document = window->createWidget(documentId, MainWindow::WidgetKind::Document,
		title, glyph, factory, tr("Узел системного маршрутизатора"), type, context,
		variables);

	if (!document) {
		const auto message = tr("Не удалось открыть документ узла «%1»").arg(node->name());
		if (controller) {
			controller->printError(message);
		}
		emit errorOccurred(message);
	}
}

bool SystemRouterTreeDocument::hasDisplayedParameters(const LazyTreeNodePtr& node) const {
	if (!node) {
		return false;
	}

	const auto names = node->properties().keys();
	for (const auto& name : names) {
		// Глиф — оформление узла, а не его параметр.
		if (LazyTreeNodeGlyph::isProperty(name)) {
			continue;
		}

		if (PropertyMime::isHidden(node->propertyMimeTypes(name))) {
			continue;
		}

		return true;
	}

	return false;
}

void SystemRouterTreeDocument::onNodeActivated(const LazyTreeNodePtr& node) {
	if (!node) {
		return;
	}

	// Привязанные команды: диалог предлагает запустить скрипт или посмотреть
	// параметры узла. Диалог показывается и для одной команды.
	const auto previews = d->commandService
		? d->commandService->previewsForNode(node)
		: QList<SystemRouterScriptPreview>();
	if (!previews.isEmpty()) {
		SystemRouterScriptDialog dialog(previews, this);
		const int result = dialog.exec();
		if (result == SystemRouterScriptDialog::ViewResult) {
			openNodeView(node);
			return;
		}
		if (result == QDialog::Accepted) {
			executePreview(node, dialog.selectedPreview());
		}
		return;
	}

	// Узел с параметрами: активация открывает документ просмотра.
	if (hasDisplayedParameters(node)) {
		openNodeView(node);
		return;
	}

	// Без команд и параметров узел ведёт себя как обычный элемент дерева:
	// двойной щелчок разворачивает/сворачивает его.
	toggleNodeExpansion(node);
}

void SystemRouterTreeDocument::toggleNodeExpansion(const LazyTreeNodePtr& node) {
	if (!d->tree || !node) {
		return;
	}

	auto* view = d->tree->treeView();
	if (!view) {
		return;
	}

	auto* item = view->findNode(node->id());
	if (!item || item->rowCount() == 0) {
		return;
	}

	const QModelIndex index = item->index();
	view->setExpanded(index, !view->isExpanded(index));
}

void SystemRouterTreeDocument::executePreview(const LazyTreeNodePtr& node,
	const SystemRouterScriptPreview& preview) {
	auto context = this->context();
	auto* controller = context ? context->commandController() : nullptr;
	if (!controller) {
		emit errorOccurred(tr("Не удалось выполнить скрипт: контроллер команд недоступен"));
		return;
	}

	if (!preview.commandFound) {
		controller->printError(preview.error.isEmpty()
			? tr("Команда не найдена")
			: preview.error);
		return;
	}

	auto* processor = controller->commandProcessor();
	if (!processor) {
		controller->printError(tr("Процессор команд недоступен"));
		return;
	}

	// Параметры с application/x-mp-context-variable передаются в контекст
	// команды как переменные, а не подставляются в текст скрипта.
	auto executionContext = context->createScopedContext(
		QStringLiteral("system-router-command-%1")
			.arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
	if (node) {
		const auto properties = node->properties();
		for (auto it = properties.constBegin(); it != properties.constEnd(); it++) {
			const auto mimeTypes = node->propertyMimeTypes(it.key());
			if (!hasMime(mimeTypes, SystemRouterMime::kContextVariable)) {
				continue;
			}

			executionContext->setVariable(it.key(), it.value());
			executionContext->setVariableMimeTypes(it.key(), mimeTypes);
		}
	}

	if (!preview.missingParameters.isEmpty()) {
		controller->printWarning(tr("Не найдены параметры: %1")
			.arg(preview.missingParameters.join(QStringLiteral(", "))));
	}

	if (!processor->execute(preview.renderedScript, executionContext)) {
		controller->printError(tr("Не удалось выполнить скрипт «%1»")
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
