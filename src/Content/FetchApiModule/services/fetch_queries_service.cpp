#include "Content/FetchApiModule/services/fetch_queries_service.h"

class FetchQueriesService::Private {
public:
	explicit Private(FetchQueriesService* parent)
		: q(parent) {
	}

	FetchQueriesService* q;
	ILazyNodesDataProvider* provider = nullptr;

	void setError(const QString& error) {
		lastError = error;
	}

	QString lastError;
};

FetchQueriesService::FetchQueriesService(ILazyNodesDataProvider* provider, QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->provider = provider;
}

FetchQueriesService::~FetchQueriesService() = default;

ILazyNodesDataProvider* FetchQueriesService::dataProvider() const {
	return d->provider;
}

LazyTreeNodeList FetchQueriesService::queries(const QVariant& parentId) const {
	if (!d->provider) {
		return {};
	}

	return d->provider->get(parentId);
}

LazyTreeNodeList FetchQueriesService::search(const QString& text) const {
	if (!d->provider) {
		return {};
	}

	return d->provider->search(text);
}

LazyTreeNodePtr FetchQueriesService::query(const QVariant& id) const {
	if (!d->provider) {
		return nullptr;
	}

	return d->provider->getNode(id);
}

bool FetchQueriesService::addQuery(const LazyTreeNodePtr& node) {
	if (!d->provider || !node) {
		return false;
	}

	return d->provider->add(node);
}

bool FetchQueriesService::renameQuery(const QVariant& id, const QString& name) {
	if (!d->provider) {
		return false;
	}

	const QString trimmed = name.trimmed();
	if (trimmed.isEmpty()) {
		d->setError(tr("Имя запроса не может быть пустым"));
		return false;
	}

	const auto node = d->provider->getNode(id);
	if (!node) {
		d->setError(tr("Запрос не найден"));
		return false;
	}

	node->setName(trimmed);
	if (!d->provider->update(node)) {
		d->setError(d->provider->lastError());
		return false;
	}

	return true;
}

bool FetchQueriesService::removeQuery(const QVariant& id) {
	if (!d->provider) {
		return false;
	}

	if (!d->provider->remove(id)) {
		d->setError(d->provider->lastError());
		return false;
	}

	return true;
}

bool FetchQueriesService::moveQuery(const QVariant& id, const QVariant& parentId) {
	if (!d->provider) {
		return false;
	}

	if (id == parentId) {
		d->setError(tr("Запрос не может быть родителем самому себе"));
		return false;
	}

	const auto node = d->provider->getNode(id);
	if (!node) {
		d->setError(tr("Запрос не найден"));
		return false;
	}

	node->setParentId(parentId);
	if (!d->provider->update(node)) {
		d->setError(d->provider->lastError());
		return false;
	}

	return true;
}

QString FetchQueriesService::lastError() const {
	return d->lastError;
}
