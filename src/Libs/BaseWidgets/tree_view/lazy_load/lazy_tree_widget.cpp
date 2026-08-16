#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_widget.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_highlight_delegate.h"

#include <QVBoxLayout>
#include <QStandardItemModel>
#include <QLineEdit>
#include <QTimer>
#include <QToolButton>
#include <QStackedWidget>
#include <QTreeView>
#include <QMenu>
#include <QMessageBox>

namespace {
constexpr int TreeNodeRawData = Qt::UserRole + 1;
constexpr int TreeNodeId = Qt::UserRole + 2;
constexpr int TreeNodeIsTemporary = Qt::UserRole + 3;
}

class LazyTreeViewWidget::Private {
public:
	Private(LazyTreeViewWidget* parent) : q(parent) {}
	LazyTreeViewWidget* q;


	ILazyNodesDataProvider* provider;

	QLineEdit* searchLineEdit;
	QTimer* searchTimer;

	QToolButton* btnAdd;
	QToolButton* btnDelete;
	QToolButton* btnBack;
	QToolButton* btnShowSearch;

	QStackedWidget* stackedViews;
	QTreeView* normalTreeView;
	QTreeView* searchTreeView;

	QStandardItemModel* normalModel;
	QStandardItemModel* searchModel;

	HighlightDelegate* normalDelegate;
	HighlightDelegate* searchDelegate;

	bool isSearchMode = false;
	bool isAddingNode = false;

	void setupUI();
	void setupToolbar();
	void loadTreeData();
	void loadSearchResults(const QString& searchText);
	void buildSubTree(QTreeView* treeView, QStandardItem* parentItem, const LazyTreeNodeList& nodes, bool isTemporary);
	void addDummyNode(QStandardItem* parentItem);
	void removeDummyNode(QStandardItem* parentItem);
	bool hasDummyNode(QStandardItem* item) const;
	void expandAndSelectInNormalTree(const LazyTreeNodePtr &node);
	void preareRootItemModel(QStandardItemModel* model);
	QStandardItem* findNode(QStandardItem* parent, const QVariant& id) const;
};

void LazyTreeViewWidget::Private::preareRootItemModel(QStandardItemModel* model) {
	auto newNodeNormalRootData = provider->createTreeNode();
	auto normalRootItem = model->invisibleRootItem();
	normalRootItem->setData(QVariant::fromValue(newNodeNormalRootData), TreeNodeRawData);
}

LazyTreeViewWidget::LazyTreeViewWidget(ILazyNodesDataProvider* provider, QWidget* parent)
	: d(std::make_unique<Private>(this)) {
	d->provider = provider;
	d->normalModel = new QStandardItemModel(this);
	d->searchModel = new QStandardItemModel(this);

	d->normalDelegate = new HighlightDelegate("", this);
	d->searchDelegate = new HighlightDelegate("", this);

	d->setupUI();
	d->setupToolbar();

	// Настройка обычного дерева
	d->normalTreeView->setModel(d->normalModel);
	d->normalTreeView->setItemDelegate(d->normalDelegate);
	d->normalTreeView->setHeaderHidden(true);
	d->normalTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(d->normalTreeView, &QWidget::customContextMenuRequested, this, &LazyTreeViewWidget::showContextMenu);
	connect(d->normalTreeView, &QTreeView::expanded, this, &LazyTreeViewWidget::onTreeViewExpanded);
	connect(d->normalTreeView, &QTreeView::activated, this, &LazyTreeViewWidget::onNodeActivated);
	connect(d->normalTreeView->selectionModel(), &QItemSelectionModel::currentChanged, this, &LazyTreeViewWidget::onSelectionChanged);

	// Настройка дерева поиска
	d->searchTreeView->setModel(d->searchModel);
	d->searchTreeView->setItemDelegate(d->searchDelegate);
	d->searchTreeView->setHeaderHidden(true);
	connect(d->searchTreeView, &QTreeView::expanded, this, &LazyTreeViewWidget::onTreeViewExpanded);
	connect(d->searchTreeView, &QTreeView::activated, this, &LazyTreeViewWidget::onNodeActivated);
	connect(d->searchTreeView->selectionModel(), &QItemSelectionModel::currentChanged, this, &LazyTreeViewWidget::onSelectionChanged);

	connect(d->normalModel, &QStandardItemModel::itemChanged, this, &LazyTreeViewWidget::onItemChanged);

	// Таймер для debounce поиска (300 мс)
	d->searchTimer = new QTimer(this);
	d->searchTimer->setSingleShot(true);
	connect(d->searchTimer, &QTimer::timeout, this, &LazyTreeViewWidget::performSearch);
	connect(d->searchLineEdit, &QLineEdit::textChanged, this, &LazyTreeViewWidget::onSearchTextChanged);

	d->loadTreeData();
}

