#include <QtTest/QtTest>
#include "ApplicationLayer/items/items_service.h"

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


private:

	ItemEntity createTestEntity(const QString& id = "test-item");
};

void TestItemsService::initTestCase() {}
void TestItemsService::cleanupTestCase() {}

void TestItemsService::init() {

}

void TestItemsService::cleanup() {

}

QTEST_MAIN(TestItemsService)

#include "test_items_service.moc"
