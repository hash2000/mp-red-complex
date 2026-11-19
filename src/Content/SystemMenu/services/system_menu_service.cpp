#include "Content/SystemMenu/services/system_menu_service.h"

#include <QSet>

class SystemMenuService::Private {
public:
	explicit Private(SystemMenuService* parent)
		: q(parent) {
	}

	SystemMenuService* q;
	ILazyNodesDataProvider* provider = nullptr;
	QString lastError;

	void clearError() {
		lastError.clear();
	}

	bool fail(const QString& message) {
		lastError = message;
		emit q->errorOccurred(message);
		return false;
	}

	bool siblingNameExists(const QVariant& parentId, const QString& name,
		const QVariant& excludeId = {}) const {
		if (!provider) {
			return false;
		}

		const auto siblings = provider->get(parentId);
		for (const auto& sibling : siblings) {
			if (!sibling || sibling->id() == excludeId) {
				continue;
			}

			if (sibling->name().trimmed().compare(name, Qt::CaseInsensitive) == 0) {
				return true;
			}
		}

		return false;
	}

	bool wouldCreateCycle(const QVariant& id, const QVariant& parentId) const {
		if (!provider) {
			return false;
		}

		QSet<QString> visited;
		QVariant current = parentId;
		while (current.isValid() && !current.isNull()) {
			const QString key = current.toString();
			if (current == id || visited.contains(key)) {
				return true;
			}

			visited.insert(key);
			const auto parent = provider->getNode(current);
			if (!parent) {
				break;
			}

			current = parent->parentId();
		}

		return false;
	}
};

SystemMenuService::SystemMenuService(ILazyNodesDataProvider* provider, QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->provider = provider;
}

SystemMenuService::~SystemMenuService() = default;

ILazyNodesDataProvider* SystemMenuService::dataProvider() const {
	return d->provider;
}

LazyTreeNodeList SystemMenuService::rootNodes() const {
	return d->provider ? d->provider->get(QVariant()) : LazyTreeNodeList();
}

LazyTreeNodeList SystemMenuService::children(const QVariant& parentId) const {
	return d->provider ? d->provider->get(parentId) : LazyTreeNodeList();
}

LazyTreeNodeList SystemMenuService::search(const QString& text) const {
	return d->provider ? d->provider->search(text) : LazyTreeNodeList();
}

LazyTreeNodePtr SystemMenuService::item(const QVariant& id) const {
	return d->provider ? d->provider->getNode(id) : nullptr;
}

LazyTreeNodePtr SystemMenuService::createItem(const QVariant& parentId, const QString& name,
	const QVariantMap& properties) {
	d->clearError();

	if (!d->provider) {
		d->fail(tr("Поставщик данных системного меню не задан"));
		return nullptr;
	}

	const QString trimmed = name.trimmed();
	if (trimmed.isEmpty()) {
		d->fail(tr("Имя пункта меню не может быть пустым"));
		return nullptr;
	}

	if (d->siblingNameExists(parentId, trimmed)) {
		d->fail(tr("Пункт меню с именем «%1» уже существует").arg(trimmed));
		return nullptr;
	}

	const auto node = d->provider->createTreeNode();
	if (!node) {
		d->fail(tr("Не удалось создать узел системного меню"));
		return nullptr;
	}

	node->setParentId(parentId);
	node->setName(trimmed);
	for (auto it = properties.constBegin(); it != properties.constEnd(); it++) {
		node->setProperty(it.key(), it.value());
	}

	if (!d->provider->add(node)) {
		d->fail(d->provider->lastError());
		return nullptr;
	}

	emit itemChanged(node->id());
	return node;
}

bool SystemMenuService::updateItem(const LazyTreeNodePtr& node) {
	d->clearError();

	if (!d->provider || !node) {
		return d->fail(tr("Поставщик данных системного меню не задан"));
	}

	if (!d->provider->update(node)) {
		return d->fail(d->provider->lastError());
	}

	emit itemChanged(node->id());
	return true;
}

bool SystemMenuService::renameItem(const QVariant& id, const QString& name) {
	d->clearError();

	if (!d->provider) {
		return d->fail(tr("Поставщик данных системного меню не задан"));
	}

	const QString trimmed = name.trimmed();
	if (trimmed.isEmpty()) {
		return d->fail(tr("Имя пункта меню не может быть пустым"));
	}

	const auto node = d->provider->getNode(id);
	if (!node) {
		return d->fail(tr("Пункт меню не найден"));
	}

	if (d->siblingNameExists(node->parentId(), trimmed, id)) {
		return d->fail(tr("Пункт меню с именем «%1» уже существует").arg(trimmed));
	}

	node->setName(trimmed);
	if (!d->provider->update(node)) {
		return d->fail(d->provider->lastError());
	}

	emit itemChanged(id);
	return true;
}

bool SystemMenuService::moveItem(const QVariant& id, const QVariant& parentId) {
	d->clearError();

	if (!d->provider) {
		return d->fail(tr("Поставщик данных системного меню не задан"));
	}

	if (id == parentId) {
		return d->fail(tr("Пункт меню не может быть родителем самому себе"));
	}

	if (d->wouldCreateCycle(id, parentId)) {
		return d->fail(tr("Нельзя переместить пункт меню внутрь его поддерева"));
	}

	const auto node = d->provider->getNode(id);
	if (!node) {
		return d->fail(tr("Пункт меню не найден"));
	}

	node->setParentId(parentId);
	if (!d->provider->update(node)) {
		return d->fail(d->provider->lastError());
	}

	emit itemChanged(id);
	return true;
}

bool SystemMenuService::removeItem(const QVariant& id) {
	d->clearError();

	if (!d->provider) {
		return d->fail(tr("Поставщик данных системного меню не задан"));
	}

	if (!d->provider->remove(id)) {
		return d->fail(d->provider->lastError());
	}

	emit itemChanged(id);
	return true;
}

QString SystemMenuService::lastError() const {
	return d->lastError;
}
