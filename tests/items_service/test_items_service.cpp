#include <QtTest/QtTest>
#include "ApplicationLayer/items/items_service.h"
#include "mock_items_data_provider.h"

class TestItemsService : public QObject {
	Q_OBJECT

public:
	TestItemsService() = default;
	~TestItemsService() = default;

private slots:
	void initTestCase();
	void cleanupTestCase();
	void init();
	void cleanup();

	void testLoadEntities();
	void testEntityById();
	void testCreateItemByEntity();
	void testDuplicateItem();
	void testItemsView();

private:
	std::unique_ptr<MockItemsDataProvider> dataProvider;
	std::unique_ptr<ItemsService> service;

	ItemEntity createTestEntity(const QString& id = "test-item");
};

void TestItemsService::initTestCase() {}
void TestItemsService::cleanupTestCase() {}

void TestItemsService::init() {
	dataProvider = std::make_unique<MockItemsDataProvider>();
	service = std::make_unique<ItemsService>(dataProvider.get());
	service->loadEntities();
}

void TestItemsService::cleanup() {
	service.reset();
	dataProvider.reset();
}

ItemEntity TestItemsService::createTestEntity(const QString& id) {
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

void TestItemsService::testLoadEntities() {
	auto entity1 = createTestEntity("item-1");
	auto entity2 = createTestEntity("item-2");

	dataProvider->addEntity("item-1", entity1);
	dataProvider->addEntity("item-2", entity2);

	bool result = service->loadEntities();
	QVERIFY(result);

	int count = 0;
	for (const auto& e : service->entities()) {
		count++;
	}
	QCOMPARE(count, 2);
}

void TestItemsService::testEntityById() {
	auto entity = createTestEntity("test-entity");
	dataProvider->addEntity("test-entity", entity);
	service->loadEntities();

	const auto* foundEntity = service->entityById("test-entity");
	QVERIFY(foundEntity != nullptr);
	QCOMPARE(foundEntity->id, QString("test-entity"));

	const auto* notFoundEntity = service->entityById("nonexistent");
	QVERIFY(notFoundEntity == nullptr);
}

void TestItemsService::testCreateItemByEntity() {
	auto entity = createTestEntity("test-entity");
	dataProvider->addEntity("test-entity", entity);
	service->loadEntities();

	const auto* item = service->createItemByEntity("test-entity");
	QVERIFY(item != nullptr);
	QCOMPARE(item->entityId, QString("test-entity"));
	QVERIFY(!item->id.isEmpty());
}

void TestItemsService::testDuplicateItem() {
	auto entity = createTestEntity("test-entity");
	dataProvider->addEntity("test-entity", entity);
	service->loadEntities();

	const auto* originalItem = service->createItemByEntity("test-entity");
	QVERIFY(originalItem != nullptr);

	const auto* duplicatedItem = service->duplicate(originalItem->id);
	QVERIFY(duplicatedItem != nullptr);
	QCOMPARE(duplicatedItem->entityId, originalItem->entityId);
	QVERIFY(duplicatedItem->id != originalItem->id);
}

void TestItemsService::testItemsView() {
	auto entity = createTestEntity("test-entity");
	dataProvider->addEntity("test-entity", entity);
	service->loadEntities();

	const auto* item1 = service->createItemByEntity("test-entity");
	const auto* item2 = service->createItemByEntity("test-entity");
	const auto* item3 = service->duplicate(item1->id);

	QVERIFY(item1 != nullptr);
	QVERIFY(item2 != nullptr);
	QVERIFY(item3 != nullptr);

	int count = 0;
	for (const auto& item : service->items()) {
		count++;
	}
	QCOMPARE(count, 3);
}

QTEST_MAIN(TestItemsService)

#include "test_items_service.moc"
