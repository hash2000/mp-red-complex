#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_widget.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_data_provider.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_node.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_highlight_delegate.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_node_role.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_view.h"

#include <QAction>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QStandardItem>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

class LazyTreeWidget::Private {
public:
	explicit Private(LazyTreeWidget* parent)
		: q(parent) {
	}

	LazyTreeWidget* q;
	ILazyNodesDataProvider* provider = nullptr;

	QLineEdit* searchLineEdit = nullptr;
	QTimer* searchTimer = nullptr;

	QToolButton* btnAdd = nullptr;
	QToolButton* btnDelete = nullptr;
	QToolButton* btnEdit = nullptr;
	QToolButton* btnRefresh = nullptr;
	QToolButton* btnBack = nullptr;

	QStackedWidget* stackedViews = nullptr;
	LazyTreeView* normalTreeView = nullptr;
	LazyTreeView* searchTreeView = nullptr;

	HighlightDelegate* normalDelegate = nullptr;
	HighlightDelegate* searchDelegate = nullptr;

	bool isSearchMode = false;
	bool confirmDelete = true;

	void setupUi();
	void setupConnections();
	void loadTreeData();
	void loadSearchResults(const QString& searchText);
	void updateActions();
	LazyTreeView* activeView() const;
};

LazyTreeWidget::LazyTreeWidget(ILazyNodesDataProvider* provider, QWidget* parent)
	: QWidget(parent)
	, d(std::make_unique<Private>(this)) {
	d->provider = provider;
	setObjectName(QStringLiteral("LazyTreeWidget"));

	d->normalDelegate = new HighlightDelegate(QString(), this);
	d->searchDelegate = new HighlightDelegate(QString(), this);

	d->setupUi();
	d->setupConnections();
	d->loadTreeData();
	d->updateActions();
}

LazyTreeWidget::~LazyTreeWidget() = default;

ILazyNodesDataProvider* LazyTreeWidget::provider() const {
	return d->provider;
}

LazyTreeView* LazyTreeWidget::treeView() const {
	return d->normalTreeView;
}

LazyTreeView* LazyTreeWidget::searchView() const {
	return d->searchTreeView;
}

QLineEdit* LazyTreeWidget::searchLineEdit() const {
	return d->searchLineEdit;
}

bool LazyTreeWidget::isSearchMode() const {
	return d->isSearchMode;
}

void LazyTreeWidget::setConfirmDelete(bool confirm) {
	d->confirmDelete = confirm;
}

bool LazyTreeWidget::confirmDelete() const {
	return d->confirmDelete;
}

void LazyTreeWidget::Private::setupUi() {
	auto* mainLayout = new QVBoxLayout(q);
	mainLayout->setContentsMargins(2, 2, 2, 2);
	mainLayout->setSpacing(2);

	searchLineEdit = new QLineEdit(q);
	searchLineEdit->setObjectName(QStringLiteral("LazyTreeSearchEdit"));
	searchLineEdit->setPlaceholderText(LazyTreeWidget::tr("Поиск узлов…"));
	searchLineEdit->setClearButtonEnabled(true);
	mainLayout->addWidget(searchLineEdit);

	auto* toolbarLayout = new QHBoxLayout();
	toolbarLayout->setSpacing(4);

	const auto createButton = [this, toolbarLayout](const QString& objectName, const QString& text,
		const QString& tooltip) {
		auto* button = new QToolButton(q);
		button->setObjectName(objectName);
		button->setText(text);
		button->setToolTip(tooltip);
		button->setToolButtonStyle(Qt::ToolButtonTextOnly);
		toolbarLayout->addWidget(button);
		return button;
	};

	btnAdd = createButton(QStringLiteral("LazyTreeAddButton"), QStringLiteral("+"),
		LazyTreeWidget::tr("Добавить"));
	btnDelete = createButton(QStringLiteral("LazyTreeDeleteButton"), QStringLiteral("⌫"),
		LazyTreeWidget::tr("Удалить"));
	btnEdit = createButton(QStringLiteral("LazyTreeEditButton"), QStringLiteral("✎"),
		LazyTreeWidget::tr("Переименовать"));
	btnRefresh = createButton(QStringLiteral("LazyTreeRefreshButton"), QStringLiteral("🗘"),
		LazyTreeWidget::tr("Обновить"));
	btnBack = createButton(QStringLiteral("LazyTreeBackButton"), QStringLiteral("🔙"),
		LazyTreeWidget::tr("Вернуться к обычному виду"));
	btnBack->hide();

	toolbarLayout->addStretch();
	mainLayout->addLayout(toolbarLayout);

	stackedViews = new QStackedWidget(q);
	normalTreeView = new LazyTreeView(provider, q);
	normalTreeView->setObjectName(QStringLiteral("LazyTreeView"));
	normalTreeView->setItemDelegate(normalDelegate);
	normalTreeView->setHeaderHidden(true);
	normalTreeView->setContextMenuPolicy(Qt::CustomContextMenu);

	searchTreeView = new LazyTreeView(provider, q);
	searchTreeView->setObjectName(QStringLiteral("LazyTreeSearchView"));
	searchTreeView->setItemDelegate(searchDelegate);
	searchTreeView->setHeaderHidden(true);

	stackedViews->addWidget(normalTreeView);
	stackedViews->addWidget(searchTreeView);
	mainLayout->addWidget(stackedViews);

	searchTimer = new QTimer(q);
	searchTimer->setSingleShot(true);
	searchTimer->setInterval(300);
}

