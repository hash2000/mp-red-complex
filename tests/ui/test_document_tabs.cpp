#include "Libs/BaseWidgets/properties/properties_list_widget.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/command_processor.h"
#include "Libs/Engine/scripting/system_options/system_option.h"
#include "Libs/Engine/scripting/system_options/system_options_registry.h"
#include "Libs/Engine/scripting/widgets/widget_registry.h"
#include "Libs/Engine/services/services_registry.h"
#include "Libs/Engine/ui/doc_tab_panel.h"
#include "Libs/Engine/ui/doc_tab_panel/document_tabs_order.h"
#include "Libs/Engine/ui/doc_widget.h"
#include "Libs/Engine/ui/main_window.h"
#include "Libs/Engine/ui/main_window/system_options_document.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QMouseEvent>
#include <QtTest/QtTest>

#include <algorithm>
#include <memory>

namespace {

class TestCommandController : public CommandController {
public:
	TestCommandController() : CommandController(nullptr, nullptr) {}
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

/// Вкладки панели в порядке разметки (сверху вниз).
QList<QWidget*> tabItems(const DocumentTabsPanel& panel) {
	QList<QWidget*> items;
	for (auto* widget : panel.findChildren<QWidget*>(QStringLiteral("DocumentTabItem"))) {
		if (!widget->isHidden()) {
			items << widget;
		}
	}

	std::sort(items.begin(), items.end(), [](QWidget* left, QWidget* right) {
		return left->mapToGlobal(QPoint(0, 0)).y() < right->mapToGlobal(QPoint(0, 0)).y();
	});
	return items;
}

/// Перетаскивание вкладки мышью: нажатие, движение выше цели, отпускание.
void dragTab(QWidget* item, const QPoint& targetGlobal) {
	const auto localCenter = item->rect().center();
	const auto pressGlobal = item->mapToGlobal(localCenter);

	QMouseEvent press(QEvent::MouseButtonPress, localCenter, pressGlobal,
		Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
	QCoreApplication::sendEvent(item, &press);

	QMouseEvent move(QEvent::MouseMove, localCenter, targetGlobal,
		Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
	QCoreApplication::sendEvent(item, &move);

	QMouseEvent release(QEvent::MouseButtonRelease, localCenter, targetGlobal,
		Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
	QCoreApplication::sendEvent(item, &release);
}

} // namespace

class TestDocumentTabs : public QObject {
	Q_OBJECT

private slots:
	void orderStrategiesAreRegistered();
	void systemOptionDescribesStrategies();
	void manualStrategyKeepsOrder();
	void colorStrategyGroupsByColor();
	void xtypeStrategyGroupsByType();
	void panelKeepsManualOrder();
	void panelMovesDocument();
	void panelGroupsByColor();
	void panelGroupsByType();
	void panelFollowsSystemOption();
	void panelFallsBackOnUnknownOption();
	void dragReordersManualTabs();
	void dragIsDisabledWhenGrouped();
	void documentStoresTabColorAndType();
	void documentColorIsStoredInContext();
	void systemOptionsDocumentUsesPrivilegedContext();
	void systemOptionsDocumentEditsGlobalSettings();
};

void TestDocumentTabs::orderStrategiesAreRegistered() {
	DocumentTabsOrderRegistry registry;

	QCOMPARE(registry.ids(), QStringList({ "manual", "color", "xtype" }));
	QCOMPARE(registry.defaultId(), QStringLiteral("manual"));
	QCOMPARE(registry.strategies().size(), 3);

	auto* manual = registry.strategy("manual");
	QVERIFY(manual);
	QVERIFY(manual->supportsManualMove());

	// Группировка не поддерживает ручную сортировку: порядок задаёт правило.
	for (const auto& id : { QStringLiteral("color"), QStringLiteral("xtype") }) {
		auto* strategy = registry.strategy(id);
		QVERIFY(strategy);
		QVERIFY(!strategy->supportsManualMove());
		QVERIFY(!strategy->title().isEmpty());
	}

	QVERIFY(!registry.strategy("missing"));
}

void TestDocumentTabs::systemOptionDescribesStrategies() {
	// Встроенный системный параметр собирается из реестра правил.
	SystemOptionsRegistry registry;
	registerDocumentTabsOrderOption(registry);

	const auto option = registry.option(SystemOptions::kDocumentsTabsOrder);
	QVERIFY(option);
	QCOMPARE(option->type, SystemOptionType::Enum);
	QCOMPARE(option->values, QStringList({ "manual", "color", "xtype" }));
	QCOMPARE(option->valueTitles.size(), 3);
	QCOMPARE(option->defaultValue.toString(), QStringLiteral("manual"));
}

void TestDocumentTabs::manualStrategyKeepsOrder() {
	DocumentWidget first;
	DocumentWidget second;
	DocumentWidget third;

	DocumentTabsOrderRegistry registry;
	const auto groups = registry.strategy("manual")->arrange({ &first, &second, &third });

	QCOMPARE(groups.size(), 1);
	QVERIFY(groups.first().title.isEmpty());
	QCOMPARE(groups.first().documents, QList<DocumentWidget*>({ &first, &second, &third }));
}

void TestDocumentTabs::colorStrategyGroupsByColor() {
	DocumentWidget red;
	red.setColor(QColor("#ff0000"));
	DocumentWidget blue;
	blue.setColor(QColor("#0000ff"));
	DocumentWidget plain;
	DocumentWidget anotherRed;
	anotherRed.setColor(QColor("#ff0000"));

	DocumentTabsOrderRegistry registry;
	const auto groups = registry.strategy("color")->arrange({ &red, &plain, &blue, &anotherRed });

	// Цветные группы идут по имени цвета, «Без цвета» — последняя.
	QCOMPARE(groups.size(), 3);
	QCOMPARE(groups.at(0).title, QString("#0000ff"));
	QCOMPARE(groups.at(0).documents, QList<DocumentWidget*>({ &blue }));
	QCOMPARE(groups.at(1).title, QString("#ff0000"));
	QCOMPARE(groups.at(1).documents, QList<DocumentWidget*>({ &red, &anotherRed }));
	QCOMPARE(groups.at(2).title, QStringLiteral("Без цвета"));
	QCOMPARE(groups.at(2).documents, QList<DocumentWidget*>({ &plain }));
}

void TestDocumentTabs::xtypeStrategyGroupsByType() {
	DocumentWidget editor;
	editor.setXtype("code-editor");
	DocumentWidget router;
	router.setXtype("system-router");
	DocumentWidget untyped;
	DocumentWidget secondEditor;
	secondEditor.setXtype("code-editor");

	DocumentTabsOrderRegistry registry;
	const auto groups = registry.strategy("xtype")->arrange({ &router, &untyped, &editor, &secondEditor });

	QCOMPARE(groups.size(), 3);
	QCOMPARE(groups.at(0).title, QString("code-editor"));
	QCOMPARE(groups.at(0).documents, QList<DocumentWidget*>({ &editor, &secondEditor }));
	QCOMPARE(groups.at(1).title, QString("system-router"));
	QCOMPARE(groups.at(1).documents, QList<DocumentWidget*>({ &router }));
	QCOMPARE(groups.at(2).title, QStringLiteral("Без типа"));
	QCOMPARE(groups.at(2).documents, QList<DocumentWidget*>({ &untyped }));
}

void TestDocumentTabs::panelKeepsManualOrder() {
	DocumentTabsPanel panel;
	DocumentWidget first;
	first.setTitle("Первый");
	DocumentWidget second;
	second.setTitle("Второй");

	panel.addDocument(&first);
	panel.addDocument(&second);

	QCOMPARE(panel.documents(), QList<DocumentWidget*>({ &first, &second }));

	// Повторное добавление ничего не меняет.
	panel.addDocument(&first);
	QCOMPARE(panel.documents().size(), 2);

	// Удаление убирает документ из порядка.
	panel.removeDocument(&first);
	QCOMPARE(panel.documents(), QList<DocumentWidget*>({ &second }));
}

void TestDocumentTabs::panelMovesDocument() {
	DocumentTabsPanel panel;
	DocumentWidget first;
	DocumentWidget second;
	DocumentWidget third;
	panel.addDocument(&first);
	panel.addDocument(&second);
	panel.addDocument(&third);

	QVERIFY(panel.moveDocument(&third, 0));
	QCOMPARE(panel.documents(), QList<DocumentWidget*>({ &third, &first, &second }));

	// Позиция ограничивается размером списка.
	QVERIFY(panel.moveDocument(&third, 99));
	QCOMPARE(panel.documents(), QList<DocumentWidget*>({ &first, &second, &third }));
	QVERIFY(!panel.moveDocument(nullptr, 0));
}

void TestDocumentTabs::panelGroupsByColor() {
	DocumentTabsPanel panel;
	panel.resize(220, 400);
	DocumentWidget red;
	red.setColor(QColor("#ff0000"));
	DocumentWidget plain;
	panel.addDocument(&red);
	panel.addDocument(&plain);
	panel.show();
	QCoreApplication::processEvents();

	// Ручная сортировка — группы не создаются.
	QVERIFY(panel.groupTitles().isEmpty());

	// Цвет вкладки действительно рисуется полосой слева от значка.
	const auto items = tabItems(panel);
	QCOMPARE(items.size(), 2);
	const auto image = items.at(0)->grab().toImage();
	bool hasColorBar = false;
	for (int y = 4; y < image.height() - 4 && !hasColorBar; y++) {
		for (int x = 2; x < 8; x++) {
			if (image.pixelColor(x, y) == QColor("#ff0000")) {
				hasColorBar = true;
				break;
			}
		}
	}
	QVERIFY(hasColorBar);

	QVERIFY(panel.setOrderStrategy("color"));
	QCOMPARE(panel.orderStrategy(), QString("color"));
	QCOMPARE(panel.groupTitles(), QStringList({ "#ff0000", "Без цвета" }));

	// Смена цвета на лету перестраивает группы.
	plain.setColor(QColor("#00ff00"));
	QCOMPARE(panel.groupTitles().size(), 2);
	QVERIFY(panel.groupTitles().contains(QString("#00ff00")));

	// Неизвестное правило не включается.
	QVERIFY(!panel.setOrderStrategy("missing"));
	QCOMPARE(panel.orderStrategy(), QString("color"));
}

void TestDocumentTabs::panelGroupsByType() {
	DocumentTabsPanel panel;
	DocumentWidget editor;
	editor.setXtype("code-editor");
	DocumentWidget router;
	router.setXtype("system-router");
	DocumentWidget untyped;
	panel.addDocument(&editor);
	panel.addDocument(&router);
	panel.addDocument(&untyped);

	QVERIFY(panel.setOrderStrategy("xtype"));
	QCOMPARE(panel.groupTitles(), QStringList({ "code-editor", "system-router", "Без типа" }));
}

void TestDocumentTabs::panelFollowsSystemOption() {
	// Given: панель читает правило из глобального контекста (системный параметр)
	TestCommandController controller;
	controller.initSystemContexts();
	QCOMPARE(controller.globalContext()->variable(SystemOptions::kDocumentsTabsOrder).toString(),
		QString("manual"));

	DocumentTabsPanel panel(controller.globalContext());
	QCOMPARE(panel.orderStrategy(), QString("manual"));

	// When: настройка изменена в глобальном контексте
	QVERIFY(controller.globalContext()->setVariable(SystemOptions::kDocumentsTabsOrder, "xtype"));

	// Then: панель переключила правило
	QCOMPARE(panel.orderStrategy(), QString("xtype"));
}

void TestDocumentTabs::panelFallsBackOnUnknownOption() {
	TestCommandController controller;
	controller.initSystemContexts();
	DocumentTabsPanel panel(controller.globalContext());

	QVERIFY(controller.globalContext()->setVariable(SystemOptions::kDocumentsTabsOrder, "unknown"));

	// Неизвестное значение настройки не ломает панель: работает правило по умолчанию.
	QCOMPARE(panel.orderStrategy(), QString("manual"));
}

void TestDocumentTabs::dragReordersManualTabs() {
	DocumentTabsPanel panel;
	panel.resize(220, 400);
	DocumentWidget first;
	first.setTitle("Первый");
	DocumentWidget second;
	second.setTitle("Второй");
	panel.addDocument(&first);
	panel.addDocument(&second);
	panel.show();
	QCoreApplication::processEvents();

	const auto items = tabItems(panel);
	QCOMPARE(items.size(), 2);

	// Перетаскиваем нижнюю вкладку выше верхней.
	const auto target = items.at(0)->mapToGlobal(QPoint(0, 0));
	dragTab(items.at(1), target);

	QCOMPARE(panel.documents(), QList<DocumentWidget*>({ &second, &first }));
}

void TestDocumentTabs::dragIsDisabledWhenGrouped() {
	DocumentTabsPanel panel;
	panel.resize(220, 400);
	DocumentWidget red;
	red.setColor(QColor("#ff0000"));
	DocumentWidget plain;
	panel.addDocument(&red);
	panel.addDocument(&plain);
	QVERIFY(panel.setOrderStrategy("color"));
	panel.show();
	QCoreApplication::processEvents();

	const auto items = tabItems(panel);
	QVERIFY(items.size() >= 2);

	// В режиме группировки порядок задаёт правило, перетаскивание отключено.
	const auto order = panel.documents();
	dragTab(items.at(1), items.at(0)->mapToGlobal(QPoint(0, 0)));
	QCOMPARE(panel.documents(), order);
}

void TestDocumentTabs::documentStoresTabColorAndType() {
	DocumentWidget document;

	QSignalSpy colorChanged(&document, &DocumentWidget::colorChanged);
	QSignalSpy xtypeChanged(&document, &DocumentWidget::xtypeChanged);

	QVERIFY(!document.color().isValid());
	document.setColor(QColor("#ff8800"));
	QCOMPARE(document.color(), QColor("#ff8800"));
	QCOMPARE(colorChanged.count(), 1);

	document.setXtype("code-editor");
	QCOMPARE(document.xtype(), QString("code-editor"));
	QCOMPARE(xtypeChanged.count(), 1);
	document.setXtype("code-editor");
	QCOMPARE(xtypeChanged.count(), 1);

	// Сброс цвета возвращает «цвет не задан».
	document.setColor(QColor());
	QVERIFY(!document.color().isValid());
	QCOMPARE(colorChanged.count(), 2);
}

void TestDocumentTabs::documentColorIsStoredInContext() {
	TestCommandController controller;
	auto context = controller.globalContext()->createDocumentContext("colored");

	DocumentWidget document;
	document.setContext(context);

	// Состояние вкладки живёт в переменных контекста документа.
	document.setColor(QColor("#123456"));
	document.setXtype("test-widget");
	QCOMPARE(context->variable("color").value<QColor>(), QColor("#123456"));
	QCOMPARE(context->variable("xtype").toString(), QString("test-widget"));

	// Изменение переменной извне обновляет документ.
	context->setVariable("color", QColor("#654321"));
	QCOMPARE(document.color(), QColor("#654321"));

	// Сброс цвета удаляет переменную из контекста.
	document.setColor(QColor());
	QVERIFY(!context->ownVariables().contains("color"));
}

void TestDocumentTabs::systemOptionsDocumentUsesPrivilegedContext() {
	TestCommandController controller;
	controller.initSystemContexts();
	controller.widgets()->registerFactory({ SystemOptionsDocument::className(),
		SystemOptionsDocument::xtype(), /*singletone*/ true, QStringLiteral("⚙"),
		QStringLiteral("Системные настройки"), DocumentRule::SystemOptions },
		[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			return new SystemOptionsDocument(handler.parent);
		});

	MainWindow window(controller.globalContext(), nullptr, false);
	auto* widget = window.createWidget(SystemOptionsDocument::xtype(),
		MainWindow::WidgetKind::Document, QStringLiteral("Системные настройки"), QStringLiteral("⚙"),
		[&controller] {
			return controller.widgets()->build(SystemOptionsDocument::xtype(), WidgetBuildHandler());
		},
		QString(), SystemOptionsDocument::xtype(), controller.globalContext());
	QVERIFY(widget);

	auto* document = window.document(SystemOptionsDocument::xtype());
	QVERIFY(document);
	QVERIFY(qobject_cast<SystemOptionsDocument*>(document));

	// Роль документа: контекст `.options` в режиме Mutable.
	const auto context = window.documentContext(SystemOptionsDocument::xtype());
	QVERIFY(context);
	QCOMPARE(context->nameSpace(), QString(".options"));
	QCOMPARE(context->variablesMode(), CommandContext::VariablesMode::Mutable);
}

void TestDocumentTabs::systemOptionsDocumentEditsGlobalSettings() {
	TestCommandController controller;
	controller.initSystemContexts();

	// Редактор строится по реестру системных параметров.
	SystemOptionsDocument optionsDocument;
	optionsDocument.setContext(controller.globalContext()->createDocumentContext(
		SystemOptionsDocument::xtype(), DocumentRule::SystemOptions));

	QWidget* editor = optionsDocument.createPropertiesEditor(nullptr);
	QVERIFY(editor);
	auto* properties = editor->findChild<PropertiesListWidget*>(
		QStringLiteral("SystemOptionsProperties"));
	QVERIFY(properties);
	QCOMPARE(properties->propertyCount(), 1);

	auto* combo = editor->findChild<QComboBox*>();
	QVERIFY(combo);
	QCOMPARE(combo->count(), 3);
	QCOMPARE(combo->currentText(), QStringLiteral("Ручная сортировка"));

	// Выбор в редакторе меняет системный параметр глобального контекста.
	combo->setCurrentIndex(2);
	QCOMPARE(controller.globalContext()->variable(SystemOptions::kDocumentsTabsOrder).toString(),
		QString("xtype"));

	delete editor;
}

QTEST_MAIN(TestDocumentTabs)
#include "test_document_tabs.moc"
