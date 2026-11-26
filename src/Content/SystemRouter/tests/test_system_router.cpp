#include "Content/SystemRouter/documents/system_router_node_properties_document.h"
#include "Content/SystemRouter/documents/system_router_node_view_document.h"
#include "Content/SystemRouter/documents/system_router_tree_document.h"
#include "Content/SystemRouter/models/system_router_mime.h"
#include "Content/SystemRouter/services/system_router_command_service.h"
#include "Content/SystemRouter/services/system_router_service.h"
#include "Content/SystemRouter/widgets/system_router_node_view_widget.h"
#include "Content/SystemRouter/widgets/system_router_script_dialog.h"
#include "Content/SystemRouter/widgets/system_router_tree_widget.h"
#include "Libs/BaseWidgets/properties/properties_list_widget.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_node_glyph.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_view.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_widget.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/command_processor.h"
#include "Libs/Engine/services/services_registry.h"
#include "Libs/Engine/ui/main_window.h"
#include "basewidgets/mock_lazy_tree_provider.h"

#include <QApplication>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QStandardItemModel>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtTest/QtTest>

#include <memory>

namespace {
const QString kCommandId = QStringLiteral("11111111-1111-1111-1111-111111111111");
const QString kSecondCommandId = QStringLiteral("22222222-2222-2222-2222-222222222222");

class MockCommandsProvider final : public ISystemRouterCommandsProvider {
public:
	QList<SystemRouterCommand> stored;

	QList<SystemRouterCommand> commands() const override {
		return stored;
	}

	std::optional<SystemRouterCommand> command(const QString& id) const override {
		const auto normalized = normalizeSystemRouterCommandId(id);
		for (const auto& command : stored) {
			if (command.id == normalized) {
				return command;
			}
		}
		return std::nullopt;
	}

	bool saveCommand(const SystemRouterCommand& command) override {
		for (auto& storedCommand : stored) {
			if (storedCommand.id == command.id) {
				storedCommand = command;
				return true;
			}
		}
		stored.append(command);
		return true;
	}

	bool removeCommand(const QString& id) override {
		const auto normalized = normalizeSystemRouterCommandId(id);
		for (int i = 0; i < stored.size(); i++) {
			if (stored.at(i).id == normalized) {
				stored.removeAt(i);
				return true;
			}
		}
		return false;
	}
};

/// Контроллер команд без процессора и сервисов: нужен только для контекста
/// документов и оболочки MainWindow в тестах активации узла.
class TestCommandController : public CommandController {
public:
	TestCommandController()
		: CommandController(nullptr, nullptr) {
	}

	~TestCommandController() override = default;

	CommandProcessor* commandProcessor() const override {
		return nullptr;
	}

	ServicesRegistry* services() const override {
		return nullptr;
	}

private:
	void initContext() override {
	}
};

/// Значение редактора PropertiesListWidget/просмотра по идентификатору
/// свойства.
template<typename T>
T* findEditorByProperty(QWidget* root, const QString& propertyId) {
	for (auto* child : root->findChildren<T*>()) {
		if (child->property("propertyId").toString() == propertyId) {
			return child;
		}
	}

	return nullptr;
}
} // namespace

class TestSystemRouter : public QObject {
	Q_OBJECT
private slots:
	void init();
	void cleanup();

	void createsItems();
	void renamesAndMovesItems();
	void rejectsEmptyNameAndProviderErrors();
	void documentHostsLazyTreeWidget();
	void nodePropertiesDocument();
	void nodePropertiesAlwaysExpanded();
	void nodePropertiesGlyph();
	void codeEditorPropertyUsesButton();
	void doubleClickEmitsNodeActivated();
	void doubleClickWithoutCommandTogglesNode();
	void doubleClickWithParametersOpensView();
	void doubleClickWithHiddenParametersTogglesNode();
	void nodeViewDocumentShowsSortedParameters();
	void nodeViewCopyButtonsCopyNameAndValue();
	void nodeViewKeepsRowsAtTopAndScrolls();
	void nodeViewRefreshesOnNodeChange();
	void scriptDialogOffersView();
	void commandServiceCrud();
	void rendersScriptParameters();
	void previewsForNodeFindsCommands();
	void scriptDialogShowsPreview();

private:
	std::unique_ptr<MockLazyTreeProvider> _provider;
	std::unique_ptr<SystemRouterService> _service;
};

void TestSystemRouter::init() {
	_provider = std::make_unique<MockLazyTreeProvider>();
	_provider->setChildren(QVariant(), {});
	_service = std::make_unique<SystemRouterService>(_provider.get());
}

void TestSystemRouter::cleanup() {
	_service.reset();
	_provider.reset();
}

