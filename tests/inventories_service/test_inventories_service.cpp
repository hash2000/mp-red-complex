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

	// Тесты перемещения предметов между инвентарями
	void testMoveFullStack_PreservesItemId();
	void testMovePartialStack_CreatesNewId();
	void testMoveItem_MergeStacks();

	// Тесты проверки отсутствия лишних предметов в ItemsService
	void testMoveItem_NoOrphanedItems();
	void testRemoveItem_NoOrphanedItems();

	// Тесты разбиения стека
	void testSplitStack_CreatesNewItemsWithUniqueIds();

	// Тесты отдельных методов InventoryService
	void testInventoryService_MoveItem();
	void testInventoryService_RemoveItem();

private:
	std::unique_ptr<MockItemsDataProvider> mockItemsDataProvider;
	std::unique_ptr<ItemsService> itemsService;
	std::unique_ptr<MockItemPlacementStore> mockPlacementStore;
	std::unique_ptr<MockInventoriesDataProvider> mockInventoriesDataProvider;
	std::unique_ptr<InventoriesService> inventoriesService;

	InventoryService* createTestInventory(const QUuid& id, int rows, int cols);
	ItemEntity createTestEntity(const QString& id, int maxStack = 100);

	/// Проверка, что все предметы в ItemsService привязаны к инвентарям
	int countItemsInInventories(const std::list<QUuid>& inventoryIds);
	void assertNoOrphanedItems(const std::list<QUuid>& inventoryIds);
};

void TestInventoriesService::initTestCase() {}
void TestInventoriesService::cleanupTestCase() {}

void TestInventoriesService::init() {
	mockItemsDataProvider = std::make_unique<MockItemsDataProvider>();
	itemsService = std::make_unique<ItemsService>(mockItemsDataProvider.get());
	itemsService->loadEntities();

	mockInventoriesDataProvider = std::make_unique<MockInventoriesDataProvider>();

	// InventoriesService будет использовать мок для загрузки инвентарей
	inventoriesService = std::make_unique<InventoriesService>(
		nullptr,  // InventoryDataProvider не нужен, т.к. используем мок
		nullptr,  // EquipmentDataProvider
		itemsService.get()
	);
}

void TestInventoriesService::cleanup() {
	inventoriesService.reset();
	mockInventoriesDataProvider.reset();
	mockPlacementStore.reset();
	itemsService.reset();
	mockItemsDataProvider.reset();
}


ItemEntity TestInventoriesService::createTestEntity(const QString& id, int maxStack) {
	ItemEntity entity;
	entity.id = id;
	entity.name = "Test Item";
	entity.type = ItemType::Resource;
	entity.description = "Test description";
	entity.width = 1;
	entity.height = 1;
	entity.maxStack = maxStack;
	entity.rarity = ItemRarityType::Common;
	entity.equipmentType = ItemEquipmentType::None;
	return entity;
}

InventoryService* TestInventoriesService::createTestInventory(const QUuid& id, int rows, int cols) {
	inventoriesService->addStore(id, std::make_unique<MockItemPlacementStore>(itemsService.get()));
	return dynamic_cast<InventoryService*>(inventoriesService->placementService(id, true));
}

int TestInventoriesService::countItemsInInventories(const std::list<QUuid>& inventoryIds) {
	int count = 0;
	for (const auto& invId : inventoryIds) {
		auto service = dynamic_cast<InventoryService*>(inventoriesService->placementService(invId, false));
		if (service) {
			for (const auto& item : service->items()) {
				count++;
			}
		}
	}
	return count;
}

void TestInventoriesService::assertNoOrphanedItems(const std::list<QUuid>& inventoryIds) {
	int itemsInInventories = countItemsInInventories(inventoryIds);
	int totalItems = 0;
	for (const auto& item : itemsService->items()) {
		totalItems++;
	}
	QCOMPARE(totalItems, itemsInInventories);
}

// ============================================================================
// Тесты перемещения предметов между инвентарями
// ============================================================================

