#include <QtTest/QtTest>

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