void TestSystemRouter::createsItems() {
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

void TestSystemRouter::renamesAndMovesItems() {
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

void TestSystemRouter::rejectsEmptyNameAndProviderErrors() {
	QVERIFY(!_service->createItem(QVariant(), QStringLiteral("   ")));
	QVERIFY(!_service->lastError().isEmpty());

	auto item = _service->createItem(QVariant(), QStringLiteral("Узел"));
	QVERIFY(item);

	QVERIFY(!_service->createItem(QVariant(), QStringLiteral("узел")));
	QVERIFY(!_service->lastError().isEmpty());

	_provider->updateResult = false;
	QVERIFY(!_service->renameItem(item->id(), QStringLiteral("Новое")));
	QVERIFY(!_service->lastError().isEmpty());

	QVERIFY(!_service->moveItem(item->id(), item->id()));
}

void TestSystemRouter::documentHostsLazyTreeWidget() {
	auto item = _service->createItem(QVariant(), QStringLiteral("Узел"));
	QVERIFY(item);
	_provider->setChildren(QVariant(), { item });

	SystemRouterTreeDocument document(_service.get());

	QCOMPARE(document.title(), QStringLiteral("Системный маршрутизатор"));
	QCOMPARE(document.glyph(), QStringLiteral("🧭"));
	QVERIFY(document.tree());

	const int callsBefore = _provider->getCalls.size();
	document.refresh();
	QCOMPARE(_provider->getCalls.size(), callsBefore + 1);

	auto* model = qobject_cast<QStandardItemModel*>(document.tree()->treeView()->model());
	QVERIFY(model);
	QCOMPARE(model->invisibleRootItem()->rowCount(), 1);
	QCOMPARE(model->invisibleRootItem()->child(0)->text(), QStringLiteral("Узел"));
}

void TestSystemRouter::nodePropertiesDocument() {
	auto item = _service->createItem(QVariant(), QStringLiteral("Узел"));
	QVERIFY(item);

	item->setProperty(QStringLiteral("count"), 1);
	item->setPropertyMimeTypes(QStringLiteral("count"),
		{ QStringLiteral("application/x-integer") });
	item->setProperty(QStringLiteral("title"), QStringLiteral("Текст"));
	item->setPropertyMimeTypes(QStringLiteral("title"),
		{ QStringLiteral("text/plain") });
	QVERIFY(_service->updateItem(item));

	SystemRouterNodePropertiesDocument document(_service.get(), nullptr, item->id());
	auto* list = document.findChild<PropertiesListWidget*>(
		QStringLiteral("SystemRouterNodePropertiesList"));
	QVERIFY(list);
	QCOMPARE(list->propertyCount(), 2);

	QVERIFY(document.addProperty(QStringLiteral("flag"),
		{ QStringLiteral("application/x-boolean") }));
	QCOMPARE(list->propertyCount(), 3);
	QCOMPARE(item->properties().value(QStringLiteral("flag")).toBool(), false);
	QCOMPARE(item->propertyMimeTypes(QStringLiteral("flag")),
		QStringList({ QStringLiteral("application/x-boolean") }));

	QVERIFY(document.addProperty(QStringLiteral("script"),
		{ QString::fromLatin1(SystemRouterMime::kCommand) }, kCommandId));
	QCOMPARE(item->propertyMimeTypes(QStringLiteral("script")),
		QStringList({ QString::fromLatin1(SystemRouterMime::kCommand) }));

	QVERIFY(document.setPropertyValue(QStringLiteral("count"), 42));
	QCOMPARE(item->properties().value(QStringLiteral("count")).toInt(), 42);

	QVERIFY(document.setPropertyMimeTypes(QStringLiteral("count"),
		{ QStringLiteral("text/plain"), QStringLiteral("application/x-integer") }));
	QCOMPARE(item->propertyMimeTypes(QStringLiteral("count")),
		QStringList({ QStringLiteral("text/plain"), QStringLiteral("application/x-integer") }));

	QVERIFY(document.removeProperty(QStringLiteral("flag")));
	QCOMPARE(list->propertyCount(), 3);
	QVERIFY(!document.removeProperty(QStringLiteral("missing")));
}

void TestSystemRouter::nodePropertiesAlwaysExpanded() {
	auto item = _service->createItem(QVariant(), QStringLiteral("Узел"));
	QVERIFY(item);

	SystemRouterNodePropertiesDocument document(_service.get(), nullptr, item->id());
	auto* check = document.findChild<QCheckBox*>(QStringLiteral("SystemRouterAlwaysExpanded"));
	QVERIFY(check);
	QVERIFY(check->isEnabled());
	QVERIFY(!check->isChecked());
	QVERIFY(!document.alwaysExpanded());

	// Пользователь включает параметр: значение сохраняет поставщик.
	check->setChecked(true);
	QCOMPARE(_provider->expandedCalls.size(), 1);
	QCOMPARE(_provider->expandedCalls.front().first, item->id());
	QCOMPARE(_provider->expandedCalls.front().second, true);
	QVERIFY(check->isChecked());
	QVERIFY(document.alwaysExpanded());
	QVERIFY(item->expanded());

	// Ошибка сохранения возвращает флажок в прежнее состояние.
	_provider->expandedResult = false;
	QVERIFY(!document.setAlwaysExpanded(false));
	QVERIFY(check->isChecked());
	QVERIFY(document.alwaysExpanded());
}

void TestSystemRouter::nodePropertiesGlyph() {
	auto item = _service->createItem(QVariant(), QStringLiteral("Узел"));
	QVERIFY(item);

	SystemRouterNodePropertiesDocument document(_service.get(), nullptr, item->id());
	auto* glyphEdit = document.findChild<QLineEdit*>(QStringLiteral("SystemRouterGlyphEdit"));
	QVERIFY(glyphEdit);
	QVERIFY(glyphEdit->isEnabled());
	QCOMPARE(glyphEdit->text(), QString());
	QCOMPARE(document.glyph(), QString());

	// Значение из поля сохраняется в свойство узла.
	glyphEdit->setText(QStringLiteral("⚙"));
	QVERIFY(QMetaObject::invokeMethod(glyphEdit, "editingFinished"));
	QCOMPARE(document.glyph(), QStringLiteral("⚙"));
	QCOMPARE(LazyTreeNodeGlyph::assigned(item), QStringLiteral("⚙"));

	// Глиф редактируется полем, а не как обычное свойство узла.
	auto* list = document.findChild<PropertiesListWidget*>(
		QStringLiteral("SystemRouterNodePropertiesList"));
	QVERIFY(list);
	QCOMPARE(list->propertyCount(), 0);

	document.refresh();
	QCOMPARE(glyphEdit->text(), QStringLiteral("⚙"));

	// Пустое значение возвращает глиф по умолчанию.
	QVERIFY(document.setGlyph(QString()));
	QCOMPARE(document.glyph(), QString());
	QCOMPARE(glyphEdit->text(), QString());
	QVERIFY(!item->properties().contains(LazyTreeNodeGlyph::propertyName()));
}

void TestSystemRouter::codeEditorPropertyUsesButton() {
	auto item = _service->createItem(QVariant(), QStringLiteral("Узел"));
	QVERIFY(item);
	item->setProperty(QStringLiteral("script"), kCommandId);
	item->setPropertyMimeTypes(QStringLiteral("script"),
		{ QString::fromLatin1(SystemRouterMime::kCommand),
			QString::fromLatin1(SystemRouterMime::kCommandParameter),
			QString::fromLatin1(SystemRouterMime::kContextVariable) });
	QVERIFY(_service->updateItem(item));

	SystemRouterNodePropertiesDocument document(_service.get(), nullptr, item->id());
	QToolButton* editor = nullptr;
	for (auto* button : document.findChildren<QToolButton*>()) {
		if (button->property("propertyId").toString() == QStringLiteral("script")) {
			editor = button;
			break;
		}
	}

	QVERIFY(editor);
	QCOMPARE(editor->text(), QStringLiteral("..."));
}

void TestSystemRouter::doubleClickEmitsNodeActivated() {
	auto item = _service->createItem(QVariant(), QStringLiteral("Узел"));
	QVERIFY(item);
	_provider->setChildren(QVariant(), { item });

	SystemRouterTreeWidget widget(_service.get());
	int activated = 0;
	LazyTreeNodePtr activatedNode;
	connect(&widget, &LazyTreeWidget::nodeActivated, this,
		[&activated, &activatedNode](const LazyTreeNodePtr& node) {
			activated++;
			activatedNode = node;
		});

	auto* model = qobject_cast<QStandardItemModel*>(widget.treeView()->model());
	QVERIFY(model);
	QVERIFY(model->rowCount() > 0);
	const QModelIndex index = model->index(0, 0);

	// QTreeView::activated — путь двойного щелчка/Enter.
	const bool invoked = QMetaObject::invokeMethod(widget.treeView(), "activated",
		Qt::DirectConnection, Q_ARG(QModelIndex, index));
	QVERIFY(invoked);
	QCOMPARE(activated, 1);
	QVERIFY(activatedNode == item);
}

void TestSystemRouter::doubleClickWithoutCommandTogglesNode() {
	auto root = _service->createItem(QVariant(), QStringLiteral("Root"));
	QVERIFY(root);
	auto child = _service->createItem(root->id(), QStringLiteral("Child"));
	QVERIFY(child);

	// Узел должен быть ленивым, чтобы у него был дочерний элемент для
	// разворачивания.
	root->setChildren(LazyTreeNodeList());
	_provider->setChildren(root->id(), { child });

	SystemRouterTreeDocument document(_service.get(), nullptr);
	auto* view = document.tree()->treeView();
	auto* model = qobject_cast<QStandardItemModel*>(view->model());
	QVERIFY(model);
	const QModelIndex index = model->index(0, 0);
	QVERIFY(index.isValid());

	QVERIFY(!view->isExpanded(index));
	QVERIFY(QMetaObject::invokeMethod(view, "activated", Qt::DirectConnection,
		Q_ARG(QModelIndex, index)));
	QVERIFY(view->isExpanded(index));
	QVERIFY(QMetaObject::invokeMethod(view, "activated", Qt::DirectConnection,
		Q_ARG(QModelIndex, index)));
	QVERIFY(!view->isExpanded(index));
}

void TestSystemRouter::doubleClickWithParametersOpensView() {
	auto root = _service->createItem(QVariant(), QStringLiteral("Root"));
	QVERIFY(root);
	QVERIFY(root->setProperty(QStringLiteral("title"), QStringLiteral("Текст")));
	QVERIFY(root->setPropertyMimeTypes(QStringLiteral("title"),
		{ QStringLiteral("text/plain") }));
	QVERIFY(_service->updateItem(root));
	_provider->setChildren(QVariant(), { root });

	// Оболочка нужна документу: он создаёт просмотр через MainWindow.
	TestCommandController controller;
	MainWindow window(controller.globalContext(), nullptr, false);

	SystemRouterTreeDocument document(_service.get(), nullptr);
	document.setContext(controller.globalContext()->createScopedContext(
		QStringLiteral("system-router-view-test")));

	auto* view = document.tree()->treeView();
	auto* model = qobject_cast<QStandardItemModel*>(view->model());
	QVERIFY(model);
	const QModelIndex index = model->index(0, 0);
	QVERIFY(index.isValid());

	QVERIFY(QMetaObject::invokeMethod(view, "activated", Qt::DirectConnection,
		Q_ARG(QModelIndex, index)));

	const auto documentId = QStringLiteral("system-router-node-view-%1")
		.arg(root->id().toString());
	auto* viewDocument = qobject_cast<SystemRouterNodeViewDocument*>(window.document(documentId));
	QVERIFY(viewDocument);
	QCOMPARE(viewDocument->nodeId(), root->id());
	QVERIFY(viewDocument->view());
	QCOMPARE(viewDocument->view()->propertyNames(),
		QStringList({ QStringLiteral("title") }));
	QCOMPARE(viewDocument->title(), QStringLiteral("Просмотр: Root"));
}

void TestSystemRouter::doubleClickWithHiddenParametersTogglesNode() {
	auto root = _service->createItem(QVariant(), QStringLiteral("Root"));
	QVERIFY(root);
	QVERIFY(root->setProperty(QStringLiteral("internal"), QStringLiteral("служебное")));
	QVERIFY(root->setPropertyMimeTypes(QStringLiteral("internal"),
		{ QString::fromLatin1(SystemRouterMime::kHidden), QStringLiteral("text/plain") }));
	QVERIFY(_service->updateItem(root));

	auto child = _service->createItem(root->id(), QStringLiteral("Child"));
	QVERIFY(child);

	// Узел ленивый: у него есть дочерний элемент для разворачивания.
	root->setChildren(LazyTreeNodeList());
	_provider->setChildren(QVariant(), { root });
	_provider->setChildren(root->id(), { child });

	SystemRouterTreeDocument document(_service.get(), nullptr);
	auto* view = document.tree()->treeView();
	auto* model = qobject_cast<QStandardItemModel*>(view->model());
	QVERIFY(model);
	const QModelIndex index = model->index(0, 0);
	QVERIFY(index.isValid());

	QVERIFY(!view->isExpanded(index));
	QVERIFY(QMetaObject::invokeMethod(view, "activated", Qt::DirectConnection,
		Q_ARG(QModelIndex, index)));
	QVERIFY(view->isExpanded(index));
}

void TestSystemRouter::nodeViewDocumentShowsSortedParameters() {
	auto item = _service->createItem(QVariant(), QStringLiteral("Узел"));
	QVERIFY(item);
	QVERIFY(item->setProperty(QStringLiteral("title"), QStringLiteral("Текст")));
	QVERIFY(item->setPropertyMimeTypes(QStringLiteral("title"),
		{ QStringLiteral("text/plain") }));
	QVERIFY(item->setProperty(QStringLiteral("count"), 3));
	QVERIFY(item->setPropertyMimeTypes(QStringLiteral("count"),
		{ QStringLiteral("application/x-integer") }));
	QVERIFY(item->setProperty(QStringLiteral("token"), QStringLiteral("секрет")));
	QVERIFY(item->setPropertyMimeTypes(QStringLiteral("token"),
		{ QString::fromLatin1(SystemRouterMime::kSecret), QStringLiteral("text/plain") }));
	QVERIFY(item->setProperty(QStringLiteral("internal"), QStringLiteral("служебное")));
	QVERIFY(item->setPropertyMimeTypes(QStringLiteral("internal"),
		{ QString::fromLatin1(SystemRouterMime::kHidden), QStringLiteral("text/plain") }));
	QVERIFY(item->setProperty(LazyTreeNodeGlyph::propertyName(), QStringLiteral("⚙")));
	QVERIFY(item->setPropertyMimeTypes(LazyTreeNodeGlyph::propertyName(),
		{ QStringLiteral("text/plain") }));
	QVERIFY(item->setProperty(QStringLiteral("script"), kCommandId));
	QVERIFY(item->setPropertyMimeTypes(QStringLiteral("script"),
		{ QString::fromLatin1(SystemRouterMime::kCommand) }));
	QVERIFY(_service->updateItem(item));

	MockCommandsProvider commandProvider;
	commandProvider.stored.append(SystemRouterCommand{ kCommandId, 2,
		QStringLiteral("Авторизация"), QStringLiteral("authorize url={{url}}") });
	SystemRouterCommandService commandService(&commandProvider);

	SystemRouterNodeViewDocument document(_service.get(), &commandService, item->id());
	auto* view = document.view();
	QVERIFY(view);
	QCOMPARE(view->propertyNames(), QStringList({ QStringLiteral("count"),
		QStringLiteral("script"), QStringLiteral("title"), QStringLiteral("token") }));
	QCOMPARE(document.title(), QStringLiteral("Просмотр: Узел"));

	// Информация об узле: глиф, название и идентификатор — отдельными метками.
	auto* glyphLabel = view->findChild<QLabel*>(QStringLiteral("SystemRouterNodeViewGlyph"));
	QVERIFY(glyphLabel);
	QCOMPARE(glyphLabel->text(), QStringLiteral("⚙"));

	auto* nameLabel = view->findChild<QLabel*>(QStringLiteral("SystemRouterNodeViewNodeName"));
	QVERIFY(nameLabel);
	QCOMPARE(nameLabel->text(), QStringLiteral("Узел"));

	auto* idLabel = view->findChild<QLabel*>(QStringLiteral("SystemRouterNodeViewNodeId"));
	QVERIFY(idLabel);
	QVERIFY(idLabel->text().contains(item->id().toString()));

	// Число показывается редактором значения.
	auto* countEditor = findEditorByProperty<QLineEdit>(view, QStringLiteral("count"));
	QVERIFY(countEditor);
	QCOMPARE(countEditor->text(), QStringLiteral("3"));

	// Для ссылки на команду показывается текст скрипта, а не GUID блока.
	auto* commandEditor = findEditorByProperty<QPlainTextEdit>(view, QStringLiteral("script"));
	QVERIFY(commandEditor);
	QCOMPARE(commandEditor->toPlainText(), QStringLiteral("authorize url={{url}}"));
	QVERIFY(!commandEditor->toPlainText().contains(kCommandId));
	QVERIFY(commandEditor->toolTip().contains(QStringLiteral("Авторизация")));

	// Секретный параметр виден, но значение скрыто.
	auto* secretLabel = findEditorByProperty<QLabel>(view, QStringLiteral("token"));
	QVERIFY(secretLabel);
	QVERIFY(secretLabel->text().contains(QStringLiteral("скрыто")));

	// Скрытый параметр и глиф в просмотр не попадают.
	QVERIFY(findEditorByProperty<QWidget>(view, QStringLiteral("internal")) == nullptr);
	QVERIFY(findEditorByProperty<QWidget>(view, LazyTreeNodeGlyph::propertyName()) == nullptr);
}

void TestSystemRouter::nodeViewCopyButtonsCopyNameAndValue() {
	auto item = _service->createItem(QVariant(), QStringLiteral("Узел"));
	QVERIFY(item);
	QVERIFY(item->setProperty(QStringLiteral("title"), QStringLiteral("Текст")));
	QVERIFY(item->setPropertyMimeTypes(QStringLiteral("title"),
		{ QStringLiteral("text/plain") }));
	QVERIFY(item->setProperty(QStringLiteral("token"), QStringLiteral("секрет")));
	QVERIFY(item->setPropertyMimeTypes(QStringLiteral("token"),
		{ QString::fromLatin1(SystemRouterMime::kSecret), QStringLiteral("text/plain") }));
	QVERIFY(item->setProperty(QStringLiteral("script"), kCommandId));
	QVERIFY(item->setPropertyMimeTypes(QStringLiteral("script"),
		{ QString::fromLatin1(SystemRouterMime::kCommand) }));
	QVERIFY(_service->updateItem(item));

	MockCommandsProvider commandProvider;
	commandProvider.stored.append(SystemRouterCommand{ kCommandId, 2,
		QStringLiteral("Авторизация"), QStringLiteral("authorize url={{url}}") });
	SystemRouterCommandService commandService(&commandProvider);

	SystemRouterNodeViewDocument document(_service.get(), &commandService, item->id());
	auto* view = document.view();
	QVERIFY(view);

	// Сигнал виджета — контракт копирования: системный буфер обмена может быть
	// недоступен в среде тестов, а подготовленный текст — всегда.
	QSignalSpy spy(view, &SystemRouterNodeViewWidget::copied);

	const auto findCopyButton = [view](const QString& objectName, const QString& name) -> QToolButton* {
		for (auto* button : view->findChildren<QToolButton*>(objectName)) {
			if (button->property("propertyId").toString() == name) {
				return button;
			}
		}
		return nullptr;
	};

	auto* copyName = findCopyButton(QStringLiteral("SystemRouterNodeViewCopyName"),
		QStringLiteral("title"));
	QVERIFY(copyName);
	QVERIFY(copyName->isEnabled());
	copyName->click();
	QCOMPARE(spy.count(), 1);
	QCOMPARE(spy.at(0).at(0).toString(), QStringLiteral("title"));

	auto* copyValue = findCopyButton(QStringLiteral("SystemRouterNodeViewCopyValue"),
		QStringLiteral("title"));
	QVERIFY(copyValue);
	QVERIFY(copyValue->isEnabled());
	copyValue->click();
	QCOMPARE(spy.count(), 2);
	QCOMPARE(spy.at(1).at(0).toString(), QStringLiteral("Текст"));

	// Значение секретного параметра скопировать нельзя.
	auto* secretCopy = findCopyButton(QStringLiteral("SystemRouterNodeViewCopyValue"),
		QStringLiteral("token"));
	QVERIFY(secretCopy);
	QVERIFY(!secretCopy->isEnabled());
	secretCopy->click();
	QCOMPARE(spy.count(), 2);

	// Для параметра-команды копируется текст скрипта, а не GUID блока.
	auto* scriptCopy = findCopyButton(QStringLiteral("SystemRouterNodeViewCopyValue"),
		QStringLiteral("script"));
	QVERIFY(scriptCopy);
	QVERIFY(scriptCopy->isEnabled());
	scriptCopy->click();
	QCOMPARE(spy.count(), 3);
	QCOMPARE(spy.at(2).at(0).toString(), QStringLiteral("authorize url={{url}}"));
}

void TestSystemRouter::nodeViewKeepsRowsAtTopAndScrolls() {
	auto item = _service->createItem(QVariant(), QStringLiteral("Узел"));
	QVERIFY(item);
	for (int i = 0; i < 40; i++) {
		const auto name = QStringLiteral("param%1").arg(i, 2, 10, QLatin1Char('0'));
		QVERIFY(item->setProperty(name, i));
		QVERIFY(item->setPropertyMimeTypes(name,
			{ QStringLiteral("application/x-integer") }));
	}
	QVERIFY(_service->updateItem(item));

	SystemRouterNodeViewDocument document(_service.get(), nullptr, item->id());
	document.resize(420, 200);
	document.show();
	QApplication::processEvents();

	auto* content = document.findChild<QWidget*>(
		QStringLiteral("SystemRouterNodeViewContent"));
	QVERIFY(content);
	auto* contentLayout = qobject_cast<QVBoxLayout*>(content->layout());
	QVERIFY(contentLayout);

	// Свободное место по вертикали забирает распорка в конце списка:
	// блоки параметров остаются компактными у верхнего края.
	const auto lastItem = contentLayout->itemAt(contentLayout->count() - 1);
	QVERIFY(lastItem);
	QVERIFY(lastItem->spacerItem() != nullptr);

	auto* scroll = document.findChild<QScrollArea*>(
		QStringLiteral("SystemRouterNodeViewScroll"));
	QVERIFY(scroll);
	QVERIFY(scroll->verticalScrollBar()->maximum() > 0);

	// Первый блок прижат к верхнему левому краю области просмотра,
	// а не центрируется по её высоте.
	QLabel* firstName = nullptr;
	for (auto* label : content->findChildren<QLabel*>(
		QStringLiteral("SystemRouterNodeViewParameterName"))) {
		if (label->property("propertyName").toString() == QStringLiteral("param00")) {
			firstName = label;
			break;
		}
	}
	QVERIFY(firstName);
	const auto topLeft = firstName->mapTo(scroll->viewport(), QPoint(0, 0));
	QVERIFY2(topLeft.y() >= 0 && topLeft.y() < 10,
		qPrintable(QStringLiteral("первый параметр не у верхнего края: y=%1")
			.arg(topLeft.y())));
}

void TestSystemRouter::nodeViewRefreshesOnNodeChange() {
	auto item = _service->createItem(QVariant(), QStringLiteral("Узел"));
	QVERIFY(item);
	QVERIFY(item->setProperty(QStringLiteral("title"), QStringLiteral("Текст")));
	QVERIFY(item->setPropertyMimeTypes(QStringLiteral("title"),
		{ QStringLiteral("text/plain") }));
	QVERIFY(_service->updateItem(item));

	SystemRouterNodeViewDocument document(_service.get(), nullptr, item->id());
	auto* view = document.view();
	QVERIFY(view);
	QCOMPARE(view->propertyNames(), QStringList({ QStringLiteral("title") }));

	QVERIFY(_service->renameItem(item->id(), QStringLiteral("Новое имя")));
	QCOMPARE(document.title(), QStringLiteral("Просмотр: Новое имя"));

	QVERIFY(item->setProperty(QStringLiteral("count"), 5));
	QVERIFY(item->setPropertyMimeTypes(QStringLiteral("count"),
		{ QStringLiteral("application/x-integer") }));
	QVERIFY(_service->updateItem(item));
	QCOMPARE(view->propertyNames(), QStringList({ QStringLiteral("count"),
		QStringLiteral("title") }));
}

void TestSystemRouter::scriptDialogOffersView() {
	MockCommandsProvider provider;
	provider.stored.append(SystemRouterCommand{ kCommandId, 1,
		QStringLiteral("Описание"), QStringLiteral("run") });

	auto item = _service->createItem(QVariant(), QStringLiteral("Узел"));
	QVERIFY(item);
	QVERIFY(item->setProperty(QStringLiteral("script"), kCommandId));
	QVERIFY(item->setPropertyMimeTypes(QStringLiteral("script"),
		{ QString::fromLatin1(SystemRouterMime::kCommand) }));

	SystemRouterCommandService commandService(&provider);
	const auto previews = commandService.previewsForNode(item);
	QCOMPARE(previews.size(), 1);

	SystemRouterScriptDialog dialog(previews);

	auto* startButton = dialog.findChild<QPushButton*>(
		QStringLiteral("SystemRouterScriptStartButton"));
	QVERIFY(startButton);
	QVERIFY(startButton->isEnabled());
	QVERIFY(startButton->isDefault());

	auto* viewButton = dialog.findChild<QPushButton*>(
		QStringLiteral("SystemRouterScriptViewButton"));
	QVERIFY(viewButton);
	QVERIFY(viewButton->isEnabled());

	auto* cancelButton = dialog.findChild<QPushButton*>(
		QStringLiteral("SystemRouterScriptCancelButton"));
	QVERIFY(cancelButton);

	dialog.show();
	QApplication::processEvents();

	// «Запуск» и «Просмотр» — слева, «Отмена» — у правого края.
	QVERIFY(startButton->x() < viewButton->x());
	QVERIFY(viewButton->x() < cancelButton->x());
	QCOMPARE(startButton->y(), viewButton->y());
	QCOMPARE(viewButton->y(), cancelButton->y());
	QVERIFY(startButton->x() < dialog.width() / 2);
	QVERIFY(cancelButton->geometry().right() > dialog.width() * 3 / 4);

	viewButton->click();
	QCOMPARE(dialog.result(), SystemRouterScriptDialog::ViewResult);
}

void TestSystemRouter::commandServiceCrud() {
	MockCommandsProvider provider;
	SystemRouterCommandService service(&provider);

	SystemRouterCommand command;
	command.id = kCommandId;
	command.version = 2;
	command.description = QStringLiteral("Авторизация");
	command.script = QStringLiteral("authorize url={{first-url}}");
	QVERIFY(service.saveCommand(command));
	QCOMPARE(provider.stored.size(), 1);

	const auto found = service.command(kCommandId);
	QVERIFY(found.has_value());
	QCOMPARE(found->version, 2);
	QCOMPARE(found->description, QStringLiteral("Авторизация"));

	command.version = 1;
	command.script = QStringLiteral("authorize url={{first-url}}, token={{token}}");
	QVERIFY(service.saveCommand(command));
	QCOMPARE(provider.stored.size(), 1);

	QVERIFY(service.removeCommand(kCommandId));
	QVERIFY(provider.stored.isEmpty());

	SystemRouterCommand invalid;
	invalid.id = QStringLiteral("not-a-guid");
	QVERIFY(!service.saveCommand(invalid));
	QVERIFY(!service.lastError().isEmpty());
}

void TestSystemRouter::rendersScriptParameters() {
	auto item = _service->createItem(QVariant(), QStringLiteral("Узел"));
	QVERIFY(item);

	item->setProperty(QStringLiteral("first-url"), QStringLiteral("https://www.google.com"));
	item->setPropertyMimeTypes(QStringLiteral("first-url"),
		{ QString::fromLatin1(SystemRouterMime::kCommandParameter), QStringLiteral("text/plain") });
	item->setProperty(QStringLiteral("first-token"), QStringLiteral("ADFRTKEERMG4345"));
	item->setPropertyMimeTypes(QStringLiteral("first-token"),
		{ QString::fromLatin1(SystemRouterMime::kCommandParameter), QStringLiteral("text/plain") });
	item->setProperty(QStringLiteral("payload"), QStringLiteral(R"({"a":1})"));
	item->setPropertyMimeTypes(QStringLiteral("payload"),
		{ QString::fromLatin1(SystemRouterMime::kCommandParameter), QStringLiteral("application/json") });
	item->setProperty(QStringLiteral("count"), 5);
	item->setPropertyMimeTypes(QStringLiteral("count"),
		{ QString::fromLatin1(SystemRouterMime::kCommandParameter), QStringLiteral("application/x-integer") });
	item->setProperty(QStringLiteral("enabled"), true);
	item->setPropertyMimeTypes(QStringLiteral("enabled"),
		{ QString::fromLatin1(SystemRouterMime::kCommandParameter), QStringLiteral("application/x-boolean") });
	item->setProperty(QStringLiteral("text"), QStringLiteral("line1\nline2"));
	item->setPropertyMimeTypes(QStringLiteral("text"),
		{ QString::fromLatin1(SystemRouterMime::kCommandParameter), QStringLiteral("text/x-multiline") });
	item->setProperty(QStringLiteral("script"), kCommandId);
	item->setPropertyMimeTypes(QStringLiteral("script"),
		{ QString::fromLatin1(SystemRouterMime::kCommand) });

	MockCommandsProvider provider;
	SystemRouterCommand command;
	command.id = kCommandId;
	command.version = 1;
	command.description = QStringLiteral("Проверка параметров");
	command.script = QStringLiteral(
		"authorize url={{first-url}}, token={{first-token}}, "
		"payload={{payload}}, count={{count}}, enabled={{enabled}}, "
		"text={{text}}, missing={{missing}}");
	provider.stored.append(command);

	SystemRouterCommandService commandService(&provider);
	const auto previews = commandService.previewsForNode(item);
	QCOMPARE(previews.size(), 1);

	const auto& preview = previews.first();
	QVERIFY(preview.commandFound);
	QCOMPARE(preview.propertyName, QStringLiteral("script"));
	QCOMPARE(preview.commandId, kCommandId);
	QVERIFY(preview.missingParameters.contains(QStringLiteral("missing")));
	QCOMPARE(preview.renderedScript,
		QStringLiteral("authorize url=\"https://www.google.com\", "
			"token=\"ADFRTKEERMG4345\", payload=<<json>>{\"a\":1}<</json>>, "
			"count=5, enabled=true, text=<<text>>line1\nline2<</text>>, "
			"missing=<<undefined>>"));
}

void TestSystemRouter::previewsForNodeFindsCommands() {
	auto item = _service->createItem(QVariant(), QStringLiteral("Узел"));
	QVERIFY(item);
	item->setProperty(QStringLiteral("first"), kCommandId);
	item->setPropertyMimeTypes(QStringLiteral("first"),
		{ QString::fromLatin1(SystemRouterMime::kCommand) });
	item->setProperty(QStringLiteral("second"), QString::fromLatin1("{22222222-2222-2222-2222-222222222222}"));
	item->setPropertyMimeTypes(QStringLiteral("second"),
		{ QStringLiteral("text/plain"), QString::fromLatin1(SystemRouterMime::kCommand) });

	MockCommandsProvider provider;
	provider.stored.append(SystemRouterCommand{ kCommandId, 1,
		QStringLiteral("Первая"), QStringLiteral("first") });
	provider.stored.append(SystemRouterCommand{ kSecondCommandId, 3,
		QStringLiteral("Вторая"), QStringLiteral("second") });

	SystemRouterCommandService commandService(&provider);
	const auto previews = commandService.previewsForNode(item);
	QCOMPARE(previews.size(), 2);
	QCOMPARE(previews.at(0).commandId, kCommandId);
	QVERIFY(previews.at(0).commandFound);
	QCOMPARE(previews.at(0).renderedScript, QStringLiteral("first"));
	QCOMPARE(previews.at(1).commandId, kSecondCommandId);
	QVERIFY(previews.at(1).commandFound);
	QCOMPARE(previews.at(1).command.version, 3);
}

void TestSystemRouter::scriptDialogShowsPreview() {
	MockCommandsProvider provider;
	SystemRouterCommand command;
	command.id = kCommandId;
	command.version = 7;
	command.description = QStringLiteral("Описание скрипта");
	command.script = QStringLiteral("run {{missing}}");
	provider.stored.append(command);

	auto item = _service->createItem(QVariant(), QStringLiteral("Узел"));
	QVERIFY(item);
	item->setProperty(QStringLiteral("script"), kCommandId);
	item->setPropertyMimeTypes(QStringLiteral("script"),
		{ QString::fromLatin1(SystemRouterMime::kCommand) });

	SystemRouterCommandService commandService(&provider);
	const auto previews = commandService.previewsForNode(item);
	QCOMPARE(previews.size(), 1);

	SystemRouterScriptDialog dialog(previews);
	auto* list = dialog.findChild<QListWidget*>(QStringLiteral("SystemRouterScriptList"));
	QVERIFY(list);
	QCOMPARE(list->count(), 1);
	QCOMPARE(list->item(0)->text(), QStringLiteral("script"));

	auto* code = dialog.findChild<QPlainTextEdit*>(QStringLiteral("SystemRouterScriptCode"));
	QVERIFY(code);
	QCOMPARE(code->toPlainText(), QStringLiteral("run <<undefined>>"));
	QCOMPARE(dialog.selectedPreview().command.version, 7);
}

QTEST_MAIN(TestSystemRouter)
#include "test_system_router.moc"