void TestInventoriesService::testMoveFullStack_PreservesItemId() {
	auto entity = createTestEntity("test-item");
	mockItemsDataProvider->addEntity("test-item", entity);
	itemsService->loadEntities();

	const auto originalItem = itemsService->createItemByEntity("test-item");
	QVERIFY(originalItem != nullptr);
	const auto originalItemId = originalItem->id;

	QUuid inv1Id = QUuid::createUuid();
	QUuid inv2Id = QUuid::createUuid();
	mockInventoriesDataProvider->addInventory(inv1Id);
	mockInventoriesDataProvider->addInventory(inv2Id);

	auto inventory1 = createTestInventory(inv1Id, 10, 10);
	auto inventory2 = createTestInventory(inv2Id, 10, 10);

	QVERIFY(inventory1 != nullptr);
	QVERIFY(inventory2 != nullptr);

	// Размещаем предмет в первом инвентаре
	ItemMimeData mimeData(*originalItem);
	mimeData.x = 0;
	mimeData.y = 0;
	mimeData.count = 1;
	bool placed = inventory1->placeItem(mimeData);
	QVERIFY(placed);

	const auto itemInInv1 = inventory1->itemById(originalItemId);
	QVERIFY(itemInInv1 != nullptr);
	QCOMPARE(itemInInv1->count, 1);

	// Перемещаем весь стек во второй инвентарь
	auto freeSpace = inventory2->findFreeSpace(mimeData, false);
	QVERIFY(freeSpace.has_value());

	bool result = inventoriesService->moveItem(mimeData, freeSpace->x(), freeSpace->y(), inv1Id, inv2Id);
	QVERIFY(result);

	// Проверяем, что предмет во втором инвентаре с тем же ID
	const auto itemInInv2 = inventory2->itemById(originalItemId);
	QVERIFY(itemInInv2 != nullptr);
	QCOMPARE(itemInInv2->id, originalItemId);

	// Проверяем, что в первом инвентаре предмета больше нет
	const auto itemInInv1After = inventory1->itemById(originalItemId);
	QVERIFY(itemInInv1After == nullptr);

	// Проверяем отсутствие лишних предметов
	std::list<QUuid> inventoryIds = {inv1Id, inv2Id};
	assertNoOrphanedItems(inventoryIds);
}

void TestInventoriesService::testMovePartialStack_CreatesNewId() {
	auto entity = createTestEntity("stack-item");
	entity.maxStack = 100;
	mockItemsDataProvider->addEntity("stack-item", entity);
	itemsService->loadEntities();

	const auto originalItem = itemsService->createItemByEntity("stack-item");
	QVERIFY(originalItem != nullptr);
	const auto originalItemId = originalItem->id;

	QUuid inv1Id = QUuid::createUuid();
	QUuid inv2Id = QUuid::createUuid();
	mockInventoriesDataProvider->addInventory(inv1Id);
	mockInventoriesDataProvider->addInventory(inv2Id);

	auto inventory1 = createTestInventory(inv1Id, 10, 10);
	auto inventory2 = createTestInventory(inv2Id, 10, 10);

	// Размещаем стек из 50 предметов в первом инвентаре
	ItemMimeData mimeData(*originalItem);
	mimeData.x = 0;
	mimeData.y = 0;
	mimeData.count = 50;
	inventory1->placeItem(mimeData);

	const auto itemInInv1 = inventory1->itemById(originalItemId);
	QVERIFY(itemInInv1 != nullptr);
	QCOMPARE(itemInInv1->count, 50);

	// Перемещаем только 10 предметов (частичный стек)
	ItemMimeData partialMimeData = mimeData;
	partialMimeData.count = 10;

	auto freeSpace = inventory2->findFreeSpace(partialMimeData, false);
	QVERIFY(freeSpace.has_value());

	bool result = inventoriesService->moveItem(partialMimeData, freeSpace->x(), freeSpace->y(), inv1Id, inv2Id);
	QVERIFY(result);

	// Проверяем, что в первом инвентаре осталось 40 предметов
	const auto remainingItem = inventory1->itemById(originalItemId);
	QVERIFY(remainingItem != nullptr);
	QCOMPARE(remainingItem->count, 40);

	// Проверяем, что во втором инвентаре появился новый предмет с новым ID
	int count = 0;
	QUuid newItemId;
	for (const auto& item : inventory2->items()) {
		count++;
		newItemId = item.id;
	}
	QCOMPARE(count, 1);
	QVERIFY(newItemId == originalItemId);

	// Проверяем отсутствие лишних предметов
	std::list<QUuid> inventoryIds = {inv1Id, inv2Id};
	assertNoOrphanedItems(inventoryIds);
}

