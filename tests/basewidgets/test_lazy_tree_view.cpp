#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_builder.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_data_node.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_view.h"
#include "basewidgets/mock_lazy_tree_provider.h"

#include <QApplication>
#include <QFocusEvent>
#include <QLineEdit>
#include <QSignalSpy>
#include <QStandardItemModel>
#include <QtTest/QtTest>

#include <algorithm>
#include <memory>

class TestLazyTreeView : public QObject {
	Q_OBJECT
private slots:
	void init();
	void cleanup();

	void leafNodeDoesNotRequestChildren();
	void lazyNodeLoadsChildrenOnExpandOnce();
	void preloadedChildrenDoNotRequestProvider();
	void expandedFlagOpensNodes();
	void nestedTreeKeepsHierarchy();
	void setChildrenReplacesAndClears();

	void addNodeSavesName();
	void addNodeWithoutNameIsDiscarded();
	void addNodeDiscardedOnFocusOut();
	void renameNodePersists();
	void renameToEmptyReverts();
	void renameFailureReverts();
	void duplicateSiblingNameIsRejected();

	void removeNodePersists();
	void removeFailureKeepsNode();

	void moveNodePersists();
	void moveNodeRejectsDescendant();

	void findNodeSkipsRootAndMissingIds();
	void builderBuildsHierarchy();
	void builderKeepsOrphansAndBreaksCycles();

private:
	QStandardItemModel* model() const;
	QStandardItem* rootItem(int row) const;
	QLineEdit* editor() const;
	void typeAndCommit(const QString& text);
	void commitFocusOut();

	MockLazyTreeProvider* provider() const {
		return _provider.get();
	}

	std::unique_ptr<MockLazyTreeProvider> _provider;
	std::unique_ptr<LazyTreeView> _view;
};

namespace {

LazyTreeNodePtr makeRootWithChildren(const QVariant& id, const LazyTreeNodeList& children,
	bool expanded = false) {
	auto node = makeLazyTestNode(id, QVariant(), QStringLiteral("Root %1").arg(id.toString()));
	node->setChildren(children);
	node->setExpanded(expanded);
	return node;
}

} // namespace

QStandardItemModel* TestLazyTreeView::model() const {
	return qobject_cast<QStandardItemModel*>(_view->model());
}

QStandardItem* TestLazyTreeView::rootItem(int row) const {
	return model()->invisibleRootItem()->child(row);
}

QLineEdit* TestLazyTreeView::editor() const {
	return _view->viewport()->findChild<QLineEdit*>();
}

void TestLazyTreeView::typeAndCommit(const QString& text) {
	auto* lineEdit = editor();
	QVERIFY(lineEdit);
	lineEdit->setText(text);
	QTest::keyClick(lineEdit, Qt::Key_Return);
	QCoreApplication::processEvents();
}

void TestLazyTreeView::commitFocusOut() {
	auto* lineEdit = editor();
	QVERIFY(lineEdit);
	lineEdit->setFocus(Qt::MouseFocusReason);
	QCoreApplication::processEvents();

	// Возвращаем фокус дереву: редактор теряет фокус и делегат завершает правку.
	_view->setFocus(Qt::MouseFocusReason);
	QTest::qWait(20);
	QCoreApplication::processEvents();
}

void TestLazyTreeView::init() {
	_provider = std::make_unique<MockLazyTreeProvider>();
	_view = std::make_unique<LazyTreeView>(_provider.get());
	_view->resize(400, 600);
	_view->show();
	QCoreApplication::processEvents();
}

void TestLazyTreeView::cleanup() {
	_view.reset();
	_provider.reset();
}

void TestLazyTreeView::leafNodeDoesNotRequestChildren() {
	auto leaf = makeLazyTestNode(1, QVariant(), QStringLiteral("Leaf"));
	markLeafTestNode(leaf);
	_provider->setChildren(QVariant(), { leaf });

	_view->setNodes({ leaf });

	QCOMPARE(model()->invisibleRootItem()->rowCount(), 1);
	QCOMPARE(rootItem(0)->rowCount(), 0);
	QVERIFY(!model()->hasChildren(rootItem(0)->index()));

	_view->expand(rootItem(0)->index());
	QCOMPARE(_provider->getCalls.size(), 0);
}

