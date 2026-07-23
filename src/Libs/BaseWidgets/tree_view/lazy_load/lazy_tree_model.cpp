#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_model.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_item.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_data_provider.h"


class LazyTreeModel::Private {
public:
	Private(LazyTreeModel* parent) : q(parent) {}
	LazyTreeModel* q;

	ILazyNodesDataProvider* provider;
	std::unique_ptr<LazyTreeItem> rootItem;

	void setupModelData(const LazyTreeNodeList& nodes, LazyTreeItem* parent);
};

LazyTreeModel::LazyTreeModel(ILazyNodesDataProvider* provider, QObject* parent)
	: d(std::make_unique<Private>(this)) {
	d->provider = provider;
	auto rootData = d->provider->createTreeNode();
	rootData->setId(-1);
	rootData->setName("Root");
	d->rootItem = std::make_unique<LazyTreeItem>(rootData);

	// Загружаем корневые узлы
	auto roots = d->provider->treeNodes(LazyTreeNodePtr());
	d->setupModelData(roots, d->rootItem.get());
}

LazyTreeModel::~LazyTreeModel() = default;

void LazyTreeModel::Private::setupModelData(const LazyTreeNodeList& nodes, LazyTreeItem* parent) {
	for (const auto& node : nodes) {
		auto newItem = new LazyTreeItem(node, parent);
		parent->appendChild(newItem);

		// Если данные уже пришли с непустыми детьми (редкий случай для ленивой, но возможный)
		auto childrenOpt = node->children();
		if (childrenOpt.has_value() && !childrenOpt->empty()) {
			newItem->setChildrenFetched(true);

			// TODO: допилить рекурсивную загрузку
		}
	}
}

LazyTreeItem* LazyTreeModel::getItem(const QModelIndex& index) const {
	if (index.isValid()) {
		auto item = static_cast<LazyTreeItem*>(index.internalPointer());
		if (item) {
			return item;
		}
	}

	return d->rootItem.get();
}


QModelIndex LazyTreeModel::index(int row, int column, const QModelIndex& parent) const {
	if (!hasIndex(row, column, parent)) {
		return { };
	}

	LazyTreeItem* parentItem = getItem(parent);
	LazyTreeItem* childItem = parentItem->child(row);
	if (childItem) {
		return createIndex(row, column, childItem);
	}

	return { };
}

QModelIndex LazyTreeModel::parent(const QModelIndex& index) const {
	if (!index.isValid()) {
		return { };
	}

	LazyTreeItem* childItem = getItem(index);
	LazyTreeItem* parentItem = childItem->parent();

	if (parentItem == d->rootItem.get()) {
		return { };
	}

	return createIndex(parentItem->row(), 0, parentItem);
}

int LazyTreeModel::rowCount(const QModelIndex& parent) const {
	if (parent.column() > 0) {
		return 0;
	}

	return getItem(parent)->childCount();
}

int LazyTreeModel::columnCount(const QModelIndex& parent) const {
	Q_UNUSED(parent);
	return 1; // Только колонка с именем
}

bool LazyTreeModel::canFetchMore(const QModelIndex& parent) const {
	LazyTreeItem* item = getItem(parent);
	auto childrenOpt = item->data()->children();

	// Если children = nullopt, это лист (потомков нет и не будет)
	if (!childrenOpt.has_value()) {
		return false;
	}

	// Если уже загружены, не грузим снова
	if (item->isChildrenFetched()) {
		return false;
	}

	// Если пустой список, значит, потомки ожидаются (нужна загрузка)
	return true;
}

void LazyTreeModel::fetchMore(const QModelIndex& parent) {
	LazyTreeItem* item = getItem(parent);
	if (item->isChildrenFetched()) {
		return;
	}

	beginInsertRows(parent, 0, 0); // Пока не знаем точное кол-во, но для простоты начнем так
	// Правильнее: получить данные, узнать размер, затем beginInsertRows

	auto nodes = d->provider->treeNodes(item->data());
	int count = std::distance(nodes.begin(), nodes.end());
	if (count > 0) {
		beginInsertRows(parent, 0, count - 1);
		d->setupModelData(nodes, item);
		endInsertRows();
	}

	item->setChildrenFetched(true);
}

QVariant LazyTreeModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid()) {
		return { };
	}

	LazyTreeItem* item = getItem(index);
	if (role == Qt::DisplayRole || role == Qt::EditRole) {
		return item->data()->name();
	}

	if (role == Qt::UserRole) {
		return item->data()->id();
	}

	return { };
}

Qt::ItemFlags LazyTreeModel::flags(const QModelIndex& index) const {
	if (!index.isValid()) {
		return Qt::NoItemFlags;
	}

	Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	// Разрешаем редактирование, если это временный узел или по логике приложения
	flags |= Qt::ItemIsEditable;
	return flags;
}

bool LazyTreeModel::setData(const QModelIndex& index, const QVariant& value, int role) {
	if (role != Qt::EditRole || !index.isValid()) {
		return false;
	}

	LazyTreeItem* item = getItem(index);
	QString newName = value.toString().trimmed();

	// ЛОГИКА VS CODE: если имя пустое и узел временный -> удаляем его
	if (item->isTemporary() && newName.isEmpty()) {
		removeRow(index.row(), index.parent());
		return true; // Успешно "отменили"
	}

	item->data()->setName(newName);

	if (item->isTemporary()) {
		// Сохраняем через провайдер
		bool success = d->provider->addTreeNode(item->data());
		if (success) {
			item->setIsTemporary(false);
		}
		else {
			removeRow(index.row(), index.parent());
			return false;
		}
	}
	else {
		d->provider->updateTreeNode(item->data());
	}

	emit dataChanged(index, index, { role });
	return true;
}

QModelIndex LazyTreeModel::findIndexById(int id, const QModelIndex& parent) const {
	int rows = rowCount(parent);
	for (int r = 0; r < rows; ++r) {
		QModelIndex idx = index(r, 0, parent);
		LazyTreeItem* item = getItem(idx);
		if (item->data()->id() == id) {
			return idx;
		}

		if (hasChildren(idx)) { // Рекурсивный поиск
			// Примечание: если узел не развернут, мы не найдем детей.
			// В реальном приложении здесь может потребоваться рекурсивный fetchMore, 
			// но для простоты ищем только в загруженных.
			QModelIndex found = findIndexById(id, idx);
			if (found.isValid()) {
				return found;
			}
		}
	}

	return { };
}

QModelIndex LazyTreeModel::appendTemporaryItem(const QModelIndex& currentIndex) {
	LazyTreeItem* parentItem = getItem(currentIndex.isValid() ? currentIndex : QModelIndex());
	// Создаем временный узел
	auto newNode = d->provider->createTreeNode();
	newNode->setParentId(parentItem->data()->id() == -1 ? std::nullopt : std::optional<int>(parentItem->data()->id()));
	newNode->setName(""); // Пустое имя для редактирования

	// Вставляем в модель
	int row = parentItem->childCount();
	beginInsertRows(currentIndex, row, row);
	auto newItem = new LazyTreeItem(newNode, parentItem);
	newItem->setIsTemporary(true);
	parentItem->appendChild(newItem);
	endInsertRows();

	return index(row, 0, currentIndex);
}
