#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_widget.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_highlight_delegate.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_view.h"

#include <QVBoxLayout>
#include <QStandardItemModel>
#include <QLineEdit>
#include <QTimer>
#include <QToolButton>
#include <QStackedWidget>
#include <QTreeView>
#include <QMenu>
#include <QMessageBox>

class LazyTreeWidget::Private {
public:
	Private(LazyTreeWidget* parent) : q(parent) {}
	LazyTreeWidget* q;


	ILazyNodesDataProvider* provider;

	QLineEdit* searchLineEdit;
	QTimer* searchTimer;

	QToolButton* btnAdd;
	QToolButton* btnDelete;
	QToolButton* btnBack;
	QToolButton* btnShowSearch;

	QStackedWidget* stackedViews;
	LazyTreeView* normalTreeView;
	LazyTreeView* searchTreeView;

	HighlightDelegate* normalDelegate;
	HighlightDelegate* searchDelegate;

	bool isSearchMode = false;
	bool isAddingNode = false;

	void setupUI();
	void setupToolbar();
	void loadTreeData();
	void loadSearchResults(const QString& searchText);
	void buildSubTree(QTreeView* treeView, QStandardItem* parentItem, const LazyTreeNodeList& nodes, bool isTemporary);
	void expandAndSelectInNormalTree(const LazyTreeNodePtr &node);
};

LazyTreeWidget::LazyTreeWidget(ILazyNodesDataProvider* provider, QWidget* parent)
	: d(std::make_unique<Private>(this)) {
	d->provider = provider;

	d->normalDelegate = new HighlightDelegate("", this);
	d->searchDelegate = new HighlightDelegate("", this);

	d->setupUI();
	d->setupToolbar();

	// Настройка обычного дерева
	d->normalTreeView->setItemDelegate(d->normalDelegate);
	d->normalTreeView->setHeaderHidden(true);
	d->normalTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
	d->normalTreeView->viewport()->installEventFilter(this);
	connect(d->normalTreeView, &QWidget::customContextMenuRequested, this, &LazyTreeWidget::showContextMenu);
	connect(d->normalTreeView, &LazyTreeView::requestFetchChildren, this, [this](const QVariant& parentId) {
		auto nodes = d->provider->treeNodes(parentId);
		d->normalTreeView->appendChildren(parentId, nodes);
	});
	connect(d->normalTreeView, &LazyTreeView::nodeActivated, this, [this](const LazyTreeNodePtr& node, const QString& name) {
		emit nodeEditRequested(node);
	});
	connect(d->normalTreeView, &LazyTreeView::selectionChanged, this, [this](const LazyTreeNodePtr& node) {
		emit nodeSelectionChanged(node);
	});

	// Настройка дерева поиска
	d->searchTreeView->setItemDelegate(d->searchDelegate);
	d->searchTreeView->setHeaderHidden(true);
	d->searchTreeView->viewport()->installEventFilter(this);
	connect(d->searchTreeView, &LazyTreeView::requestFetchChildren, this, [this](const QVariant& parentId) {
		auto nodes = d->provider->treeNodes(parentId);
		d->searchTreeView->appendChildren(parentId, nodes);
	});

	// Таймер для debounce поиска (300 мс)
	d->searchTimer = new QTimer(this);
	d->searchTimer->setSingleShot(true);
	connect(d->searchTimer, &QTimer::timeout, this, &LazyTreeWidget::performSearch);
	connect(d->searchLineEdit, &QLineEdit::textChanged, this, &LazyTreeWidget::onSearchTextChanged);

	d->loadTreeData();
}

LazyTreeWidget::~LazyTreeWidget() = default;

void LazyTreeWidget::Private::setupUI() {
	auto mainLayout = new QVBoxLayout(q);
	mainLayout->setContentsMargins(2, 2, 2, 2);
	mainLayout->setSpacing(2);

	searchLineEdit = new QLineEdit(q);
	searchLineEdit->setPlaceholderText("🔍 Поиск узлов...");
	mainLayout->addWidget(searchLineEdit);

	auto toolbarLayout = new QHBoxLayout();
	toolbarLayout->setSpacing(5);

	btnAdd = new QToolButton(q);
	btnDelete = new QToolButton(q);
	btnBack = new QToolButton(q);
	btnShowSearch = new QToolButton(q);

	toolbarLayout->addWidget(btnShowSearch);
	toolbarLayout->addWidget(btnBack);
	toolbarLayout->addWidget(btnAdd);
	toolbarLayout->addWidget(btnDelete);
	toolbarLayout->addStretch();

	mainLayout->addLayout(toolbarLayout);

	stackedViews = new QStackedWidget(q);
	normalTreeView = new LazyTreeView(q);
	searchTreeView = new LazyTreeView(q);

	stackedViews->addWidget(normalTreeView);
	stackedViews->addWidget(searchTreeView);

	mainLayout->addWidget(stackedViews);
}

