#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_data_node.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_view.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_widget.h"
#include "basewidgets/mock_lazy_tree_provider.h"

#include <QApplication>
#include <QLineEdit>
#include <QStandardItemModel>
#include <QToolButton>
#include <QtTest/QtTest>

#include <memory>

class TestLazyTreeWidget : public QObject {
	Q_OBJECT
private slots:
	void init();
	void cleanup();

	void loadsRootsOnCreation();
	void addNodeThroughToolbar();
	void searchSwitchesViewsAndBack();
	void emptySearchReturnsToNormalView();
	void deleteRemovesNode();
	void refreshAllReloadsRoots();
	void refreshNodeReloadsChildren();
	void updateNodeRenamesItem();
	void selectionChangedIsForwarded();
	void searchErrorIsForwarded();
	void exposesNamedControls();

private:
	LazyTreeWidget* widget() const {
		return _widget.get();
	}

	MockLazyTreeProvider* provider() const {
		return _provider.get();
	}

	QStandardItemModel* model() const {
		return qobject_cast<QStandardItemModel*>(_widget->treeView()->model());
	}

	QToolButton* button(const char* name) const {
		return _widget->findChild<QToolButton*>(QString::fromLatin1(name));
	}

	std::unique_ptr<MockLazyTreeProvider> _provider;
	std::unique_ptr<LazyTreeWidget> _widget;
};

void TestLazyTreeWidget::init() {
	_provider = std::make_unique<MockLazyTreeProvider>();
	_provider->setChildren(QVariant(), {});
	_widget = std::make_unique<LazyTreeWidget>(_provider.get());
	_widget->resize(400, 600);
	_widget->show();
	QCoreApplication::processEvents();
}

void TestLazyTreeWidget::cleanup() {
	_widget.reset();
	_provider.reset();
}

void TestLazyTreeWidget::loadsRootsOnCreation() {
	auto node = makeLazyTestNode(1, QVariant(), QStringLiteral("Узел"));
	markLeafTestNode(node);
	_provider->setChildren(QVariant(), { node });
	_provider->getCalls.clear();

	_widget = std::make_unique<LazyTreeWidget>(_provider.get());

	QCOMPARE(model()->invisibleRootItem()->rowCount(), 1);
	QCOMPARE(model()->invisibleRootItem()->child(0)->text(), QStringLiteral("Узел"));
	QCOMPARE(_provider->getCalls.size(), 1);
}

void TestLazyTreeWidget::addNodeThroughToolbar() {
	auto* addButton = button("LazyTreeAddButton");
	QVERIFY(addButton);
	addButton->click();
	QCoreApplication::processEvents();

	auto* editor = _widget->treeView()->viewport()->findChild<QLineEdit*>();
	QVERIFY(editor);
	editor->setText(QStringLiteral("Пункт"));
	QTest::keyClick(editor, Qt::Key_Return);
	QCoreApplication::processEvents();

	QCOMPARE(_provider->addedNodes.size(), 1);
	QCOMPARE(model()->invisibleRootItem()->rowCount(), 1);
	QCOMPARE(model()->invisibleRootItem()->child(0)->text(), QStringLiteral("Пункт"));
}

void TestLazyTreeWidget::searchSwitchesViewsAndBack() {
	auto result = makeLazyTestNode(1, QVariant(), QStringLiteral("Найден"));
	markLeafTestNode(result);
	_provider->searchResult = { result };

	_widget->searchLineEdit()->setText(QStringLiteral("най"));
	QTRY_COMPARE(_provider->searchCalls.size(), 1);

	QVERIFY(_widget->isSearchMode());
	QCOMPARE(_widget->searchView()->model()->rowCount(), 1);

	auto* backButton = button("LazyTreeBackButton");
	QVERIFY(backButton);
	backButton->click();
	QCoreApplication::processEvents();

	QVERIFY(!_widget->isSearchMode());
}

void TestLazyTreeWidget::emptySearchReturnsToNormalView() {
	_widget->searchLineEdit()->setText(QStringLiteral("най"));
	QTRY_VERIFY(_widget->isSearchMode());

	_widget->searchLineEdit()->setText(QString());
	QCoreApplication::processEvents();

	QVERIFY(!_widget->isSearchMode());
}

