#include "DataLayer/inventories/inventories_data_provider_json_impl.h"
#include "Resources/resources.h"
#include <QDir>

class InventoriesDataProviderJsonImpl::Private {
public:
	Private(InventoriesDataProviderJsonImpl* parent) : q(parent) {
	}

	InventoriesDataProviderJsonImpl* q;
	Resources* resources;
};


InventoriesDataProviderJsonImpl::InventoriesDataProviderJsonImpl(Resources* resources)
: d(std::make_unique<Private>(this)) {
	d->resources = resources;
}

InventoriesDataProviderJsonImpl::~InventoriesDataProviderJsonImpl() = default;


void InventoriesDataProviderJsonImpl::loadInventories(std::list<QUuid>& inventories) {
	const QString basePath = d->resources->Variables.get("Resources.Path", "").toString();
	const QString path = QDir(basePath).filePath("data/inventory");

	QDir dir(path);
	if (!dir.exists()) {
		qWarning() << "InventoriesDataProviderJsonImpl: path does not exist:" << path;
		return;
	}

	const auto entries = dir.entryInfoList(QStringList() << "*.json", QDir::Files);
	for (const auto& entry : entries) {
		const QString fileName = entry.baseName();
		const QUuid id(fileName);
		if (!id.isNull()) {
			inventories.push_back(id);
		}
	}
}

void InventoriesDataProviderJsonImpl::loadEquipments(std::list<QUuid>& equipments) {
	const QString basePath = d->resources->Variables.get("Resources.Path", "").toString();
	const QString path = QDir(basePath).filePath("data/equipment");

	QDir dir(path);
	if (!dir.exists()) {
		qWarning() << "InventoriesDataProviderJsonImpl: path does not exist:" << path;
		return;
	}

	const auto entries = dir.entryInfoList(QStringList() << "*.json", QDir::Files);
	for (const auto& entry : entries) {
		const QString fileName = entry.baseName();
		const QUuid id(fileName);
		if (!id.isNull()) {
			equipments.push_back(id);
		}
	}
}
