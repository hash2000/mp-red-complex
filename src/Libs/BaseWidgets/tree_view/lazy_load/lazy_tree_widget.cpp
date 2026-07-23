#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_widget.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_item.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_model.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_highlight_delegate.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_data_provider.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QTimer>
#include <QToolButton>
#include <QStackedWidget>
#include <QTreeView>
#include <QMenu>
#include <QMessageBox>

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

	QStackedWidget* stackedViews;
	QTreeView* normalTreeView;
	QTreeView* searchTreeView;

	LazyTreeModel* normalModel;
	LazyTreeModel* searchModel;

	HighlightDelegate* normalDelegate;
	HighlightDelegate* searchDelegate;

	bool isSearchMode = false;

	void setupUI();
	void setupToolbar();
	void expandAndSelectInNormalTree(int id);
};

LazyTreeViewWidget::LazyTreeViewWidget(ILazyNodesDataProvider* provider, QWidget* parent)
	: d(std::make_unique<Private>(this)) {
	d->provider = provider;
	d->normalModel = new LazyTreeModel(d->provider, this);
	d->searchModel = new LazyTreeModel(d->provider, this); // Изначально пустой или с корнем

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

	// Настройка дерева поиска
	d->searchTreeView->setModel(d->searchModel);
	d->searchTreeView->setItemDelegate(d->searchDelegate);
	d->searchTreeView->setHeaderHidden(true);

	// Таймер для debounce поиска (300 мс)
	d->searchTimer = new QTimer(this);
	d->searchTimer->setSingleShot(true);
	connect(d->searchTimer, &QTimer::timeout, this, &LazyTreeViewWidget::performSearch);
	connect(d->searchLineEdit, &QLineEdit::textChanged, this, &LazyTreeViewWidget::onSearchTextChanged);

}

LazyTreeViewWidget::~LazyTreeViewWidget() = default;

void LazyTreeViewWidget::Private::setupUI() {
	auto* mainLayout = new QVBoxLayout(q);
	mainLayout->setContentsMargins(5, 5, 5, 5);
	mainLayout->setSpacing(5);

	searchLineEdit = new QLineEdit(q);
	searchLineEdit->setPlaceholderText("🔍 Поиск узлов...");
	mainLayout->addWidget(searchLineEdit);

	auto* toolbarLayout = new QHBoxLayout();
	toolbarLayout->setSpacing(5);

	btnAdd = new QToolButton(q);
	btnDelete = new QToolButton(q);
	btnBack = new QToolButton(q);

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

	btnBack->setText("⬅️");
	btnBack->setToolTip("Вернуться к обычному виду");
	btnBack->setToolButtonStyle(Qt::ToolButtonIconOnly);
	btnBack->hide(); // Скрыта по умолчанию
	connect(btnBack, &QToolButton::clicked, q, &LazyTreeViewWidget::onBackToNormalView);
}

void LazyTreeViewWidget::Private::expandAndSelectInNormalTree(int id) {
	// Эта функция должна рекурсивно найти узел, развернуть его родителей и выбрать его.
	// Для упрощения, если модель ленивая, поиск может потребовать предварительной загрузки.
	// В полноценной реализации здесь нужен обход дерева с вызовом fetchMore при необходимости.
	QModelIndex found = normalModel->findIndexById(id);
	if (found.isValid()) {
		normalTreeView->setCurrentIndex(found);
		normalTreeView->scrollTo(found, QAbstractItemView::PositionAtCenter);
		normalTreeView->setFocus();
	}
}

void LazyTreeViewWidget::onSearchTextChanged(const QString& text) {
	d->searchTimer->start(300); // Debounce 300ms
}

void LazyTreeViewWidget::performSearch() {
	QString text = d->searchLineEdit->text().trimmed();
	d->searchDelegate->setHighlightText(text);

	if (text.isEmpty()) {
		onBackToNormalView();
		return;
	}

	d->isSearchMode = true;
	d->btnBack->show();
	d->btnAdd->hide();
	d->btnDelete->hide();
	d->stackedViews->setCurrentWidget(d->searchTreeView);

	// Пересоздаем модель поиска для чистоты
	delete d->searchModel;
	d->searchModel = new LazyTreeModel(d->provider, this);
	d->searchTreeView->setModel(d->searchModel);
	d->searchTreeView->setItemDelegate(d->searchDelegate);

	auto results = d->provider->searchTreeNodes(text);
	// Хак: мы временно подменяем корневые данные модели результатами поиска
	// В идеале, LazyTreeModel должен иметь метод setRootNodes
	// Для простоты реализации здесь: мы можем добавить публичный метод в LazyTreeModel:
	// m_searchModel->replaceRootData(results); 
	// (Реализация replaceRootData аналогична конструктору, но с очисткой m_rootItem->childItems)
}

// ДОБАВИТЬ В LazyTreeModel.h: void replaceRootData(const std::list<std::shared_ptr<ITreeNodeModel>>& nodes);
// ДОБАВИТЬ В LazyTreeModel.cpp:
/*
void LazyTreeModel::replaceRootData(const std::list<std::shared_ptr<ITreeNodeModel>>& nodes) {
		beginResetModel();
		qDeleteAll(m_rootItem->childItems);
		m_rootItem->childItems.clear();
		setupModelData(nodes, m_rootItem);
		endResetModel();
}
*/

void LazyTreeViewWidget::onAddNode() {
	QModelIndex currentIndex = d->normalTreeView->currentIndex();
	QModelIndex newIndex = d->normalModel->appendTemporaryItem(currentIndex);

	// Разворачиваем родителя, если нужно, и начинаем редактирование
	if (!currentIndex.isValid()) {
		d->normalTreeView->setExpanded(newIndex, true);
	}
	d->normalTreeView->setCurrentIndex(newIndex);
	d->normalTreeView->edit(newIndex);
}

void LazyTreeViewWidget::onDeleteNode() {
	QModelIndex currentIndex = d->normalTreeView->currentIndex();
	if (!currentIndex.isValid()) {
		return;
	}

	LazyTreeItem* item = d->normalModel->getItem(currentIndex);
	if (QMessageBox::question(this, "Удаление", "Удалить узел?") == QMessageBox::Yes) {
		if (d->provider->deleteTreeNode(item->data())) {
			// Удаление из модели
			QModelIndex parent = currentIndex.parent();
			d->normalModel->removeRow(currentIndex.row(), parent);
		}
	}
}

void LazyTreeViewWidget::onBackToNormalView() {
	// Запоминаем выбранный ID в поиске, если он есть
	int selectedId = -1;
	if (d->isSearchMode) {
		QModelIndex searchIndex = d->searchTreeView->currentIndex();
		if (searchIndex.isValid()) {
			selectedId = d->searchModel->data(searchIndex, Qt::UserRole).toInt();
		}
	}

	d->isSearchMode = false;
	d->searchLineEdit->clear();
	d->btnBack->hide();
	d->btnAdd->show();
	d->btnDelete->show();
	d->stackedViews->setCurrentWidget(d->normalTreeView);

	if (selectedId != -1) {
		d->expandAndSelectInNormalTree(selectedId);
	}
}

void LazyTreeViewWidget::showContextMenu(const QPoint& pos) {
	QMenu menu(this);
	menu.addAction("➕ Добавить узел", this, &LazyTreeViewWidget::onAddNode);
	menu.addAction("➖ Удалить узел", this, &LazyTreeViewWidget::onDeleteNode);
	menu.exec(d->normalTreeView->viewport()->mapToGlobal(pos));
}