void TestLazyTreeView::lazyNodeLoadsChildrenOnExpandOnce() {
	auto lazy = makeLazyTestNode(1, QVariant(), QStringLiteral("Lazy"));
	markLazyTestNode(lazy);
	_provider->setChildren(QVariant(), { lazy });

	auto first = makeLazyTestNode(11, lazy->id(), QStringLiteral("First"));
	auto second = makeLazyTestNode(12, lazy->id(), QStringLiteral("Second"));
	markLeafTestNode(first);
	markLeafTestNode(second);
	_provider->setChildren(lazy->id(), { first, second });

	_view->setNodes({ lazy });

	QCOMPARE(rootItem(0)->rowCount(), 1);
	QVERIFY(_view->isDummyItem(rootItem(0)->child(0)));
	QVERIFY(model()->hasChildren(rootItem(0)->index()));

	_view->expand(rootItem(0)->index());

	QCOMPARE(_provider->getCalls.size(), 1);
	QCOMPARE(rootItem(0)->rowCount(), 2);
	QCOMPARE(rootItem(0)->child(0)->text(), QStringLiteral("First"));
	QCOMPARE(rootItem(0)->child(1)->text(), QStringLiteral("Second"));

	_view->collapse(rootItem(0)->index());
	_view->expand(rootItem(0)->index());
	QCOMPARE(_provider->getCalls.size(), 1);
}

void TestLazyTreeView::preloadedChildrenDoNotRequestProvider() {
	auto first = makeLazyTestNode(11, 1, QStringLiteral("First"));
	markLeafTestNode(first);
	auto second = makeLazyTestNode(12, 1, QStringLiteral("Second"));
	markLeafTestNode(second);
	auto root = makeRootWithChildren(1, { first, second });

	_view->setNodes({ root });

	QCOMPARE(rootItem(0)->rowCount(), 2);
	QVERIFY(!_view->isExpanded(rootItem(0)->index()));

	_view->expand(rootItem(0)->index());

	QVERIFY(_view->isExpanded(rootItem(0)->index()));
	QCOMPARE(_provider->getCalls.size(), 0);
}

void TestLazyTreeView::expandedFlagOpensNodes() {
	auto child = makeLazyTestNode(11, 1, QStringLiteral("Child"));
	markLeafTestNode(child);
	auto root = makeRootWithChildren(1, { child }, /*expanded*/ true);

	_view->setNodes({ root });

	QVERIFY(_view->isExpanded(rootItem(0)->index()));
}

void TestLazyTreeView::nestedTreeKeepsHierarchy() {
	auto grandChild = makeLazyTestNode(111, 11, QStringLiteral("Grand child"));
	markLeafTestNode(grandChild);
	auto child = makeLazyTestNode(11, 1, QStringLiteral("Child"));
	child->setChildren({ grandChild });
	child->setExpanded(true);
	auto root = makeRootWithChildren(1, { child }, /*expanded*/ true);

	_view->setNodes({ root });

	QCOMPARE(rootItem(0)->rowCount(), 1);
	QCOMPARE(rootItem(0)->child(0)->text(), QStringLiteral("Child"));
	QCOMPARE(rootItem(0)->child(0)->rowCount(), 1);
	QCOMPARE(rootItem(0)->child(0)->child(0)->text(), QStringLiteral("Grand child"));
	QVERIFY(_view->isExpanded(rootItem(0)->index()));
	QVERIFY(_view->isExpanded(rootItem(0)->child(0)->index()));
}