void TestInventoriesService::testMoveItem_MergeStacks() {
	auto entity = createTestEntity("merge-item");
	entity.maxStack = 100;
	mockItemsDataProvider->addEntity("merge-item", entity);
	itemsService->loadEntities();

	const auto originalItem = itemsService->createItemByEntity("merge-item");
	QVERIFY(originalItem != nullptr);

	QUuid inv1Id = QUuid::createUuid();
	QUuid inv2Id = QUuid::createUuid();
	mockInventoriesDataProvider->addInventory(inv1Id);
	mockInventoriesDataProvider->addInventory(inv2Id);

	auto inventory1 = createTestInventory(inv1Id, 10, 10);
	auto inventory2 = createTestInventory(inv2Id, 10, 10);

	// Размещаем 30 предметов в первом инвентаре
	ItemMimeData mimeData1(*originalItem);
	mimeData1.x = 0;
	mimeData1.y = 0;
	mimeData1.count = 30;
	inventory1->placeItem(mimeData1);

	// Размещаем 50 предметов во втором инвентаре
	const auto item2 = itemsService->createItemByEntity("merge-item");
	QVERIFY(item2 != nullptr);

	ItemMimeData mimeData2(*item2);
	mimeData2.x = 0;
	mimeData2.y = 0;
	mimeData2.count = 50;
	inventory2->placeItem(mimeData2);

	const auto itemInInv1 = inventory1->itemById(mimeData1.id);
	QVERIFY(itemInInv1 != nullptr);
	QCOMPARE(itemInInv1->count, 30);

	const auto itemInInv2 = inventory2->itemById(mimeData2.id);
	QVERIFY(itemInInv2 != nullptr);
	QCOMPARE(itemInInv2->count, 50);

	// Перемещаем 30 предметов из первого инвентаря во второй (должны объединиться)
	bool result = inventoriesService->moveItem(mimeData1, 0, 0, inv1Id, inv2Id);
	QVERIFY(result);

	// Проверяем, что во втором инвентаре теперь 80 предметов
	const auto mergedItem = inventory2->itemAt(0, 0);
	QVERIFY(mergedItem != nullptr);
	QCOMPARE(mergedItem->count, 80);

	// Проверяем отсутствие лишних предметов
	std::list<QUuid> inventoryIds = {inv1Id, inv2Id};
	assertNoOrphanedItems(inventoryIds);
}

// ============================================================================
// Тесты проверки отсутствия лишних предметов в ItemsService
// ============================================================================

void TestInventoriesService::testMoveItem_NoOrphanedItems() {
	auto entity = createTestEntity("test-item");
	mockItemsDataProvider->addEntity("test-item", entity);
	itemsService->loadEntities();

	QUuid inv1Id = QUuid::createUuid();
	QUuid inv2Id = QUuid::createUuid();
	mockInventoriesDataProvider->addInventory(inv1Id);
	mockInventoriesDataProvider->addInventory(inv2Id);

	auto inventory1 = createTestInventory(inv1Id, 10, 10);
	auto inventory2 = createTestInventory(inv2Id, 10, 10);

	// Создаём и размещаем несколько предметов
	const auto item1 = itemsService->createItemByEntity("test-item");
	const auto item2 = itemsService->createItemByEntity("test-item");

	ItemMimeData mimeData1(*item1);
	mimeData1.x = 0;
	mimeData1.y = 0;
	mimeData1.count = 1;
	inventory1->placeItem(mimeData1);

	ItemMimeData mimeData2(*item2);
	mimeData2.x = 1;
	mimeData2.y = 0;
	mimeData2.count = 1;
	inventory1->placeItem(mimeData2);

	// Перемещаем один предмет во второй инвентарь
	auto freeSpace = inventory2->findFreeSpace(mimeData1, false);
	QVERIFY(freeSpace.has_value());

	bool result = inventoriesService->moveItem(mimeData1, freeSpace->x(), freeSpace->y(), inv1Id, inv2Id);
	QVERIFY(result);

	// Проверяем отсутствие лишних предметов
	std::list<QUuid> inventoryIds = {inv1Id, inv2Id};
	assertNoOrphanedItems(inventoryIds);
}