void LazyTreeWidget::Private::setupToolbar() {
	// UTF-8 иконки и тултипы
	btnAdd->setText("➕");
	btnAdd->setToolTip("Добавить узел");
	btnAdd->setToolButtonStyle(Qt::ToolButtonIconOnly);
	connect(btnAdd, &QToolButton::clicked, q, &LazyTreeWidget::onAddNode);

	btnDelete->setText("➖");
	btnDelete->setToolTip("Удалить узел");
	btnDelete->setToolButtonStyle(Qt::ToolButtonIconOnly);
	connect(btnDelete, &QToolButton::clicked, q, &LazyTreeWidget::onDeleteNode);

	btnShowSearch->setText("🔍");
	btnShowSearch->setToolTip("Результат поиска");
	btnShowSearch->setToolButtonStyle(Qt::ToolButtonIconOnly);
	connect(btnShowSearch, &QToolButton::clicked, q, &LazyTreeWidget::onShowSearch);

	btnBack->setText("🔙");
	btnBack->setToolTip("Вернуться к обычному виду");
	btnBack->setToolButtonStyle(Qt::ToolButtonIconOnly);
	btnBack->hide(); // Скрыта по умолчанию
	connect(btnBack, &QToolButton::clicked, q, &LazyTreeWidget::onBackToNormalView);
}

void LazyTreeWidget::onSearchTextChanged(const QString& text) {
	d->searchTimer->start(300); // Debounce 300ms
}

void LazyTreeWidget::performSearch() {
	QString text = d->searchLineEdit->text().trimmed();

	if (text.isEmpty()) {
		d->searchDelegate->setHighlightText("");
		d->normalDelegate->setHighlightText("");
		onBackToNormalView();
		return;
	}

	d->isSearchMode = true;
	d->btnBack->show();
	d->btnShowSearch->hide();
	d->btnAdd->hide();
	d->btnDelete->hide();
	d->stackedViews->setCurrentWidget(d->searchTreeView);

	d->loadSearchResults(text);
	d->searchDelegate->setHighlightText(text);
	d->normalDelegate->setHighlightText(text);
}

void LazyTreeWidget::onAddNode() {
	//auto currentIndex = d->normalTreeView->currentIndex();
	//QStandardItem* parentItem = nullptr;

	//if (currentIndex.isValid()) {
	//	parentItem = d->normalModel->itemFromIndex(currentIndex);
	//}
	//else {
	//	parentItem = d->normalModel->invisibleRootItem();
	//}

	//// Создаем временный узел
	//auto newNode = d->provider->createTreeNode();
	//auto parentNodeData = parentItem->data(TreeNodeRawData).value<LazyTreeNodePtr>();
	//if (!parentNodeData) {
	//	return;
	//}

	//newNode->setParentId(parentNodeData->id());
	//newNode->setName(""); // Пустое имя для редактирования

	//auto newItem = new QStandardItem("");
	//newItem->setData(true, TreeNodeIsTemporary);
	//newItem->setData(QVariant::fromValue(newNode), TreeNodeRawData);
	//newItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);

	//parentItem->appendRow(newItem);

	//QModelIndex newIndex = d->normalModel->indexFromItem(newItem);
	//d->normalTreeView->setCurrentIndex(newIndex);
	//d->isAddingNode = true;
	//d->normalTreeView->edit(newIndex);
}

void LazyTreeWidget::onDeleteNode() {
	//QModelIndex currentIndex = d->normalTreeView->currentIndex();
	//if (!currentIndex.isValid()) {
	//	return;
	//}

	//QStandardItem* item = d->normalModel->itemFromIndex(currentIndex);
	//auto nodeData = item->data(TreeNodeRawData).value<LazyTreeNodePtr>();
	//if (!nodeData) {
	//	return;
	//}

	//if (QMessageBox::question(this, "Удаление", "Удалить узел?") == QMessageBox::Yes) {
	//	if (d->provider->deleteTreeNode(nodeData->id())) {
	//		QStandardItem* parent = item->parent();
	//		if (parent) {
	//			parent->removeRow(item->row());
	//		}
	//		else {
	//			d->normalModel->removeRow(item->row());
	//		}
	//	}
	//}
}

void LazyTreeWidget::onItemChanged(QStandardItem* item) {
	//auto isTemporary = item->data(TreeNodeIsTemporary).toBool();
	//if (!isTemporary) {
	//	return;
	//}

	//if (!d->isAddingNode) {
	//	return;
	//}

	//auto nodeData = item->data(TreeNodeRawData).value<LazyTreeNodePtr>();
	//auto newName = item->text().trimmed();

	//auto removeUneditable = [&]() {
	//	auto parent = item->parent();
	//	if (parent) {
	//		parent->removeRow(item->row());
	//	}
	//	else {
	//		d->normalModel->removeRow(item->row());
	//	}
	//};

	//auto makeItemPermanent = [&]() {
	//	// Убираем флаг редактирования
	//	Qt::ItemFlags flags = item->flags();
	//	flags &= ~Qt::ItemIsEditable;
	//	item->setFlags(flags);
	//	item->setData(false, TreeNodeIsTemporary);
	//};

	//if (newName.isEmpty()) {
	//	// Отмена создания - удаляем узел
	//	removeUneditable();
	//}
	//else {
	//	// Сохраняем через провайдер
	//	nodeData->setName(newName);
	//	bool success = d->provider->addTreeNode(nodeData);
	//	if (!success) {
	//		// Ошибка сохранения - удаляем узел
	//		removeUneditable();
	//	}
	//	else {
	//		makeItemPermanent();
	//	}
	//}

	//d->isAddingNode = false;
}

