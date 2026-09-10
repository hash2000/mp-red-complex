#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_widget.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_highlight_delegate.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_view.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_data_provider.h"

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

void LazyTreeWidget::setActionHandler(ILazyTreeNodeActionHandler* actionHandler) {
	d->normalTreeView->setActionHandler(actionHandler);
	d->searchTreeView->setActionHandler(actionHandler);
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

void LazyTreeWidget::showContextMenu(const QPoint& pos) {
	QMenu menu(this);
	menu.addAction("➕ Добавить узел", this, &LazyTreeWidget::onAddNode);
	menu.addAction("➖ Удалить узел", this, &LazyTreeWidget::onDeleteNode);
	menu.exec(d->normalTreeView->viewport()->mapToGlobal(pos));
}

void LazyTreeWidget::refreshAll() {
	d->loadTreeData();
}

void LazyTreeWidget::Private::loadSearchResults(const QString& searchText) {
	auto results = provider->searchTreeNodes(searchText);
	searchTreeView->setNodes(results);
}

void LazyTreeWidget::Private::loadTreeData() {
	auto roots = provider->treeNodes(QVariant());
	normalTreeView->setNodes(roots);
}
