#include <QtTest/QtTest>
#include "ApplicationLayer/items/items_service.h"
#include "ApplicationLayer/inventories_service.h"
#include "ApplicationLayer/inventory/inventory_service.h"
#include "ApplicationLayer/inventory/inventory_item_handler.h"
#include "ApplicationLayer/items/item_mime_data.h"
#include "mock_items_data_provider.h"

class TestInventoriesService : public QObject {
	Q_OBJECT

public:
	TestInventoriesService() = default;
	~TestInventoriesService() = default;

private slots:
	void initTestCase();
	void cleanupTestCase();
	void init();
	void cleanup();

	void testMoveFullStack_PreservesItemId();

private:
	std::unique_ptr<MockItemsDataProvider> mockItemsDataProvider;
	std::unique_ptr<ItemsService> itemsService;
	std::unique_ptr<InventoriesService> inventoriesService;
	InventoryService* inventory1;
	InventoryService* inventory2;

	ItemEntity createTestEntity(const QString& id);
	InventoryService* createTestInventory(const QString& id, int rows, int cols);
};

void TestInventoriesService::initTestCase() {}
void TestInventoriesService::cleanupTestCase() {}

void TestInventoriesService::init() {
	mockItemsDataProvider = std::make_unique<MockItemsDataProvider>();
	itemsService = std::make_unique<ItemsService>(mockItemsDataProvider.get());
	itemsService->loadEntities();

	inventoriesService = std::make_unique<InventoriesService>(
		nullptr, nullptr, nullptr, itemsService.get()
	);

	inventory1 = nullptr;
	inventory2 = nullptr;
}

void TestInventoriesService::cleanup() {
	inventory2 = nullptr;
	inventory1 = nullptr;
	inventoriesService.reset();
	itemsService.reset();
	mockItemsDataProvider.reset();
}

ItemEntity TestInventoriesService::createTestEntity(const QString& id) {
	ItemEntity entity;
	entity.id = id;
	entity.name = "Test Item";
	entity.type = ItemType::Resource;
	entity.description = "Test description";
	entity.width = 1;
	entity.height = 1;
	entity.maxStack = 100;
	entity.rarity = ItemRarityType::Common;
	entity.equipmentType = ItemEquipmentType::None;
	return entity;
}

InventoryService* TestInventoriesService::createTestInventory(const QString& id, int rows, int cols) {
	auto* service = new InventoryService(itemsService.get());

	Inventory inv;
	inv.id = QString::fromUtf8(QUuid::createUuid().toByteArray(QUuid::WithoutBraces));
	inv.name = id;
	inv.rows = rows;
	inv.cols = cols;

	service->load(inv);
	return service;
}

void TestInventoriesService::testMoveFullStack_PreservesItemId() {
	auto entity = createTestEntity("test-item");
	mockItemsDataProvider->addEntity("test-item", entity);
	itemsService->loadEntities();

	const auto* originalItem = itemsService->createItemByEntity("test-item");
	QVERIFY(originalItem != nullptr);
	QString originalItemId = originalItem->id;

	inventory1 = createTestInventory("Inventory 1", 10, 10);
	inventory2 = createTestInventory("Inventory 2", 10, 10);

	QVERIFY(inventory1 != nullptr);
	QVERIFY(inventory2 != nullptr);

	ItemMimeData mimeData(*originalItem);
	mimeData.x = 0;
	mimeData.y = 0;
	mimeData.count = 1;
	bool placed = inventory1->duplicateItem(mimeData);
	QVERIFY(placed);

	const auto* itemInInv1 = inventory1->itemById(originalItemId);
	QVERIFY(itemInInv1 != nullptr);
	QCOMPARE(itemInInv1->id, originalItemId);

	auto freeSpace = inventory2->findFreeSpace(mimeData, false);
	QVERIFY(freeSpace.has_value());

	bool result = inventory2->transferItem(mimeData, freeSpace->x(), freeSpace->y());
	QVERIFY(result);

	inventory1->removeItem(mimeData);

	const auto* itemInInv1After = inventory1->itemById(originalItemId);
	QVERIFY(itemInInv1After == nullptr);

	const auto* itemInInv2 = inventory2->itemById(originalItemId);
	QVERIFY(itemInInv2 != nullptr);
	QCOMPARE(itemInInv2->id, originalItemId);

	qDebug() << "Original ID:" << originalItemId;
	qDebug() << "Item in inv2 ID:" << itemInInv2->id;
}

QTEST_MAIN(TestInventoriesService)

#include "test_inventories_service.moc"