void TestLazyTreeWidget::deleteRemovesNode() {
	auto node = makeLazyTestNode(1, QVariant(), QStringLiteral("Узел"));
	markLeafTestNode(node);
	_provider->setChildren(QVariant(), { node });
	_widget = std::make_unique<LazyTreeWidget>(_provider.get());
	_widget->setConfirmDelete(false);

	_widget->treeView()->setCurrentIndex(model()->index(0, 0));
	button("LazyTreeDeleteButton")->click();
	QCoreApplication::processEvents();

	QCOMPARE(_provider->removedIds.size(), 1);
	QCOMPARE(model()->invisibleRootItem()->rowCount(), 0);
}

void TestLazyTreeWidget::refreshAllReloadsRoots() {
	const int callsBefore = _provider->getCalls.size();
	_widget->refreshAll();

	QCOMPARE(_provider->getCalls.size(), callsBefore + 1);
}

void TestLazyTreeWidget::refreshNodeReloadsChildren() {
	auto root = makeLazyTestNode(1, QVariant(), QStringLiteral("Корень"));
	markLazyTestNode(root);
	_provider->setChildren(QVariant(), { root });

	auto child = makeLazyTestNode(11, 1, QStringLiteral("Ребёнок"));
	markLeafTestNode(child);
	_provider->setChildren(root->id(), { child });

	_widget = std::make_unique<LazyTreeWidget>(_provider.get());
	_widget->treeView()->setCurrentIndex(model()->index(0, 0));

	button("LazyTreeRefreshButton")->click();
	QCoreApplication::processEvents();

	QCOMPARE(model()->invisibleRootItem()->child(0)->rowCount(), 1);
	QCOMPARE(model()->invisibleRootItem()->child(0)->child(0)->text(), QStringLiteral("Ребёнок"));
	QVERIFY(!_widget->treeView()->isDummyItem(model()->invisibleRootItem()->child(0)->child(0)));
}

void TestLazyTreeWidget::updateNodeRenamesItem() {
	auto node = makeLazyTestNode(1, QVariant(), QStringLiteral("Старое"));
	markLeafTestNode(node);
	_provider->setChildren(QVariant(), { node });
	_widget = std::make_unique<LazyTreeWidget>(_provider.get());

	node->setName(QStringLiteral("Новое"));
	QVERIFY(_widget->updateNode(node->id(), node));

	QCOMPARE(_provider->updatedNodes.size(), 1);
	QCOMPARE(model()->invisibleRootItem()->child(0)->text(), QStringLiteral("Новое"));
}

void TestLazyTreeWidget::selectionChangedIsForwarded() {
	auto node = makeLazyTestNode(1, QVariant(), QStringLiteral("Узел"));
	markLeafTestNode(node);
	_provider->setChildren(QVariant(), { node });
	_widget = std::make_unique<LazyTreeWidget>(_provider.get());

	int selected = 0;
	connect(_widget.get(), &LazyTreeWidget::selectionChanged, this,
		[&selected](const LazyTreeNodePtr& value) {
			if (value) {
				selected++;
			}
		});

	_widget->treeView()->setCurrentIndex(model()->index(0, 0));
	QCoreApplication::processEvents();

	QCOMPARE(selected, 1);
}

void TestLazyTreeWidget::searchErrorIsForwarded() {
	_provider->failSearch = true;

	int errors = 0;
	connect(_widget.get(), &LazyTreeWidget::errorOccurred, this, [&errors](const QString&) {
		errors++;
	});

	_widget->searchLineEdit()->setText(QStringLiteral("ошибка"));
	QTRY_COMPARE(_provider->searchCalls.size(), 1);
	QTRY_COMPARE(errors, 1);
}

void TestLazyTreeWidget::exposesNamedControls() {
	QVERIFY(_widget->searchLineEdit());
	QCOMPARE(_widget->searchLineEdit()->objectName(), QStringLiteral("LazyTreeSearchEdit"));
	QVERIFY(_widget->treeView());
	QCOMPARE(_widget->treeView()->objectName(), QStringLiteral("LazyTreeView"));
	QVERIFY(_widget->searchView());
	QVERIFY(button("LazyTreeAddButton"));
	QVERIFY(button("LazyTreeDeleteButton"));
	QVERIFY(button("LazyTreeEditButton"));
	QVERIFY(button("LazyTreeRefreshButton"));
	QVERIFY(button("LazyTreeBackButton"));
}

QTEST_MAIN(TestLazyTreeWidget)
#include "test_lazy_tree_widget.moc"