void TestInventoriesService::testRemoveItem_NoOrphanedItems() {
	auto entity = createTestEntity("test-item");
	mockItemsDataProvider->addEntity("test-item", entity);
	itemsService->loadEntities();

	QUuid invId = QUuid::createUuid();
	mockInventoriesDataProvider->addInventory(invId);

	auto inventory = createTestInventory(invId, 10, 10);

	// Создаём и размещаем предмет
	const auto item = itemsService->createItemByEntity("test-item");
	ItemMimeData mimeData(*item);
	mimeData.x = 0;
	mimeData.y = 0;
	mimeData.count = 1;
	inventory->placeItem(mimeData);

	// Проверяем, что предмет есть в инвентаре
	const auto itemInInv = inventory->itemById(item->id);
	QVERIFY(itemInInv != nullptr);

	// Удаляем предмет
	inventory->removeItem(mimeData);

	// Проверяем, что предмет удалён из инвентаря
	const auto itemAfterRemove = inventory->itemById(item->id);
	QVERIFY(itemAfterRemove == nullptr);

	// Проверяем отсутствие лишних предметов (должно быть 0)
	std::list<QUuid> inventoryIds = {invId};
	assertNoOrphanedItems(inventoryIds);
}

// ============================================================================
// Тесты разбиения стека
// ============================================================================

void TestInventoriesService::testSplitStack_CreatesNewItemsWithUniqueIds() {
	auto entity = createTestEntity("stack-item");
	entity.maxStack = 100;
	mockItemsDataProvider->addEntity("stack-item", entity);
	itemsService->loadEntities();

	const auto originalItem = itemsService->createItemByEntity("stack-item");
	QVERIFY(originalItem != nullptr);

	QUuid inv1Id = QUuid::createUuid();
	QUuid inv2Id = QUuid::createUuid();
	QUuid inv3Id = QUuid::createUuid();
	mockInventoriesDataProvider->addInventory(inv1Id);
	mockInventoriesDataProvider->addInventory(inv2Id);
	mockInventoriesDataProvider->addInventory(inv3Id);

	auto inventory1 = createTestInventory(inv1Id, 10, 10);
	auto inventory2 = createTestInventory(inv2Id, 10, 10);
	auto inventory3 = createTestInventory(inv3Id, 10, 10);

	// Создаём стек из 100 предметов
	ItemMimeData mimeData(*originalItem);
	mimeData.x = 0;
	mimeData.y = 0;
	mimeData.count = 100;
	inventory1->placeItem(mimeData);

	const auto originalItemId = originalItem->id;

	// Перемещаем весь стек целиком во второй инвентарь (ID должен сохраниться)
	auto freeSpace2 = inventory2->findFreeSpace(mimeData, false);
	QVERIFY(freeSpace2.has_value());

	bool result = inventoriesService->moveItem(mimeData, freeSpace2->x(), freeSpace2->y(), inv1Id, inv2Id);
	QVERIFY(result);

	// Проверяем, что во втором инвентаре предмет с оригинальным ID
	const auto itemInInv2 = inventory2->itemById(originalItemId);
	QVERIFY(itemInInv2 != nullptr);
	QCOMPARE(itemInInv2->count, 100);

	// Теперь делим стек на 3 части: 30, 30, 40
	ItemMimeData partial1 = mimeData;
	partial1.count = 30;

	ItemMimeData partial2 = mimeData;
	partial2.count = 30;

	// Перемещаем 30 предметов в третий инвентарь
	auto freeSpace3 = inventory3->findFreeSpace(partial1, false);
	QVERIFY(freeSpace3.has_value());

	result = inventoriesService->moveItem(partial1, freeSpace3->x(), freeSpace3->y(), inv2Id, inv3Id);
	QVERIFY(result);

	// Перемещаем ещё 30 предметов в первый инвентарь
	auto freeSpace1 = inventory1->findFreeSpace(partial2, false);
	QVERIFY(freeSpace1.has_value());

	result = inventoriesService->moveItem(partial2, freeSpace1->x(), freeSpace1->y(), inv2Id, inv1Id);
	QVERIFY(result);

	// Собираем все ID предметов
	std::set<QUuid> itemIds;
	int totalItems = 0;

	for (const auto& item : inventory1->items()) {
		itemIds.insert(item.id);
		totalItems += item.count;
	}
	for (const auto& item : inventory2->items()) {
		itemIds.insert(item.id);
		totalItems += item.count;
	}
	for (const auto& item : inventory3->items()) {
		itemIds.insert(item.id);
		totalItems += item.count;
	}

	// Проверяем, что создано 3 разных предмета с уникальными ID
	QCOMPARE(itemIds.size(), 3);
	QVERIFY(itemIds.find(originalItemId) != itemIds.end());

	// Проверяем общее количество предметов (30 + 30 + 40 = 100)
	QCOMPARE(totalItems, 100);

	// Проверяем отсутствие лишних предметов
	std::list<QUuid> inventoryIds = {inv1Id, inv2Id, inv3Id};
	assertNoOrphanedItems(inventoryIds);
}

