#include "Content/SystemRouter/documents/system_router_node_view_document.h"
#include "Content/SystemRouter/services/system_router_service.h"
#include "Content/SystemRouter/widgets/system_router_node_view_widget.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"

#include <QVBoxLayout>

namespace {
const QString kNodeIdVariable = QStringLiteral("nodeId");
}

class SystemRouterNodeViewDocument::Private {
public:
	explicit Private(SystemRouterNodeViewDocument* parent)
		: q(parent) {
	}

	SystemRouterNodeViewDocument* q;
	SystemRouterService* service = nullptr;
	QVariant nodeId;
	SystemRouterNodeViewWidget* view = nullptr;
	/// Заголовок, который документ сгенерировал сам: если пользователь
	/// переименовал вкладку, его значение не перезаписывается.
	QString generatedTitle;

	void setupUi(SystemRouterCommandService* commandService);
	void loadNode();
	void updateTitle();
	void reportWarning(const QString& message);
};

void SystemRouterNodeViewDocument::Private::setupUi(SystemRouterCommandService* commandService) {
	auto* layout = new QVBoxLayout(q);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	view = new SystemRouterNodeViewWidget(service, commandService, q);
	layout->addWidget(view);

	QObject::connect(view, &SystemRouterNodeViewWidget::errorOccurred, q,
		[this](const QString& message) {
			reportWarning(message);
		});
}

void SystemRouterNodeViewDocument::Private::loadNode() {
	if (view) {
		view->setNodeId(nodeId);
	}

	updateTitle();
}

void SystemRouterNodeViewDocument::Private::updateTitle() {
	const auto node = view ? view->node() : nullptr;
	if (!node) {
		return;
	}

	const auto title = SystemRouterNodeViewDocument::tr("Просмотр: %1").arg(node->name());
	if (title == q->title()) {
		generatedTitle = title;
		return;
	}

	// Вкладку могли переименовать вручную: тогда заголовок не трогаем.
	if (!q->title().isEmpty() && q->title() != generatedTitle) {
		return;
	}

	q->setTitle(title);
	generatedTitle = title;
}

void SystemRouterNodeViewDocument::Private::reportWarning(const QString& message) {
	if (auto* controller = q->context() ? q->context()->commandController() : nullptr) {
		controller->printWarning(message);
	}

	emit q->errorOccurred(message);
}

SystemRouterNodeViewDocument::SystemRouterNodeViewDocument(SystemRouterService* service,
	SystemRouterCommandService* commandService, const QVariant& nodeId, QWidget* parent)
	: DocumentWidget(parent)
	, d(std::make_unique<Private>(this)) {
	d->service = service;
	d->nodeId = nodeId;

	setObjectName(QStringLiteral("SystemRouterNodeViewDocument"));
	setGlyph(QStringLiteral("🔍"));
	setTitle(tr("Просмотр узла"));
	d->generatedTitle = title();

	d->setupUi(commandService);
	d->loadNode();

	// Узел мог измениться в документе свойств: просмотр обновляется по
	// сигналу сервиса. Скрытая связь с this снимается вместе с документом.
	if (service) {
		connect(service, &SystemRouterService::itemChanged, this, [this](const QVariant& id) {
			if (!d->nodeId.isValid() || id != d->nodeId) {
				return;
			}

			d->loadNode();
		});
	}
}

SystemRouterNodeViewDocument::~SystemRouterNodeViewDocument() = default;

QVariant SystemRouterNodeViewDocument::nodeId() const {
	return d->nodeId;
}

LazyTreeNodePtr SystemRouterNodeViewDocument::node() const {
	return d->view ? d->view->node() : nullptr;
}

SystemRouterNodeViewWidget* SystemRouterNodeViewDocument::view() const {
	return d->view;
}

void SystemRouterNodeViewDocument::refresh() {
	d->loadNode();
}

bool SystemRouterNodeViewDocument::save() {
	// Документ только читает параметры: сохранять нечего.
	return true;
}

void SystemRouterNodeViewDocument::onContextAttached() {
	if (!d->service) {
		return;
	}

	if (hasVariable(kNodeIdVariable)) {
		const auto id = variable(kNodeIdVariable);
		if (id.isValid() && !id.isNull() && id != d->nodeId) {
			d->nodeId = id;
			d->loadNode();
		}
	}
	else if (d->nodeId.isValid() && !d->nodeId.isNull()) {
		setVariable(kNodeIdVariable, d->nodeId);
	}
}