void TestLazyTreeView::setChildrenReplacesAndClears() {
	auto child = makeLazyTestNode(11, 1, QStringLiteral("Old"));
	markLeafTestNode(child);
	auto root = makeRootWithChildren(1, { child });
	_view->setNodes({ root });

	auto updated = makeLazyTestNode(12, 1, QStringLiteral("New"));
	markLeafTestNode(updated);
	_view->setChildren(root->id(), { updated });

	QCOMPARE(rootItem(0)->rowCount(), 1);
	QCOMPARE(rootItem(0)->child(0)->text(), QStringLiteral("New"));

	_view->setChildren(root->id(), {});
	QCOMPARE(rootItem(0)->rowCount(), 0);
	QVERIFY(!model()->hasChildren(rootItem(0)->index()));
}

void TestLazyTreeView::addNodeSavesName() {
	_provider->setChildren(QVariant(), {});
	_view->setNodes({});

	int added = 0;
	connect(_view.get(), &LazyTreeView::nodeAdded, this, [&added] { added++; });

	auto node = std::make_shared<LazyTreeDataNode>();
	QVERIFY(_view->addNodeAndStartEdit(nullptr, node));

	QCOMPARE(model()->invisibleRootItem()->rowCount(), 1);
	QCOMPARE(rootItem(0)->text(), QString());
	QVERIFY(_view->isTemporaryItem(rootItem(0)));

	typeAndCommit(QStringLiteral("Новый"));

	QCOMPARE(_provider->addedNodes.size(), 1);
	QCOMPARE(_provider->addedNodes.first()->name(), QStringLiteral("Новый"));
	QCOMPARE(rootItem(0)->text(), QStringLiteral("Новый"));
	QVERIFY(!_view->isTemporaryItem(rootItem(0)));
	QVERIFY(!rootItem(0)->flags().testFlag(Qt::ItemIsEditable));
	QVERIFY(rootItem(0)->flags().testFlag(Qt::ItemIsDragEnabled));
	QCOMPARE(added, 1);
}

void TestLazyTreeView::addNodeWithoutNameIsDiscarded() {
	_provider->setChildren(QVariant(), {});
	_view->setNodes({});

	int added = 0;
	connect(_view.get(), &LazyTreeView::nodeAdded, this, [&added] { added++; });

	QVERIFY(_view->addNodeAndStartEdit(nullptr, std::make_shared<LazyTreeDataNode>()));
	QCOMPARE(model()->invisibleRootItem()->rowCount(), 1);

	typeAndCommit(QString());

	QCOMPARE(_provider->addedNodes.size(), 0);
	QCOMPARE(model()->invisibleRootItem()->rowCount(), 0);
	QCOMPARE(added, 0);
}

void TestLazyTreeView::addNodeDiscardedOnFocusOut() {
	_provider->setChildren(QVariant(), {});
	_view->setNodes({});

	QVERIFY(_view->addNodeAndStartEdit(nullptr, std::make_shared<LazyTreeDataNode>()));
	QCOMPARE(model()->invisibleRootItem()->rowCount(), 1);

	commitFocusOut();

	QCOMPARE(_provider->addedNodes.size(), 0);
	QCOMPARE(model()->invisibleRootItem()->rowCount(), 0);
}

void TestLazyTreeView::renameNodePersists() {
	auto node = makeLazyTestNode(1, QVariant(), QStringLiteral("Старое"));
	markLeafTestNode(node);
	_provider->setChildren(QVariant(), { node });
	_view->setNodes({ node });

	int renamed = 0;
	connect(_view.get(), &LazyTreeView::nodeRenamed, this, [&renamed](const LazyTreeNodePtr&, const QString&) {
		renamed++;
	});

	_view->startEdit(rootItem(0)->index());
	typeAndCommit(QStringLiteral("Новое"));

	QCOMPARE(_provider->updatedNodes.size(), 1);
	QCOMPARE(node->name(), QStringLiteral("Новое"));
	QCOMPARE(rootItem(0)->text(), QStringLiteral("Новое"));
	QCOMPARE(renamed, 1);
}