LazyTreeViewWidget::~LazyTreeViewWidget() = default;

void LazyTreeViewWidget::Private::setupUI() {
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
	normalTreeView = new QTreeView(q);
	searchTreeView = new QTreeView(q);

	stackedViews->addWidget(normalTreeView);
	stackedViews->addWidget(searchTreeView);

	mainLayout->addWidget(stackedViews);
}

void LazyTreeViewWidget::Private::setupToolbar() {
	// UTF-8 иконки и тултипы
	btnAdd->setText("➕");
	btnAdd->setToolTip("Добавить узел");
	btnAdd->setToolButtonStyle(Qt::ToolButtonIconOnly);
	connect(btnAdd, &QToolButton::clicked, q, &LazyTreeViewWidget::onAddNode);

	btnDelete->setText("➖");
	btnDelete->setToolTip("Удалить узел");
	btnDelete->setToolButtonStyle(Qt::ToolButtonIconOnly);
	connect(btnDelete, &QToolButton::clicked, q, &LazyTreeViewWidget::onDeleteNode);

	btnShowSearch->setText("🔍");
	btnShowSearch->setToolTip("Результат поиска");
	btnShowSearch->setToolButtonStyle(Qt::ToolButtonIconOnly);
	connect(btnShowSearch, &QToolButton::clicked, q, &LazyTreeViewWidget::onShowSearch);

	btnBack->setText("🔙");
	btnBack->setToolTip("Вернуться к обычному виду");
	btnBack->setToolButtonStyle(Qt::ToolButtonIconOnly);
	btnBack->hide(); // Скрыта по умолчанию
	connect(btnBack, &QToolButton::clicked, q, &LazyTreeViewWidget::onBackToNormalView);
}

void LazyTreeViewWidget::onSearchTextChanged(const QString& text) {
	d->searchTimer->start(300); // Debounce 300ms
}

void LazyTreeViewWidget::performSearch() {
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

void LazyTreeViewWidget::onAddNode() {
	auto currentIndex = d->normalTreeView->currentIndex();
	QStandardItem* parentItem = nullptr;

	if (currentIndex.isValid()) {
		parentItem = d->normalModel->itemFromIndex(currentIndex);
	}
	else {
		parentItem = d->normalModel->invisibleRootItem();
	}

	// Создаем временный узел
	auto newNode = d->provider->createTreeNode();
	auto parentNodeData = parentItem->data(TreeNodeRawData).value<LazyTreeNodePtr>();
	if (!parentNodeData) {
		return;
	}

	newNode->setParentId(parentNodeData->id());
	newNode->setName(""); // Пустое имя для редактирования

	auto newItem = new QStandardItem("");
	newItem->setData(true, TreeNodeIsTemporary);
	newItem->setData(QVariant::fromValue(newNode), TreeNodeRawData);
	newItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);

	parentItem->appendRow(newItem);

	QModelIndex newIndex = d->normalModel->indexFromItem(newItem);
	d->normalTreeView->setCurrentIndex(newIndex);
	d->isAddingNode = true;
	d->normalTreeView->edit(newIndex);
}

void LazyTreeViewWidget::onDeleteNode() {
	QModelIndex currentIndex = d->normalTreeView->currentIndex();
	if (!currentIndex.isValid()) {
		return;
	}

	QStandardItem* item = d->normalModel->itemFromIndex(currentIndex);
	auto nodeData = item->data(TreeNodeRawData).value<LazyTreeNodePtr>();
	if (!nodeData) {
		return;
	}

	if (QMessageBox::question(this, "Удаление", "Удалить узел?") == QMessageBox::Yes) {
		if (d->provider->deleteTreeNode(nodeData->id())) {
			QStandardItem* parent = item->parent();
			if (parent) {
				parent->removeRow(item->row());
			}
			else {
				d->normalModel->removeRow(item->row());
			}
		}
	}
}

