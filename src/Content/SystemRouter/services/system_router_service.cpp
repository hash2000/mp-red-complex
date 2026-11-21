#include "Content/SystemRouter/services/system_router_service.h"

#include <QSet>

#include <functional>

class SystemRouterService::Private {
public:
	explicit Private(SystemRouterService* parent)
		: q(parent) {
	}

	SystemRouterService* q;
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

SystemRouterService::SystemRouterService(ILazyNodesDataProvider* provider, QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->provider = provider;
}

SystemRouterService::~SystemRouterService() = default;

ILazyNodesDataProvider* SystemRouterService::dataProvider() const {
	return d->provider;
}

LazyTreeNodeList SystemRouterService::rootNodes() const {
	return d->provider ? d->provider->get(QVariant()) : LazyTreeNodeList();
}

LazyTreeNodeList SystemRouterService::allNodes() const {
	d->clearError();

	if (!d->provider) {
		d->lastError = tr("Поставщик данных системного маршрутизатора не задан");
		return {};
	}

	LazyTreeNodeList result;
	QSet<QString> visited;

	std::function<void(const QVariant&)> visit = [&](const QVariant& parentId) {
		const auto nodes = d->provider->get(parentId);
		for (const auto& node : nodes) {
			if (!node) {
				continue;
			}

			const auto key = node->id().toString();
			if (visited.contains(key)) {
				continue;
			}

			visited.insert(key);
			result.push_back(node);

			// Для листа children() == nullopt, для узла с детьми — пустой
			// список или уже загруженный набор. В обоих случаях обходим
			// потомков через поставщика, чтобы получить плоский список.
			if (node->children().has_value()) {
				visit(node->id());
			}
		}
	};

	visit(QVariant());

	const auto providerError = d->provider->lastError();
	if (!providerError.isEmpty()) {
		d->lastError = providerError;
		return {};
	}

	return result;
}

LazyTreeNodeList SystemRouterService::children(const QVariant& parentId) const {
	return d->provider ? d->provider->get(parentId) : LazyTreeNodeList();
}

LazyTreeNodeList SystemRouterService::search(const QString& text) const {
	return d->provider ? d->provider->search(text) : LazyTreeNodeList();
}

LazyTreeNodePtr SystemRouterService::item(const QVariant& id) const {
	return d->provider ? d->provider->getNode(id) : nullptr;
}

LazyTreeNodePtr SystemRouterService::createItem(const QVariant& parentId, const QString& name,
	const QVariantMap& properties) {
	d->clearError();

	if (!d->provider) {
		d->fail(tr("Поставщик данных системного маршрутизатора не задан"));
		return nullptr;
	}

	const QString trimmed = name.trimmed();
	if (trimmed.isEmpty()) {
		d->fail(tr("Имя узла не может быть пустым"));
		return nullptr;
	}

	if (d->siblingNameExists(parentId, trimmed)) {
		d->fail(tr("Узел с именем «%1» уже существует").arg(trimmed));
		return nullptr;
	}

	const auto node = d->provider->createTreeNode();
	if (!node) {
		d->fail(tr("Не удалось создать узел системного маршрутизатора"));
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

bool SystemRouterService::updateItem(const LazyTreeNodePtr& node) {
	d->clearError();

	if (!d->provider || !node) {
		return d->fail(tr("Поставщик данных системного маршрутизатора не задан"));
	}

	if (!d->provider->update(node)) {
		return d->fail(d->provider->lastError());
	}

	emit itemChanged(node->id());
	return true;
}

bool SystemRouterService::renameItem(const QVariant& id, const QString& name) {
	d->clearError();

	if (!d->provider) {
		return d->fail(tr("Поставщик данных системного маршрутизатора не задан"));
	}

	const QString trimmed = name.trimmed();
	if (trimmed.isEmpty()) {
		return d->fail(tr("Имя узла не может быть пустым"));
	}

	const auto node = d->provider->getNode(id);
	if (!node) {
		return d->fail(tr("Узел не найден"));
	}

	if (d->siblingNameExists(node->parentId(), trimmed, id)) {
		return d->fail(tr("Узел с именем «%1» уже существует").arg(trimmed));
	}

	node->setName(trimmed);
	if (!d->provider->update(node)) {
		return d->fail(d->provider->lastError());
	}

	emit itemChanged(id);
	return true;
}

bool SystemRouterService::moveItem(const QVariant& id, const QVariant& parentId) {
	d->clearError();

	if (!d->provider) {
		return d->fail(tr("Поставщик данных системного маршрутизатора не задан"));
	}

	if (id == parentId) {
		return d->fail(tr("Узел не может быть родителем самому себе"));
	}

	if (d->wouldCreateCycle(id, parentId)) {
		return d->fail(tr("Нельзя переместить узел внутрь его поддерева"));
	}

	const auto node = d->provider->getNode(id);
	if (!node) {
		return d->fail(tr("Узел не найден"));
	}

	node->setParentId(parentId);
	if (!d->provider->update(node)) {
		return d->fail(d->provider->lastError());
	}

	emit itemChanged(id);
	return true;
}

bool SystemRouterService::removeItem(const QVariant& id) {
	d->clearError();

	if (!d->provider) {
		return d->fail(tr("Поставщик данных системного маршрутизатора не задан"));
	}

	if (!d->provider->remove(id)) {
		return d->fail(d->provider->lastError());
	}

	emit itemChanged(id);
	return true;
}

bool SystemRouterService::setAlwaysExpanded(const QVariant& id, bool alwaysExpanded) {
	d->clearError();

	if (!d->provider) {
		return d->fail(tr("Поставщик данных системного маршрутизатора не задан"));
	}

	if (!id.isValid() || id.isNull()) {
		return d->fail(tr("Узел не найден"));
	}

	if (!d->provider->setExpanded(id, alwaysExpanded)) {
		const auto providerError = d->provider->lastError();
		return d->fail(providerError.isEmpty()
			? tr("Не удалось сохранить признак «узел всегда открыт»")
			: providerError);
	}

	emit itemChanged(id);
	return true;
}

QString SystemRouterService::lastError() const {
	return d->lastError;
}
