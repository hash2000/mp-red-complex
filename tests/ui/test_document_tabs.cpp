#include "Libs/BaseWidgets/properties/properties_list_widget.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/command_processor.h"
#include "Libs/Engine/scripting/system_options/system_option.h"
#include "Libs/Engine/scripting/system_options/system_options_registry.h"
#include "Libs/Engine/scripting/widgets/widget_registry.h"
#include "Libs/Engine/services/services_registry.h"
#include "Libs/Engine/themes/document_tab_theme.h"
#include "Libs/Engine/ui/doc_tab_panel.h"
#include "Libs/Engine/ui/doc_tab_panel/document_tabs_order.h"
#include "Libs/Engine/ui/doc_widget.h"
#include "Libs/Engine/ui/main_window.h"
#include "Libs/Engine/ui/main_window/system_options_document.h"
#include "Libs/Resources/resources.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QMouseEvent>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <algorithm>
#include <memory>

namespace {

class TestCommandController : public CommandController {
public:
	TestCommandController() : CommandController(nullptr, nullptr) {}
	explicit TestCommandController(Resources* resources) : CommandController(resources, nullptr) {}
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

/// Вкладка панели с цветом из темы.
DocumentTabView tabOf(DocumentWidget* document, const QColor& themeColor = QColor()) {
	DocumentTabView tab;
	tab.document = document;
	tab.themeColor = themeColor;
	return tab;
}

QList<DocumentWidget*> documentsOf(const QList<DocumentTabView>& tabs) {
	QList<DocumentWidget*> result;
	for (const auto& tab : tabs) {
		result << tab.document;
	}

	return result;
}

bool writeTextFile(const QString& path, const QByteArray& data) {
	QFile file(path);
	return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
}

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

/// Есть ли в снимке вкладки полоса указанного цвета (слева от значка).
bool hasColorBar(QWidget* item, const QColor& color) {
	const auto image = item->grab().toImage();
	for (int y = 4; y < image.height() - 4; y++) {
		for (int x = 2; x < 8; x++) {
			if (image.pixelColor(x, y) == color) {
				return true;
			}
		}
	}

	return false;
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
	void colorStrategyUsesThemeColor();
	void xtypeStrategyGroupsByType();
	void documentTabThemeParsesColors();
	void documentTabThemeIgnoresMissingFile();
	void documentTabThemeFromConfigurations();
	void panelKeepsManualOrder();
	void panelTabToolTipFromContextVariable();
	void panelMovesDocument();
	void panelGroupsByColor();
	void panelGroupsByType();
	void panelUsesThemeColors();
	void panelUsesThemeGlyph();
	void panelLoadsThemeFromResources();
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
	const auto groups = registry.strategy("manual")->arrange(
		{ tabOf(&first), tabOf(&second), tabOf(&third) });

	QCOMPARE(groups.size(), 1);
	QVERIFY(groups.first().title.isEmpty());
	QCOMPARE(documentsOf(groups.first().tabs),
		QList<DocumentWidget*>({ &first, &second, &third }));
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
	const auto groups = registry.strategy("color")->arrange(
		{ tabOf(&red), tabOf(&plain), tabOf(&blue), tabOf(&anotherRed) });

	// Цветные группы идут по имени цвета, «Без цвета» — последняя.
	QCOMPARE(groups.size(), 3);
	QCOMPARE(groups.at(0).title, QString("#0000ff"));
	QCOMPARE(documentsOf(groups.at(0).tabs), QList<DocumentWidget*>({ &blue }));
	QCOMPARE(groups.at(1).title, QString("#ff0000"));
	QCOMPARE(documentsOf(groups.at(1).tabs), QList<DocumentWidget*>({ &red, &anotherRed }));
	QCOMPARE(groups.at(2).title, QStringLiteral("Без цвета"));
	QCOMPARE(documentsOf(groups.at(2).tabs), QList<DocumentWidget*>({ &plain }));
}

void TestDocumentTabs::colorStrategyUsesThemeColor() {
	// Цвет из темы участвует в группировке так же, как явный цвет документа.
	DocumentWidget themed;
	DocumentWidget explicitColor;
	explicitColor.setColor(QColor("#ff0000"));
	DocumentWidget plain;

	DocumentTabsOrderRegistry registry;
	const auto groups = registry.strategy("color")->arrange({
		tabOf(&themed, QColor("#ff0000")),
		tabOf(&explicitColor, QColor("#0000ff")),   // явный цвет важнее темы
		tabOf(&plain)
	});

	QCOMPARE(groups.size(), 2);
	QCOMPARE(groups.at(0).title, QString("#ff0000"));
	QCOMPARE(documentsOf(groups.at(0).tabs), QList<DocumentWidget*>({ &themed, &explicitColor }));
	QCOMPARE(groups.at(1).title, QStringLiteral("Без цвета"));
	QCOMPARE(documentsOf(groups.at(1).tabs), QList<DocumentWidget*>({ &plain }));
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
	const auto groups = registry.strategy("xtype")->arrange(
		{ tabOf(&router), tabOf(&untyped), tabOf(&editor), tabOf(&secondEditor) });

	QCOMPARE(groups.size(), 3);
	QCOMPARE(groups.at(0).title, QString("code-editor"));
	QCOMPARE(documentsOf(groups.at(0).tabs), QList<DocumentWidget*>({ &editor, &secondEditor }));
	QCOMPARE(groups.at(1).title, QString("system-router"));
	QCOMPARE(documentsOf(groups.at(1).tabs), QList<DocumentWidget*>({ &router }));
	QCOMPARE(groups.at(2).title, QStringLiteral("Без типа"));
	QCOMPARE(documentsOf(groups.at(2).tabs), QList<DocumentWidget*>({ &untyped }));
}

void TestDocumentTabs::documentTabThemeParsesColors() {
	// Given: тема привязывает цвет и глиф к классу, xtype и default
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto path = dir.filePath(DocumentTabTheme::fileName());
	QVERIFY(writeTextFile(path, R"(tabs:
  default: "#00ff00"
  defaultGlyph: "❔"
  colors:
    App.Documents.EditorDocument: "#61afef"
    code-editor: "#e06c75"
  glyphs:
    App.Documents.EditorDocument: "✏"
    code-editor: "📝"
)"));
	QVERIFY(DocumentTabTheme::fileName().endsWith("documents.yaml"));

	// When
	const auto theme = DocumentTabTheme::loadFromFile(path);

	// Then: ключи разбираются, класс важнее xtype, default — запасные значения
	QVERIFY(!theme.isEmpty());
	QCOMPARE(theme.colors().size(), 2);
	QCOMPARE(theme.colorFor("App.Documents.EditorDocument", "code-editor"), QColor("#61afef"));
	QCOMPARE(theme.colorFor(QString(), "code-editor"), QColor("#e06c75"));
	QCOMPARE(theme.colorFor("App.Documents.Other", "other"), QColor("#00ff00"));
	QCOMPARE(theme.defaultColor(), QColor("#00ff00"));

	QCOMPARE(theme.glyphs().size(), 2);
	QCOMPARE(theme.glyphFor("App.Documents.EditorDocument", "code-editor"), QString("✏"));
	QCOMPARE(theme.glyphFor(QString(), "code-editor"), QString("📝"));
	QCOMPARE(theme.glyphFor("App.Documents.Other", "other"), QString("❔"));
	QCOMPARE(theme.defaultGlyph(), QString("❔"));
}

void TestDocumentTabs::documentTabThemeIgnoresMissingFile() {
	const auto missing = DocumentTabTheme::loadFromFile(QStringLiteral("no/such/documents.yaml"));
	QVERIFY(missing.isEmpty());
	QVERIFY(!missing.defaultColor().isValid());
	QVERIFY(missing.defaultGlyph().isEmpty());
	QVERIFY(!missing.colorFor("App.Document", "doc").isValid());
	QVERIFY(missing.glyphFor("App.Document", "doc").isEmpty());

	const auto emptyDirectory = DocumentTabTheme::loadFromThemeDirectory(QString());
	QVERIFY(emptyDirectory.isEmpty());
}

void TestDocumentTabs::documentTabThemeFromConfigurations() {
	// Эталонная тема поставляется с привязками цвета и глифа вкладок.
	const QString themeDir = QStringLiteral(REDCOMPLEX_CONFIGURATIONS_DIR) +
		QStringLiteral("/assets/themes/dark");
	const auto theme = DocumentTabTheme::loadFromThemeDirectory(themeDir);

	QVERIFY(!theme.isEmpty());
	QVERIFY(theme.colorFor("Content.CodeEditor.CodeEditorDocument", "code-editor").isValid());
	QVERIFY(theme.colorFor(QString(), "code-editor").isValid());
	QVERIFY(theme.colorFor("System.Documents.SystemOptions", QString()).isValid());

	// Глифы задаются тем же способом: по классу и по xtype.
	QCOMPARE(theme.glyphFor("Content.CodeEditor.CodeEditorDocument", "code-editor"), QString("📝"));
	QCOMPARE(theme.glyphFor(QString(), "code-editor"), QString("📝"));
	QCOMPARE(theme.glyphFor("System.Documents.SystemOptions", QString()), QString("⚙"));
	QCOMPARE(theme.glyphFor("Content.SystemRouter.SystemRouterTreeDocument", QString()), QString("🗂"));

	// Неизвестный документ остаётся в цвете темы и с глифом по умолчанию:
	// default/defaultGlyph в эталонной теме не заданы.
	QVERIFY(!theme.colorFor("Unknown.Document", "unknown").isValid());
	QVERIFY(theme.glyphFor("Unknown.Document", "unknown").isEmpty());
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

void TestDocumentTabs::panelTabToolTipFromContextVariable() {
	TestCommandController controller;
	DocumentTabsPanel panel;
	panel.resize(220, 400);

	DocumentWidget document;
	auto context = controller.globalContext()->createDocumentContext(
		QStringLiteral("editor"));
	QVERIFY(context);
	document.setContext(context);
	document.setTitle(QStringLiteral("Файл"));
	document.setTabToolTipVariable(QStringLiteral("path"));
	context->setVariable(QStringLiteral("path"), QStringLiteral("/tmp/file.txt"));

	panel.addDocument(&document);
	panel.show();
	QCoreApplication::processEvents();

	const auto items = tabItems(panel);
	QCOMPARE(items.size(), 1);
	QCOMPARE(items.first()->toolTip(), QStringLiteral("/tmp/file.txt"));

	// Путь изменился — подсказка вкладки обновилась.
	context->setVariable(QStringLiteral("path"), QStringLiteral("/tmp/other.txt"));
	QCOMPARE(tabItems(panel).first()->toolTip(), QStringLiteral("/tmp/other.txt"));

	// Переменная не задана — подсказкой служит заголовок.
	document.setTabToolTipVariable(QString());
	QCOMPARE(tabItems(panel).first()->toolTip(), document.title());

	// Сам документ (рабочая область) подсказку не получает.
	QVERIFY(document.toolTip().isEmpty());
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
	QVERIFY(hasColorBar(items.at(0), QColor("#ff0000")));

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

void TestDocumentTabs::panelUsesThemeColors() {
	// Given: тема привязывает цвета к классу и xtype
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto path = dir.filePath(DocumentTabTheme::fileName());
	QVERIFY(writeTextFile(path, R"(tabs:
  colors:
    App.Documents.EditorDocument: "#61afef"
    code-editor: "#e06c75"
)"));

	DocumentTabsPanel panel;
	panel.resize(220, 400);
	panel.setDocumentTabTheme(DocumentTabTheme::loadFromFile(path));
	QVERIFY(!panel.documentTabTheme().isEmpty());

	DocumentWidget byClass;
	byClass.setTitle("Класс");
	byClass.setClassName("App.Documents.EditorDocument");
	byClass.setXtype("code-editor");

	DocumentWidget byXtype;
	byXtype.setTitle("Тип");
	byXtype.setXtype("code-editor");

	DocumentWidget explicitColor;
	explicitColor.setTitle("Свой цвет");
	explicitColor.setClassName("App.Documents.EditorDocument");
	explicitColor.setColor(QColor("#ff0000"));

	panel.addDocument(&byClass);
	panel.addDocument(&byXtype);
	panel.addDocument(&explicitColor);
	panel.show();
	QCoreApplication::processEvents();

	// Цвет класса важнее цвета xtype.
	QCOMPARE(panel.documentTabTheme().colorFor("App.Documents.EditorDocument", "code-editor"),
		QColor("#61afef"));
	QCOMPARE(panel.documentTabTheme().colorFor(QString(), "code-editor"), QColor("#e06c75"));

	// Цвет из темы рисуется полосой; явный цвет документа важнее темы.
	const auto items = tabItems(panel);
	QCOMPARE(items.size(), 3);
	QVERIFY(hasColorBar(items.at(0), QColor("#61afef")));
	QVERIFY(hasColorBar(items.at(1), QColor("#e06c75")));
	QVERIFY(hasColorBar(items.at(2), QColor("#ff0000")));

	// Группировка по цвету учитывает цвет темы.
	QVERIFY(panel.setOrderStrategy("color"));
	QCOMPARE(panel.groupTitles(), QStringList({ "#61afef", "#e06c75", "#ff0000" }));
}

void TestDocumentTabs::panelUsesThemeGlyph() {
	// Given: тема привязывает стандартные глифы к классу, xtype и default
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto path = dir.filePath(DocumentTabTheme::fileName());
	QVERIFY(writeTextFile(path, R"(tabs:
  defaultGlyph: "❔"
  glyphs:
    App.Documents.EditorDocument: "✏"
    code-editor: "📝"
)"));

	DocumentTabsPanel panel;
	panel.resize(220, 400);
	panel.setDocumentTabTheme(DocumentTabTheme::loadFromFile(path));

	DocumentWidget byClass;
	byClass.setTitle("Класс");
	byClass.setClassName("App.Documents.EditorDocument");
	byClass.setXtype("code-editor");

	DocumentWidget byXtype;
	byXtype.setTitle("Тип");
	byXtype.setXtype("code-editor");

	DocumentWidget explicitGlyph;
	explicitGlyph.setTitle("Свой глиф");
	explicitGlyph.setClassName("App.Documents.EditorDocument");
	explicitGlyph.setGlyph("★");

	DocumentWidget unknown;
	unknown.setTitle("Без привязки");

	panel.addDocument(&byClass);
	panel.addDocument(&byXtype);
	panel.addDocument(&explicitGlyph);
	panel.addDocument(&unknown);
	panel.show();
	QCoreApplication::processEvents();

	// Документ получает стандартный глиф из темы; явный глиф важнее.
	QCOMPARE(byClass.glyph(), QString("✏"));
	QCOMPARE(byClass.themeGlyph(), QString("✏"));
	QCOMPARE(byXtype.glyph(), QString("📝"));
	QCOMPARE(explicitGlyph.glyph(), QString("★"));
	QCOMPARE(unknown.glyph(), QString("❔"));

	// Сброс явного глифа возвращает глиф темы.
	explicitGlyph.setGlyph(QString());
	QCOMPARE(explicitGlyph.glyph(), QString("✏"));

	// Вкладка рисует глиф темы.
	const auto glyphText = [](QWidget* item) {
		auto* label = item->findChild<QLabel*>(QStringLiteral("DocumentTabGlyph"));
		return label ? label->text() : QString();
	};
	const auto items = tabItems(panel);
	QCOMPARE(items.size(), 4);
	QCOMPARE(glyphText(items.at(0)), QString("✏"));
	QCOMPARE(glyphText(items.at(1)), QString("📝"));
	QCOMPARE(glyphText(items.at(2)), QString("✏"));
	QCOMPARE(glyphText(items.at(3)), QString("❔"));

	// Тема без defaultGlyph возвращает документу глиф по умолчанию.
	panel.setDocumentTabTheme(DocumentTabTheme());
	QCOMPARE(unknown.glyph(), DocumentWidget::defaultGlyph());
	QCOMPARE(explicitGlyph.glyph(), DocumentWidget::defaultGlyph());
}

void TestDocumentTabs::panelLoadsThemeFromResources() {
	// Тема известна панели через ресурсы приложения (`path.theme`).
	const QString themeDir = QStringLiteral(REDCOMPLEX_CONFIGURATIONS_DIR) +
		QStringLiteral("/assets/themes/dark");

	Resources resources;
	resources.Directories.set(QStringLiteral("path.theme"), QDir(themeDir));
	TestCommandController controller(&resources);
	controller.initSystemContexts();

	// Вкладка: цвет и глиф по классу/типу берутся из темы без ручной настройки.
	DocumentTabsPanel panel(controller.globalContext());
	QVERIFY(!panel.documentTabTheme().isEmpty());
	QVERIFY(panel.documentTabTheme().colorFor(QString(), "code-editor").isValid());
	QCOMPARE(panel.documentTabTheme().glyphFor(QString(), "code-editor"), QString("📝"));

	// Документ, созданный окном, получает класс типа, цвет и глиф вкладки из темы.
	controller.widgets()->registerFactory({ "App.Documents.EditorDocument", "code-editor",
		/*singletone*/ false, QStringLiteral("Редактор") },
		[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			auto* document = new DocumentWidget(handler.parent);
			document->setTitle(QStringLiteral("Редактор"));
			return document;
		});

	MainWindow window(controller.globalContext(), nullptr, false);
	auto* widget = window.createWidget(QStringLiteral("editor"),
		MainWindow::WidgetKind::Document, QStringLiteral("Редактор"), QString(),
		[] { return new DocumentWidget(); }, QString(), QStringLiteral("code-editor"),
		controller.globalContext());
	QVERIFY(widget);

	auto* document = window.document(QStringLiteral("editor"));
	QVERIFY(document);
	QCOMPARE(document->xtype(), QStringLiteral("code-editor"));
	QCOMPARE(document->className(), QStringLiteral("App.Documents.EditorDocument"));
	QCOMPARE(document->context()->variable(QStringLiteral("className")).toString(),
		QStringLiteral("App.Documents.EditorDocument"));

	// Панель окна загрузила ту же тему, нашла цвет и глиф для документа.
	auto* windowPanel = window.findChild<DocumentTabsPanel*>();
	QVERIFY(windowPanel);
	QVERIFY(!windowPanel->documentTabTheme().isEmpty());
	QCOMPARE(windowPanel->documentTabTheme().colorFor(document->className(), document->xtype()),
		panel.documentTabTheme().colorFor(QString(), "code-editor"));
	QCOMPARE(document->glyph(), QString("📝"));
	QCOMPARE(document->themeGlyph(), QString("📝"));
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

	document.setClassName("App.Documents.EditorDocument");
	QCOMPARE(document.className(), QString("App.Documents.EditorDocument"));
	document.setClassName("App.Documents.EditorDocument");
	QCOMPARE(document.className(), QString("App.Documents.EditorDocument"));

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
	document.setClassName("Test.Widget");
	QCOMPARE(context->variable("color").value<QColor>(), QColor("#123456"));
	QCOMPARE(context->variable("xtype").toString(), QString("test-widget"));
	QCOMPARE(context->variable("className").toString(), QString("Test.Widget"));

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
		SystemOptionsDocument::xtype(), /*singletone*/ true,
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