void LazyTreeViewWidget::onItemChanged(QStandardItem* item) {
	auto isTemporary = item->data(TreeNodeIsTemporary).toBool();
	if (!isTemporary) {
		return;
	}

	if (!d->isAddingNode) {
		return;
	}

	auto nodeData = item->data(TreeNodeRawData).value<LazyTreeNodePtr>();
	auto newName = item->text().trimmed();

	auto removeUneditable = [&]() {
		auto parent = item->parent();
		if (parent) {
			parent->removeRow(item->row());
		}
		else {
			d->normalModel->removeRow(item->row());
		}
	};

	auto makeItemPermanent = [&]() {
		// Убираем флаг редактирования
		Qt::ItemFlags flags = item->flags();
		flags &= ~Qt::ItemIsEditable;
		item->setFlags(flags);
		item->setData(false, TreeNodeIsTemporary);
	};

	if (newName.isEmpty()) {
		// Отмена создания - удаляем узел
		removeUneditable();
	}
	else {
		// Сохраняем через провайдер
		nodeData->setName(newName);
		bool success = d->provider->addTreeNode(nodeData);
		if (!success) {
			// Ошибка сохранения - удаляем узел
			removeUneditable();
		}
		else {
			makeItemPermanent();
		}
	}

	d->isAddingNode = false;
}

void LazyTreeViewWidget::onSelectionChanged() {
	QTreeView* treeView = qobject_cast<QTreeView*>(sender()->parent());
	if (!treeView) {
		return;
	}

	QModelIndex currentIndex = treeView->currentIndex();
	if (!currentIndex.isValid()) {
		return;
	}

	QStandardItemModel* model = qobject_cast<QStandardItemModel*>(treeView->model());
	if (!model) {
		return;
	}

	QStandardItem* item = model->itemFromIndex(currentIndex);
	if (!item) {
		return;
	}

	QVariant nodeId = item->data(TreeNodeRawData);
	emit nodeSelectionChanged(nodeId);
}

void LazyTreeViewWidget::onShowSearch() {
	d->isSearchMode = true;
	d->btnBack->show();
	d->btnShowSearch->hide();
	d->btnAdd->hide();
	d->btnDelete->hide();
	d->stackedViews->setCurrentWidget(d->searchTreeView);
}

void LazyTreeViewWidget::onBackToNormalView() {
	// Запоминаем выбранный ID в поиске, если он есть
	LazyTreeNodePtr selected;
	if (d->isSearchMode) {
		QModelIndex searchIndex = d->searchTreeView->currentIndex();
		if (searchIndex.isValid()) {
			selected = d->searchModel->data(searchIndex, TreeNodeRawData).value<LazyTreeNodePtr>();
		}
	}

	d->isSearchMode = false;
	d->btnBack->hide();
	d->btnShowSearch->show();
	d->btnAdd->show();
	d->btnDelete->show();
	d->stackedViews->setCurrentWidget(d->normalTreeView);

	if (selected) {
		d->expandAndSelectInNormalTree(selected);
	}
}

void LazyTreeViewWidget::showContextMenu(const QPoint& pos) {
	QMenu menu(this);
	menu.addAction("➕ Добавить узел", this, &LazyTreeViewWidget::onAddNode);
	menu.addAction("➖ Удалить узел", this, &LazyTreeViewWidget::onDeleteNode);
	menu.exec(d->normalTreeView->viewport()->mapToGlobal(pos));
}

bool LazyTreeViewWidget::updateNode(const QVariant& nodeId, const LazyTreeNodePtr& nodeData) {
	auto rootItem = d->normalModel->invisibleRootItem();
	auto item = d->findNode(rootItem, nodeId);
	if (!item) {
		return false;
	}

	auto oldNodeData = item->data(TreeNodeRawData).value<LazyTreeNodePtr>();
	if (!nodeData) {
		return false;
	}

	if (!d->provider->updateTreeNode(nodeData)) {
		return false;
	}

	item->setData(QVariant::fromValue(nodeData), TreeNodeRawData);
	item->setText(nodeData->name());

	return true;
}

void LazyTreeViewWidget::refreshAll() {
	d->loadTreeData();
}

void LazyTreeViewWidget::onNodeActivated(const QModelIndex& index) {
	if (!index.isValid()) {
		return;
	}

	QStandardItem* item = nullptr;
	if (sender() == d->normalTreeView) {
		item = d->normalModel->itemFromIndex(index);
	}
	else if (sender() == d->searchTreeView) {
		item = d->searchModel->itemFromIndex(index);
	}

	if (!item) {
		return;
	}

	// Пропускаем временные узлы (они редактируются через встроенный редактор)
	if (item->data(TreeNodeIsTemporary).toBool()) {
		return;
	}

	auto nodeData = item->data(TreeNodeRawData).value<LazyTreeNodePtr>();
	if (!nodeData) {
		return;
	}

	// Эмитим сигнал о запросе редактирования
	emit nodeEditRequested(nodeData);
}