void TestLazyTreeView::renameToEmptyReverts() {
	auto node = makeLazyTestNode(1, QVariant(), QStringLiteral("Старое"));
	markLeafTestNode(node);
	_provider->setChildren(QVariant(), { node });
	_view->setNodes({ node });

	_view->startEdit(rootItem(0)->index());
	typeAndCommit(QString());

	QCOMPARE(_provider->updatedNodes.size(), 0);
	QCOMPARE(node->name(), QStringLiteral("Старое"));
	QCOMPARE(rootItem(0)->text(), QStringLiteral("Старое"));
}

void TestLazyTreeView::renameFailureReverts() {
	auto node = makeLazyTestNode(1, QVariant(), QStringLiteral("Старое"));
	markLeafTestNode(node);
	_provider->setChildren(QVariant(), { node });
	_view->setNodes({ node });

	_provider->updateResult = false;

	int errors = 0;
	connect(_view.get(), &LazyTreeView::errorOccurred, this, [&errors] { errors++; });

	_view->startEdit(rootItem(0)->index());
	typeAndCommit(QStringLiteral("Новое"));

	QCOMPARE(node->name(), QStringLiteral("Старое"));
	QCOMPARE(rootItem(0)->text(), QStringLiteral("Старое"));
	QCOMPARE(errors, 1);
}

void TestLazyTreeView::duplicateSiblingNameIsRejected() {
	auto first = makeLazyTestNode(1, QVariant(), QStringLiteral("Первый"));
	markLeafTestNode(first);
	auto second = makeLazyTestNode(2, QVariant(), QStringLiteral("Второй"));
	markLeafTestNode(second);
	_provider->setChildren(QVariant(), { first, second });
	_view->setNodes({ first, second });

	int errors = 0;
	connect(_view.get(), &LazyTreeView::errorOccurred, this, [&errors] { errors++; });

	_view->startEdit(rootItem(1)->index());
	typeAndCommit(QStringLiteral("первый"));

	QCOMPARE(second->name(), QStringLiteral("Второй"));
	QCOMPARE(rootItem(1)->text(), QStringLiteral("Второй"));
	QCOMPARE(_provider->updatedNodes.size(), 0);
	QCOMPARE(errors, 1);
}

void TestLazyTreeView::removeNodePersists() {
	auto node = makeLazyTestNode(1, QVariant(), QStringLiteral("Узел"));
	markLeafTestNode(node);
	_provider->setChildren(QVariant(), { node });
	_view->setNodes({ node });

	_view->setCurrentIndex(rootItem(0)->index());
	_view->removeSelectedNode();

	QCOMPARE(_provider->removedIds.size(), 1);
	QCOMPARE(_provider->removedIds.first(), node->id());
	QCOMPARE(model()->invisibleRootItem()->rowCount(), 0);
}

void TestLazyTreeView::removeFailureKeepsNode() {
	auto node = makeLazyTestNode(1, QVariant(), QStringLiteral("Узел"));
	markLeafTestNode(node);
	_provider->setChildren(QVariant(), { node });
	_view->setNodes({ node });

	_provider->removeResult = false;

	int errors = 0;
	connect(_view.get(), &LazyTreeView::errorOccurred, this, [&errors] { errors++; });

	_view->setCurrentIndex(rootItem(0)->index());
	_view->removeSelectedNode();

	QCOMPARE(model()->invisibleRootItem()->rowCount(), 1);
	QCOMPARE(errors, 1);
}

