#include <QtTest/QtTest>
#include "ApplicationLayer/items/items_service.h"
#include "mock_items_data_provider.h"

class TestItemsService : public QObject {
	Q_OBJECT

private slots:
	// Инициализация перед каждым тестом
	void initTestCase();
	void cleanupTestCase();
	void init();
	void cleanup();

	// Тесты
	void testLoadEntities();
	void testEntityById();
	void testCreateItemByEntity();
	void testDuplicateItem();
	void testItemsView();

private:
	std::unique_ptr<MockItemsDataProvider> dataProvider;
	std::unique_ptr<ItemsService> service;

	// Тестовые данные
	ItemEntity createTestEntity(const QString& id = "test-item");
};

void TestItemsService::initTestCase() {
	// Инициализация перед всеми тестами
}

void TestItemsService::cleanupTestCase() {
	// Очистка после всех тестов
}

void TestItemsService::init() {
	// Инициализация перед каждым тестом
	dataProvider = std::make_unique<MockItemsDataProvider>();
	service = std::make_unique<ItemsService>(dataProvider.get());
}

void TestItemsService::cleanup() {
	// Очистка после каждого теста
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
	// Добавляем тестовые сущности
	auto entity1 = createTestEntity("item-1");
	auto entity2 = createTestEntity("item-2");
	
	dataProvider->addEntity("item-1", entity1);
	dataProvider->addEntity("item-2", entity2);

	// Загружаем сущности
	bool result = service->loadEntities();

	// Проверяем результат
	QVERIFY(result);
	
	// Проверяем количество загруженных сущностей
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

	// Проверяем поиск существующей сущности
	const auto foundEntity = service->entityById("test-entity");
	QVERIFY(foundEntity != nullptr);
	QCOMPARE(foundEntity->id, QString("test-entity"));
	QCOMPARE(foundEntity->name, QString("Test Item"));

	// Проверяем поиск несуществующей сущности
	const auto notFoundEntity = service->entityById("nonexistent");
	QVERIFY(notFoundEntity == nullptr);
}

void TestItemsService::testCreateItemByEntity() {
	auto entity = createTestEntity("test-entity");
	dataProvider->addEntity("test-entity", entity);
	service->loadEntities();

	// Создаём предмет по сущности
	const auto item = service->createItemByEntity("test-entity");

	// Проверяем результат
	QVERIFY(item != nullptr);
	QCOMPARE(item->entityId, QString("test-entity"));
	QVERIFY(!item->id.isEmpty()); // UUID должен быть сгенерирован
	QVERIFY(item->entity != nullptr);
	QCOMPARE(item->entity->name, QString("Test Item"));

	// Проверяем, что предмет добавлен в список
	const auto foundItem = service->itemById(item->id);
	QVERIFY(foundItem != nullptr);
	QCOMPARE(foundItem->id, item->id);
}

void TestItemsService::testDuplicateItem() {
	auto entity = createTestEntity("test-entity");
	dataProvider->addEntity("test-entity", entity);
	service->loadEntities();

	// Создаём исходный предмет
	const auto originalItem = service->createItemByEntity("test-entity");
	QVERIFY(originalItem != nullptr);

	// Дублируем предмет
	const auto duplicatedItem = service->duplicate(originalItem->id);

	// Проверяем результат
	QVERIFY(duplicatedItem != nullptr);
	QCOMPARE(duplicatedItem->entityId, originalItem->entityId);
	QVERIFY(!duplicatedItem->id.isEmpty());
	
	// ID должен отличаться от оригинала
	QVERIFY(duplicatedItem->id != originalItem->id);
	
	// Сущность должна быть той же
	QCOMPARE(duplicatedItem->entity, originalItem->entity);
}

void TestItemsService::testItemsView() {
	auto entity = createTestEntity("test-entity");
	dataProvider->addEntity("test-entity", entity);
	service->loadEntities();

	// Создаём несколько предметов
	const auto item1 = service->createItemByEntity("test-entity");
	const auto item2 = service->createItemByEntity("test-entity");
	const auto item3 = service->duplicate(item1->id);

	QVERIFY(item1 != nullptr);
	QVERIFY(item2 != nullptr);
	QVERIFY(item3 != nullptr);

	// Проверяем количество предметов через items() view
	int count = 0;
	for (const auto& item : service->items()) {
		count++;
	}
	QCOMPARE(count, 3);
}

// Макрос для запуска тестов
QTEST_MAIN(TestItemsService)

#include "test_items_service.moc"