void LazyTreeWidget::onShowSearch() {
	d->isSearchMode = true;
	d->btnBack->show();
	d->btnShowSearch->hide();
	d->btnAdd->hide();
	d->btnDelete->hide();
	d->stackedViews->setCurrentWidget(d->searchTreeView);
}

void LazyTreeWidget::onBackToNormalView() {
	//// Запоминаем выбранный ID в поиске, если он есть
	//LazyTreeNodePtr selected;
	//if (d->isSearchMode) {
	//	QModelIndex searchIndex = d->searchTreeView->currentIndex();
	//	if (searchIndex.isValid()) {
	//		selected = d->searchModel->data(searchIndex, TreeNodeRawData).value<LazyTreeNodePtr>();
	//	}
	//}

	//d->isSearchMode = false;
	//d->btnBack->hide();
	//d->btnShowSearch->show();
	//d->btnAdd->show();
	//d->btnDelete->show();
	//d->stackedViews->setCurrentWidget(d->normalTreeView);

	//if (selected) {
	//	d->expandAndSelectInNormalTree(selected);
	//}
}

void LazyTreeWidget::showContextMenu(const QPoint& pos) {
	QMenu menu(this);
	menu.addAction("➕ Добавить узел", this, &LazyTreeWidget::onAddNode);
	menu.addAction("➖ Удалить узел", this, &LazyTreeWidget::onDeleteNode);
	menu.exec(d->normalTreeView->viewport()->mapToGlobal(pos));
}

bool LazyTreeWidget::updateNode(const QVariant& nodeId, const LazyTreeNodePtr& nodeData) {
	//auto rootItem = d->normalModel->invisibleRootItem();
	//auto item = d->findNode(rootItem, nodeId);
	//if (!item) {
	//	return false;
	//}

	//auto oldNodeData = item->data(TreeNodeRawData).value<LazyTreeNodePtr>();
	//if (!nodeData) {
	//	return false;
	//}

	//if (!d->provider->updateTreeNode(nodeData)) {
	//	return false;
	//}

	//item->setData(QVariant::fromValue(nodeData), TreeNodeRawData);
	//item->setText(nodeData->name());

	return true;
}

void LazyTreeWidget::refreshAll() {
	d->loadTreeData();
}

void LazyTreeWidget::Private::loadTreeData() {
	auto roots = provider->treeNodes(QVariant());
	normalTreeView->setNodes(roots);
}

void LazyTreeWidget::Private::loadSearchResults(const QString& searchText) {
	//searchModel->clear();
	//auto results = provider->searchTreeNodes(searchText);
	//auto rootNode = searchModel->invisibleRootItem();
	//preareRootItemModel(searchModel);
	//buildSubTree(searchTreeView, rootNode, results, false);
}

void LazyTreeWidget::Private::buildSubTree(QTreeView* treeView, QStandardItem* parentItem, const LazyTreeNodeList& nodes, bool isTemporary) {
	//for (const auto& nodeData : nodes) {
	//	auto item = new QStandardItem(nodeData->name());
	//	const auto& children = nodeData->children();

	//	Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	//	if (isTemporary) {
	//		flags |= Qt::ItemIsEditable;
	//	}

	//	item->setData(QVariant::fromValue(nodeData), TreeNodeRawData);
	//	item->setData(nodeData->id(), TreeNodeId);
	//	item->setData(isTemporary, TreeNodeIsTemporary);
	//	item->setFlags(flags);

	//	parentItem->appendRow(item);

	//	if (children.has_value()) {
	//		if (!children->empty()) {
	//			// Дети уже загружены провайдером
	//			buildSubTree(treeView, item, *children, false);

	//			auto model = qobject_cast<QStandardItemModel*>(treeView->model());
	//			auto index = model->indexFromItem(item);
	//			if (!index.isValid()) {
	//				continue;
	//			}

	//			treeView->setExpanded(index, nodeData->expanded());
	//		}
	//		else {
	//			// Дети есть (hasChildren=true), но не загружены
	//			// Добавляем dummy-узел
	//			addDummyNode(item);
	//		}
	//	}
	//}
}

void LazyTreeWidget::Private::expandAndSelectInNormalTree(const LazyTreeNodePtr& node) {
	//auto rootNode = normalModel->invisibleRootItem();
	//auto found = findNode(rootNode, node->id());
	//if (!found) {
	//	return;
	//}

	//auto index = normalModel->indexFromItem(found);
	//normalTreeView->setCurrentIndex(index);
	//normalTreeView->scrollTo(index, QAbstractItemView::PositionAtCenter);
	//normalTreeView->setExpanded(index, true);
	//normalTreeView->setFocus();
}