void TestLazyTreeView::moveNodePersists() {
	auto first = makeLazyTestNode(1, QVariant(), QStringLiteral("A"));
	markLeafTestNode(first);
	auto second = makeLazyTestNode(2, QVariant(), QStringLiteral("B"));
	markLeafTestNode(second);
	_provider->setChildren(QVariant(), { first, second });
	_view->setNodes({ first, second });

	int moved = 0;
	connect(_view.get(), &LazyTreeView::nodeMoved, this, [&moved] { moved++; });

	QVERIFY(_view->moveNode(first->id(), second->id()));

	QCOMPARE(_provider->updatedNodes.size(), 1);
	QCOMPARE(first->parentId(), second->id());
	QCOMPARE(model()->invisibleRootItem()->rowCount(), 1);
	QCOMPARE(rootItem(0)->text(), QStringLiteral("B"));
	QCOMPARE(rootItem(0)->rowCount(), 1);
	QCOMPARE(rootItem(0)->child(0)->text(), QStringLiteral("A"));
	QCOMPARE(moved, 1);
}

void TestLazyTreeView::moveNodeRejectsDescendant() {
	auto child = makeLazyTestNode(11, 1, QStringLiteral("Child"));
	markLeafTestNode(child);
	auto root = makeRootWithChildren(1, { child });
	_view->setNodes({ root });

	QVERIFY(!_view->moveNode(root->id(), child->id()));
	QCOMPARE(_provider->updatedNodes.size(), 0);
}

void TestLazyTreeView::findNodeSkipsRootAndMissingIds() {
	auto node = makeLazyTestNode(1, QVariant(), QStringLiteral("Узел"));
	markLeafTestNode(node);
	_provider->setChildren(QVariant(), { node });
	_view->setNodes({ node });

	QVERIFY(_view->findNode(node->id()) != nullptr);
	QVERIFY(_view->findNode(QVariant()) == nullptr);
	QVERIFY(_view->findNode(999) == nullptr);
	QCOMPARE(_view->selectedNodeId(), QVariant());
}

void TestLazyTreeView::builderBuildsHierarchy() {
	auto root = makeLazyTestNode(1, QVariant(), QStringLiteral("Root"));
	markLeafTestNode(root);
	auto child = makeLazyTestNode(2, 1, QStringLiteral("Child"));
	markLeafTestNode(child);
	auto grandChild = makeLazyTestNode(3, 2, QStringLiteral("Grand child"));
	markLeafTestNode(grandChild);

	const auto roots = LazyTreeBuilder::buildFromFlat({ grandChild, root, child });

	QCOMPARE(roots.size(), 1);
	QCOMPARE(roots.front(), root);
	QVERIFY(root->children().has_value());
	QCOMPARE(root->children()->size(), 1);
	QCOMPARE(root->children()->front(), child);
	QVERIFY(child->children().has_value());
	QCOMPARE(child->children()->front(), grandChild);
}

void TestLazyTreeView::builderKeepsOrphansAndBreaksCycles() {
	auto root = makeLazyTestNode(1, QVariant(), QStringLiteral("Root"));
	markLeafTestNode(root);
	auto orphan = makeLazyTestNode(2, 999, QStringLiteral("Orphan"));
	markLeafTestNode(orphan);
	auto selfParent = makeLazyTestNode(3, 3, QStringLiteral("Self"));
	markLeafTestNode(selfParent);
	auto cycleA = makeLazyTestNode(4, 5, QStringLiteral("Cycle A"));
	markLeafTestNode(cycleA);
	auto cycleB = makeLazyTestNode(5, 4, QStringLiteral("Cycle B"));
	markLeafTestNode(cycleB);

	const auto roots = LazyTreeBuilder::buildFromFlat({ root, orphan, selfParent, cycleA, cycleB });

	QCOMPARE(roots.size(), size_t(5));
	const auto contains = [&roots](const LazyTreeNodePtr& node) {
		return std::find(roots.begin(), roots.end(), node) != roots.end();
	};
	QVERIFY(contains(root));
	QVERIFY(contains(orphan));
	QVERIFY(contains(selfParent));
	QVERIFY(contains(cycleA));
	QVERIFY(contains(cycleB));
	QVERIFY(cycleA->children().value_or(LazyTreeNodeList()).empty());
	QVERIFY(cycleB->children().value_or(LazyTreeNodeList()).empty());
}

QTEST_MAIN(TestLazyTreeView)
#include "test_lazy_tree_view.moc"
