#include <QtTest/QtTest>
#include "ApplicationLayer/items/items_service.h"
#include "ApplicationLayer/inventories_service.h"
#include "ApplicationLayer/inventory/inventory_service.h"
#include "ApplicationLayer/inventory/inventory_item_handler.h"
#include "ApplicationLayer/items/item_mime_data.h"

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

};

void TestInventoriesService::initTestCase() {}
void TestInventoriesService::cleanupTestCase() {}

void TestInventoriesService::init() {

}

void TestInventoriesService::cleanup() {

}


QTEST_MAIN(TestInventoriesService)

#include "test_inventories_service.moc"