void LazyTreeViewWidget::onTreeViewExpanded(const QModelIndex& index) {
	if (!index.isValid()) {
		return;
	}

	QStandardItem* item = nullptr;
	QTreeView* treeView = nullptr;
	if (sender() == d->normalTreeView) {
		item = d->normalModel->itemFromIndex(index);
		treeView = d->normalTreeView;
	}
	else if (sender() == d->searchTreeView) {
		item = d->searchModel->itemFromIndex(index);
		treeView = d->searchTreeView;
	}

	if (!item) {
		return;
	}

	// Проверяем, есть ли dummy-узел
	if (!d->hasDummyNode(item)) {
		return;
	}

	auto nodeData = item->data(TreeNodeRawData).value<LazyTreeNodePtr>();
	if (!nodeData) {
		return;
	}

	d->removeDummyNode(item);

	// Загружаем реальных детей
	auto nodes = d->provider->treeNodes(nodeData->id());
	if (!nodes.empty()) {
		d->buildSubTree(treeView, item, nodes, false);
	}
}

void LazyTreeViewWidget::Private::loadTreeData() {
	searchModel->clear();
	normalModel->clear();
	auto roots = provider->treeNodes(QVariant());
	auto rootItem = normalModel->invisibleRootItem();
	preareRootItemModel(searchModel);
	preareRootItemModel(normalModel);
	buildSubTree(normalTreeView, rootItem, roots, false);
}

void LazyTreeViewWidget::Private::loadSearchResults(const QString& searchText) {
	searchModel->clear();
	auto results = provider->searchTreeNodes(searchText);
	auto rootNode = searchModel->invisibleRootItem();
	preareRootItemModel(searchModel);
	buildSubTree(searchTreeView, rootNode, results, false);
}

void LazyTreeViewWidget::Private::buildSubTree(QTreeView* treeView, QStandardItem* parentItem, const LazyTreeNodeList& nodes, bool isTemporary) {
	for (const auto& nodeData : nodes) {
		auto item = new QStandardItem(nodeData->name());
		const auto& children = nodeData->children();

		Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
		if (isTemporary) {
			flags |= Qt::ItemIsEditable;
		}

		item->setData(QVariant::fromValue(nodeData), TreeNodeRawData);
		item->setData(nodeData->id(), TreeNodeId);
		item->setData(isTemporary, TreeNodeIsTemporary);
		item->setFlags(flags);

		parentItem->appendRow(item);

		if (children.has_value()) {
			if (!children->empty()) {
				// Дети уже загружены провайдером
				buildSubTree(treeView, item, *children, false);

				auto model = qobject_cast<QStandardItemModel*>(treeView->model());
				auto index = model->indexFromItem(item);
				if (!index.isValid()) {
					continue;
				}

				treeView->setExpanded(index, nodeData->expanded());
			}
			else {
				// Дети есть (hasChildren=true), но не загружены
				// Добавляем dummy-узел
				addDummyNode(item);
			}
		}
	}
}

void LazyTreeViewWidget::Private::addDummyNode(QStandardItem* parentItem) {
	auto* dummyItem = new QStandardItem("");
	dummyItem->setFlags(Qt::NoItemFlags); // Неактивный, не выделяется
	parentItem->appendRow(dummyItem);
}

void LazyTreeViewWidget::Private::removeDummyNode(QStandardItem* parentItem) {
	if (hasDummyNode(parentItem)) {
		parentItem->removeRow(0);
	}
}

bool LazyTreeViewWidget::Private::hasDummyNode(QStandardItem* item) const {
	if (item->rowCount() == 1) {
		auto child = item->child(0);
		auto isNullData = child->data(TreeNodeRawData).isNull();
		return child && isNullData;
	}

	return false;
}

void LazyTreeViewWidget::Private::expandAndSelectInNormalTree(const LazyTreeNodePtr& node) {
	auto rootNode = normalModel->invisibleRootItem();
	auto found = findNode(rootNode, node->id());
	if (!found) {
		return;
	}

	auto index = normalModel->indexFromItem(found);
	normalTreeView->setCurrentIndex(index);
	normalTreeView->scrollTo(index, QAbstractItemView::PositionAtCenter);
	normalTreeView->setExpanded(index, true);
	normalTreeView->setFocus();
}

QStandardItem* LazyTreeViewWidget::Private::findNode(QStandardItem* parent, const QVariant& id) const {
	for (int i = 0; i < parent->rowCount(); i++) {
		auto child = parent->child(i);
		auto childNodeData = child->data(TreeNodeRawData).value<LazyTreeNodePtr>();
		if (!childNodeData) {
			return nullptr;
		}

		if (childNodeData->id() == id) {
			return child;
		}

		// Рекурсивный поиск
		QStandardItem* found = findNode(child, id);
		if (found) {
			return found;
		}
	}

	return nullptr;
}
