#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_widget.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_highlight_delegate.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_view.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_data_provider.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_node.h"

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
	QToolButton* btnEdit;
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
	connect(d->normalTreeView, &QWidget::customContextMenuRequested, this, &LazyTreeWidget::onContextMenu);
	connect(d->normalTreeView, &LazyTreeView::nodeMoved, this, &LazyTreeWidget::nodeMoved);
	connect(d->normalTreeView, &LazyTreeView::nodeRenamed, this, &LazyTreeWidget::nodeRenamed);
	connect(d->normalTreeView, &LazyTreeView::nodeRemoved, this, &LazyTreeWidget::nodeRemoved);
	connect(d->normalTreeView, &LazyTreeView::nodeAdded, this, &LazyTreeWidget::nodeAdded);
	connect(d->normalTreeView, &LazyTreeView::emptyAreaClicked, this, &LazyTreeWidget::emptyAreaClicked);
	connect(d->normalTreeView, &LazyTreeView::nodeActivated, this, &LazyTreeWidget::nodeActivated);
	connect(d->normalTreeView, &LazyTreeView::selectionChanged, this, &LazyTreeWidget::selectionChanged);

	// Настройка дерева поиска
	d->searchTreeView->setItemDelegate(d->searchDelegate);
	d->searchTreeView->setHeaderHidden(true);
	d->searchTreeView->viewport()->installEventFilter(this);

	// Таймер для debounce поиска (300 мс)
	d->searchTimer = new QTimer(this);
	d->searchTimer->setSingleShot(true);

	connect(d->searchTimer, &QTimer::timeout, this, &LazyTreeWidget::onSearchPerform);
	connect(d->searchLineEdit, &QLineEdit::textChanged, this, &LazyTreeWidget::onSearchTextChanged);
	connect(d->searchTreeView, &LazyTreeView::nodeMoved, this, &LazyTreeWidget::nodeMoved);
	connect(d->searchTreeView, &LazyTreeView::nodeRenamed, this, &LazyTreeWidget::nodeRenamed);
	connect(d->searchTreeView, &LazyTreeView::nodeAdded, this, &LazyTreeWidget::nodeAdded);
	connect(d->searchTreeView, &LazyTreeView::emptyAreaClicked, this, &LazyTreeWidget::emptyAreaClicked);
	connect(d->searchTreeView, &LazyTreeView::nodeActivated, this, &LazyTreeWidget::nodeActivated);
	connect(d->searchTreeView, &LazyTreeView::selectionChanged, this, &LazyTreeWidget::selectionChanged);

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
	btnEdit = new QToolButton(q);
	btnBack = new QToolButton(q);
	btnShowSearch = new QToolButton(q);

	toolbarLayout->addWidget(btnShowSearch);
	toolbarLayout->addWidget(btnBack);
	toolbarLayout->addWidget(btnAdd);
	toolbarLayout->addWidget(btnDelete);
	toolbarLayout->addWidget(btnEdit);
	toolbarLayout->addStretch();

	mainLayout->addLayout(toolbarLayout);

	stackedViews = new QStackedWidget(q);
	normalTreeView = new LazyTreeView(provider, q);
	searchTreeView = new LazyTreeView(provider, q);

	stackedViews->addWidget(normalTreeView);
	stackedViews->addWidget(searchTreeView);

	mainLayout->addWidget(stackedViews);
}

void LazyTreeWidget::Private::setupToolbar() {
	// UTF-8 иконки и тултипы
	btnAdd->setText("+");
	btnAdd->setToolTip("Добавить");
	btnAdd->setToolButtonStyle(Qt::ToolButtonIconOnly);
	connect(btnAdd, &QToolButton::clicked, q, &LazyTreeWidget::onAddNode);

	btnDelete->setText("⌫");
	btnDelete->setToolTip("Удалить");
	btnDelete->setToolButtonStyle(Qt::ToolButtonIconOnly);
	connect(btnDelete, &QToolButton::clicked, q, &LazyTreeWidget::onDeleteNode);

	btnEdit->setText("✎");
	btnEdit->setToolTip("Редактировать");
	btnEdit->setToolButtonStyle(Qt::ToolButtonIconOnly);
	connect(btnEdit, &QToolButton::clicked, q, &LazyTreeWidget::onEditNode);

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

void LazyTreeWidget::onSearchPerform() {
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
	auto index = d->normalTreeView->currentIndex();
	auto item = d->normalTreeView->itemFromIndex(index);
	auto newNode = d->provider->createTreeNode();
	d->normalTreeView->addNodeAndStartEdit(item, newNode);
}

void LazyTreeWidget::onDeleteNode() {
	if (d->isSearchMode) {
		return;
	}

	if (QMessageBox::question(this, "Удаление", "Удалить узел?") == QMessageBox::Yes) {
		d->normalTreeView->removeSelectedNode();
	}
}

void LazyTreeWidget::onEditNode() {
	auto tree = d->isSearchMode ? d->searchTreeView : d->normalTreeView;
	auto index = tree->currentIndex();
	if (!index.isValid()) {
		return;
	}

	tree->startEdit(index);
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
	// Запоминаем выбранный ID в поиске, если он есть
	QVariant selected;
	if (d->isSearchMode) {
		selected = d->searchTreeView->selectedNodeId();
	}

	d->isSearchMode = false;
	d->btnBack->hide();
	d->btnShowSearch->show();
	d->btnAdd->show();
	d->btnDelete->show();
	d->stackedViews->setCurrentWidget(d->normalTreeView);

	if (selected.isValid()) {
		d->normalTreeView->expandAndSelectNode(selected);
	}
}

void LazyTreeWidget::onContextMenu(const QPoint& pos) {
	QMenu menu(this);
	menu.addAction("+ Добавить", this, &LazyTreeWidget::onAddNode);
	menu.addAction("⌫ Удалить", this, &LazyTreeWidget::onDeleteNode);
	menu.addAction("✎ Редактировать", this, &LazyTreeWidget::onEditNode);
	menu.addAction("🗘 Обновить", this, &LazyTreeWidget::onRefreshNode);
	menu.exec(d->normalTreeView->viewport()->mapToGlobal(pos));
}

void LazyTreeWidget::refreshAll() {
	d->loadTreeData();
}

void LazyTreeWidget::onRefreshNode() {
	auto tree = d->isSearchMode ? d->searchTreeView : d->normalTreeView;
	auto nodeId = tree->selectedNodeId();
	auto items = d->provider->treeNodes(nodeId);
	tree->setChildren(nodeId, items);
}

void LazyTreeWidget::Private::loadSearchResults(const QString& searchText) {
	auto results = provider->searchTreeNodes(searchText);
	searchTreeView->setNodes(results);
}

void LazyTreeWidget::Private::loadTreeData() {
	auto roots = provider->treeNodes(QVariant());
	normalTreeView->setNodes(roots);
}