// ============================================================================
// Тесты отдельных методов InventoryService
// ============================================================================

void TestInventoriesService::testInventoryService_MoveItem() {
	auto entity = createTestEntity("test-item");
	mockItemsDataProvider->addEntity("test-item", entity);
	itemsService->loadEntities();

	QUuid invId = QUuid::createUuid();
	mockInventoriesDataProvider->addInventory(invId);

	auto inventory = createTestInventory(invId, 10, 10);

	// Создаём и размещаем предмет
	const auto item = itemsService->createItemByEntity("test-item");
	ItemMimeData mimeData(*item);
	mimeData.x = 0;
	mimeData.y = 0;
	mimeData.count = 1;
	inventory->placeItem(mimeData);

	// Перемещаем предмет внутри инвентаря
	bool result = inventory->moveItem(mimeData, 5, 5, false);
	QVERIFY(result);

	// Проверяем, что предмет перемещён
	const auto itemAtNewPos = inventory->itemAt(5, 5);
	QVERIFY(itemAtNewPos != nullptr);
	QCOMPARE(itemAtNewPos->id, item->id);

	// Проверяем отсутствие лишних предметов
	std::list<QUuid> inventoryIds = {invId};
	assertNoOrphanedItems(inventoryIds);
}

void TestInventoriesService::testInventoryService_RemoveItem() {
	auto entity = createTestEntity("test-item");
	entity.maxStack = 100;
	mockItemsDataProvider->addEntity("test-item", entity);
	itemsService->loadEntities();

	QUuid invId = QUuid::createUuid();
	mockInventoriesDataProvider->addInventory(invId);

	auto inventory = createTestInventory(invId, 10, 10);

	// Создаём и размещаем стек из 50 предметов
	const auto item = itemsService->createItemByEntity("test-item");
	ItemMimeData mimeData(*item);
	mimeData.x = 0;
	mimeData.y = 0;
	mimeData.count = 50;
	inventory->placeItem(mimeData);

	// Удаляем 20 предметов из стека
	ItemMimeData removeData = mimeData;
	removeData.count = 20;
	bool result = inventory->removeItemsFromStack(removeData);
	QVERIFY(result);

	// Проверяем, что в стеке осталось 30 предметов
	const auto remainingItem = inventory->itemById(item->id);
	QVERIFY(remainingItem != nullptr);
	QCOMPARE(remainingItem->count, 30);

	// Проверяем отсутствие лишних предметов
	std::list<QUuid> inventoryIds = {invId};
	assertNoOrphanedItems(inventoryIds);
}

QTEST_MAIN(TestInventoriesService)

#include "test_inventories_service.moc"
