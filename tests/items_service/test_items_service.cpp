#include <QtTest/QtTest>

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

};

void TestItemsService::initTestCase() {}
void TestItemsService::cleanupTestCase() {}

void TestItemsService::init() {

}

void TestItemsService::cleanup() {

}

QTEST_MAIN(TestItemsService)

#include "test_items_service.moc"
