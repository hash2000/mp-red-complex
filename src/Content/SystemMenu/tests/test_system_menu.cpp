#include "Content/SystemMenu/documents/system_menu_tree_document.h"
#include "Content/SystemMenu/services/system_menu_service.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_view.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_widget.h"
#include "basewidgets/mock_lazy_tree_provider.h"

#include <QStandardItemModel>
#include <QtTest/QtTest>

#include <memory>

class TestSystemMenu : public QObject {
	Q_OBJECT
private slots:
	void init();
	void cleanup();

	void createsItems();
	void renamesAndMovesItems();
	void rejectsEmptyNameAndProviderErrors();
	void documentHostsLazyTreeWidget();

private:
	std::unique_ptr<MockLazyTreeProvider> _provider;
	std::unique_ptr<SystemMenuService> _service;
};

void TestSystemMenu::init() {
	_provider = std::make_unique<MockLazyTreeProvider>();
	_provider->setChildren(QVariant(), {});
	_service = std::make_unique<SystemMenuService>(_provider.get());
}

void TestSystemMenu::cleanup() {
	_service.reset();
	_provider.reset();
}

void TestSystemMenu::createsItems() {
	QVariantMap properties;
	properties.insert(QStringLiteral("icon"), QStringLiteral("⚙"));

	auto item = _service->createItem(QVariant(), QStringLiteral("  Настройки  "), properties);
	QVERIFY(item);
	QCOMPARE(item->name(), QStringLiteral("Настройки"));
	QCOMPARE(item->properties().value(QStringLiteral("icon")).toString(), QStringLiteral("⚙"));
	QVERIFY(item->id().isValid());
	QCOMPARE(_provider->addedNodes.size(), 1);

	auto child = _service->createItem(item->id(), QStringLiteral("Общие"));
	QVERIFY(child);
	QCOMPARE(child->parentId(), item->id());
}

void TestSystemMenu::renamesAndMovesItems() {
	auto first = _service->createItem(QVariant(), QStringLiteral("Первый"));
	auto second = _service->createItem(QVariant(), QStringLiteral("Второй"));
	QVERIFY(first);
	QVERIFY(second);

	QVERIFY(_service->renameItem(first->id(), QStringLiteral("  Переименован  ")));
	QCOMPARE(_service->item(first->id())->name(), QStringLiteral("Переименован"));

	auto child = _service->createItem(first->id(), QStringLiteral("Дочерний"));
	QVERIFY(child);
	QVERIFY(!_service->moveItem(first->id(), child->id()));

	QVERIFY(_service->moveItem(first->id(), second->id()));
	QCOMPARE(_service->item(first->id())->parentId(), second->id());

	QVERIFY(_service->removeItem(first->id()));
	QCOMPARE(_provider->removedIds.size(), 1);
}

void TestSystemMenu::rejectsEmptyNameAndProviderErrors() {
	QVERIFY(!_service->createItem(QVariant(), QStringLiteral("   ")));
	QVERIFY(!_service->lastError().isEmpty());

	auto item = _service->createItem(QVariant(), QStringLiteral("Пункт"));
	QVERIFY(item);

	QVERIFY(!_service->createItem(QVariant(), QStringLiteral("пункт")));
	QVERIFY(!_service->lastError().isEmpty());

	_provider->updateResult = false;
	QVERIFY(!_service->renameItem(item->id(), QStringLiteral("Новое")));
	QVERIFY(!_service->lastError().isEmpty());

	QVERIFY(!_service->moveItem(item->id(), item->id()));
}

void TestSystemMenu::documentHostsLazyTreeWidget() {
	auto item = _service->createItem(QVariant(), QStringLiteral("Пункт"));
	QVERIFY(item);
	_provider->setChildren(QVariant(), { item });

	SystemMenuTreeDocument document(_service.get());

	QCOMPARE(document.title(), QStringLiteral("Системное меню"));
	QCOMPARE(document.glyph(), QStringLiteral("🏢"));
	QVERIFY(document.tree());

	const int callsBefore = _provider->getCalls.size();
	document.refresh();
	QCOMPARE(_provider->getCalls.size(), callsBefore + 1);

	auto* model = qobject_cast<QStandardItemModel*>(document.tree()->treeView()->model());
	QVERIFY(model);
	QCOMPARE(model->invisibleRootItem()->rowCount(), 1);
	QCOMPARE(model->invisibleRootItem()->child(0)->text(), QStringLiteral("Пункт"));
}

QTEST_MAIN(TestSystemMenu)
#include "test_system_menu.moc"