void LazyTreeWidget::Private::setupConnections() {
	const auto connectView = [this](LazyTreeView* view) {
		QObject::connect(view, &LazyTreeView::nodeMoved, q, &LazyTreeWidget::nodeMoved);
		QObject::connect(view, &LazyTreeView::nodeRenamed, q, &LazyTreeWidget::nodeRenamed);
		QObject::connect(view, &LazyTreeView::nodeRemoved, q, &LazyTreeWidget::nodeRemoved);
		QObject::connect(view, &LazyTreeView::nodeAdded, q, &LazyTreeWidget::nodeAdded);
		QObject::connect(view, &LazyTreeView::emptyAreaClicked, q, &LazyTreeWidget::emptyAreaClicked);
		QObject::connect(view, &LazyTreeView::nodeActivated, q, &LazyTreeWidget::nodeActivated);
		QObject::connect(view, &LazyTreeView::currentNodeChanged, q, &LazyTreeWidget::selectionChanged);
		QObject::connect(view, &LazyTreeView::currentNodeChanged, q, &LazyTreeWidget::onCurrentNodeChanged);
		QObject::connect(view, &LazyTreeView::errorOccurred, q, &LazyTreeWidget::errorOccurred);
	};

	connectView(normalTreeView);
	connectView(searchTreeView);

	QObject::connect(searchLineEdit, &QLineEdit::textChanged, q, &LazyTreeWidget::onSearchTextChanged);
	QObject::connect(searchLineEdit, &QLineEdit::returnPressed, q, &LazyTreeWidget::onSearchPerform);
	QObject::connect(searchTimer, &QTimer::timeout, q, &LazyTreeWidget::onSearchPerform);
	QObject::connect(btnAdd, &QToolButton::clicked, q, &LazyTreeWidget::onAddNode);
	QObject::connect(btnDelete, &QToolButton::clicked, q, &LazyTreeWidget::onDeleteNode);
	QObject::connect(btnEdit, &QToolButton::clicked, q, &LazyTreeWidget::onEditNode);
	QObject::connect(btnRefresh, &QToolButton::clicked, q, &LazyTreeWidget::onRefreshNode);
	QObject::connect(btnBack, &QToolButton::clicked, q, &LazyTreeWidget::onBackToNormalView);
	QObject::connect(normalTreeView, &QWidget::customContextMenuRequested, q, &LazyTreeWidget::onContextMenu);
}

LazyTreeView* LazyTreeWidget::Private::activeView() const {
	return isSearchMode ? searchTreeView : normalTreeView;
}

void LazyTreeWidget::Private::updateActions() {
	LazyTreeView* view = activeView();
	const bool hasProvider = provider != nullptr;
	const bool hasNode = view && view->selectedNode() != nullptr;

	btnAdd->setEnabled(!isSearchMode && hasProvider);
	btnDelete->setEnabled(hasNode && hasProvider);
	btnEdit->setEnabled(hasNode && hasProvider);
	btnRefresh->setEnabled(hasProvider);
}

void LazyTreeWidget::onSearchTextChanged(const QString& text) {
	if (text.trimmed().isEmpty()) {
		d->searchTimer->stop();
		if (d->isSearchMode) {
			onBackToNormalView();
		}
		return;
	}

	d->searchTimer->start();
}

void LazyTreeWidget::onSearchPerform() {
	d->searchTimer->stop();

	if (!d->provider) {
		return;
	}

	const QString text = d->searchLineEdit->text().trimmed();
	if (text.isEmpty()) {
		onBackToNormalView();
		return;
	}

	d->isSearchMode = true;
	d->loadSearchResults(text);
	d->searchDelegate->setHighlightText(text);
	d->normalDelegate->setHighlightText(text);
	d->btnBack->show();
	d->stackedViews->setCurrentWidget(d->searchTreeView);
	d->updateActions();
}

void LazyTreeWidget::Private::loadSearchResults(const QString& searchText) {
	const auto results = provider->search(searchText);
	searchTreeView->setNodes(results);

	const auto error = provider->lastError();
	if (!error.isEmpty()) {
		emit q->errorOccurred(error);
	}
}

void LazyTreeWidget::onAddNode() {
	if (!d->provider) {
		return;
	}

	if (d->isSearchMode) {
		onBackToNormalView();
	}

	auto* item = d->normalTreeView->itemFromIndex(d->normalTreeView->currentIndex());
	if (d->normalTreeView->isDummyItem(item) || d->normalTreeView->isTemporaryItem(item)) {
		item = nullptr;
	}

	const auto newNode = d->provider->createTreeNode();
	if (!newNode) {
		return;
	}

	d->normalTreeView->addNodeAndStartEdit(item, newNode);
}

void LazyTreeWidget::onDeleteNode() {
	LazyTreeView* view = d->activeView();
	if (!view) {
		return;
	}

	const auto node = view->selectedNode();
	if (!node) {
		return;
	}

	if (d->confirmDelete) {
		const auto answer = QMessageBox::question(this, tr("Удаление"),
			tr("Удалить узел «%1»?").arg(node->name()));
		if (answer != QMessageBox::Yes) {
			return;
		}
	}

	view->removeSelectedNode();

	if (d->isSearchMode) {
		onSearchPerform();
	}

	d->updateActions();
}

void LazyTreeWidget::onEditNode() {
	LazyTreeView* view = d->activeView();
	if (view) {
		view->startEdit(view->currentIndex());
	}
}

void LazyTreeWidget::onBackToNormalView() {
	QVariant selected;
	if (d->isSearchMode) {
		selected = d->searchTreeView->selectedNodeId();
	}

	d->isSearchMode = false;
	d->searchTimer->stop();
	d->btnBack->hide();
	d->stackedViews->setCurrentWidget(d->normalTreeView);
	d->searchDelegate->setHighlightText(QString());
	d->normalDelegate->setHighlightText(QString());

	if (selected.isValid()) {
		d->normalTreeView->expandAndSelectNode(selected);
	}

	d->updateActions();
}

void LazyTreeWidget::onRefreshNode() {
	if (!d->provider) {
		return;
	}

	if (d->isSearchMode) {
		onSearchPerform();
		return;
	}

	const QVariant nodeId = d->normalTreeView->selectedNodeId();
	if (!nodeId.isValid()) {
		refreshAll();
		return;
	}

	const auto children = d->provider->get(nodeId);
	d->normalTreeView->setChildren(nodeId, children);

	const auto error = d->provider->lastError();
	if (!error.isEmpty()) {
		emit errorOccurred(error);
	}
}

void LazyTreeWidget::onContextMenu(const QPoint& pos) {
	auto* item = d->normalTreeView->itemFromIndex(d->normalTreeView->indexAt(pos));
	LazyTreeNodePtr node;
	if (item && !d->normalTreeView->isDummyItem(item)
		&& !d->normalTreeView->isTemporaryItem(item)) {
		d->normalTreeView->setCurrentIndex(item->index());
		node = LazyTreeNodeRole::nodeByItem(item);
	}

	QMenu menu(this);

	QAction* addAction = menu.addAction(tr("+ Добавить"), this, &LazyTreeWidget::onAddNode);
	QAction* deleteAction = menu.addAction(tr("⌫ Удалить"), this, &LazyTreeWidget::onDeleteNode);
	QAction* editAction = menu.addAction(tr("✎ Переименовать"), this, &LazyTreeWidget::onEditNode);
	menu.addSeparator();
	menu.addAction(tr("🗘 Обновить"), this, &LazyTreeWidget::onRefreshNode);

	const bool hasNode = node != nullptr;
	const bool hasProvider = d->provider != nullptr;
	deleteAction->setEnabled(hasNode && hasProvider);
	editAction->setEnabled(hasNode && hasProvider);
	addAction->setEnabled(hasProvider);

	extendContextMenu(menu, node);
	menu.exec(d->normalTreeView->viewport()->mapToGlobal(pos));
}

void LazyTreeWidget::extendContextMenu(QMenu& menu, const LazyTreeNodePtr& node) {
	Q_UNUSED(menu);
	Q_UNUSED(node);
}

void LazyTreeWidget::onCurrentNodeChanged(const LazyTreeNodePtr& node) {
	Q_UNUSED(node);
	d->updateActions();
}

void LazyTreeWidget::refreshAll() {
	if (!d->provider) {
		return;
	}

	if (d->isSearchMode) {
		d->searchTreeView->setNodes(LazyTreeNodeList());
		onBackToNormalView();
	}

	d->loadTreeData();
	d->updateActions();
}

void LazyTreeWidget::Private::loadTreeData() {
	if (!provider) {
		return;
	}

	const auto roots = provider->get(QVariant());
	normalTreeView->setNodes(roots);

	const auto error = provider->lastError();
	if (!error.isEmpty()) {
		emit q->errorOccurred(error);
	}
}

void LazyTreeWidget::clearSearch() {
	QSignalBlocker blocker(d->searchLineEdit);
	d->searchLineEdit->clear();
	d->searchTimer->stop();
	onBackToNormalView();
}

bool LazyTreeWidget::updateNode(const QVariant& nodeId, const LazyTreeNodePtr& nodeData) {
	if (!d->provider || !nodeData) {
		return false;
	}

	if (!d->provider->update(nodeData)) {
		const auto error = d->provider->lastError();
		if (!error.isEmpty()) {
			emit errorOccurred(error);
		}
		return false;
	}

	QStandardItem* item = d->normalTreeView->findNode(nodeId);
	if (!item) {
		item = d->searchTreeView->findNode(nodeId);
	}

	if (item) {
		item->setText(nodeData->name());
		LazyTreeNodeRole::updateNodeRaw(item, nodeData);
	}

	return true;
}
