#include "Libs/BaseWidgets/properties/properties_group_widget.h"
#include "Libs/BaseWidgets/properties/properties_group_widget.h"
#include "Libs/BaseWidgets/properties/properties_list_widget.h"
#include "Libs/BaseWidgets/properties/tab_color_editor.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/command_processor.h"
#include "Libs/Engine/services/services_registry.h"
#include "Libs/Engine/scripting/widgets/widget_registry.h"
#include "Libs/Engine/ui/doc_widget.h"
#include "Libs/Engine/ui/main_window.h"
#include "Libs/Engine/ui/properties_panel.h"

#include <QClipboard>
#include <QCoreApplication>
#include <QFormLayout>
#include <QGuiApplication>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>
#include <QtTest/QtTest>

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

/// Документ с дополнительной группой свойств (как «Параметры поиска»
/// у редактора кода).
class GroupDocument : public DocumentWidget {
public:
	QWidget* createPropertiesEditor(QWidget* parent) override {
		auto* editor = DocumentWidget::createPropertiesEditor(parent);
		auto* form = qobject_cast<QFormLayout*>(editor->layout());
		if (form) {
			auto* label = new QLabel(QStringLiteral("doc-property"), editor);
			label->setObjectName(QStringLiteral("TestDocumentProperty"));
			form->addRow(QStringLiteral("Параметр:"), label);
		}
		return editor;
	}

	QList<DocumentPropertyGroup> createPropertyGroups(QWidget* parent) override {
		DocumentPropertyGroup group;
		group.id = QStringLiteral("search");
		group.title = QStringLiteral("Параметры поиска");
		group.editor = new QLabel(QStringLiteral("search-property"), parent);
		group.editor->setObjectName(QStringLiteral("TestSearchProperty"));
		return { group };
	}
};

/// Редактор свойства по идентификатору переменной.
template<typename T>
T* findPropertyEditor(QWidget* root, const QString& id) {
	for (auto* child : root->findChildren<QWidget*>()) {
		if (child->property("propertyId").toString() != id) {
			continue;
		}

		if (auto* editor = qobject_cast<T*>(child)) {
			return editor;
		}
		if (auto* nested = child->findChild<T*>()) {
			return nested;
		}
	}

	return nullptr;
}

/// Удалённые через deleteLater редакторы не должны попадать в поиск.
void flushDeferredDeletes() {
	QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

} // namespace

class TestPropertiesPanel : public QObject {
	Q_OBJECT

private slots:
	void emptyPanelShowsHint();
	void showsDocumentContextName();
	void copiesContextName();
	void createsTopLevelGroups();
	void groupHeaderTitleIsLeftAligned();
	void revealsDocumentPropertyGroup();
	void revealsTabPropertiesGroup();
	void contextVariablesAreLazy();
	void showsContextVariables();
	void updatesContextVariablesOnChange();
	void documentWithoutContextHasNoSection();
	void mainWindowShowsPropertiesGroup();
	void firstShowFillsGroups();
};

void TestPropertiesPanel::emptyPanelShowsHint() {
	PropertiesPanel panel;

	auto* hint = panel.findChild<QLabel*>(QStringLiteral("EmptyHint"));
	QVERIFY(hint);
	QCOMPARE(hint->text(), QStringLiteral("Нет выбранного документа"));
	QVERIFY(!panel.findChild<QLineEdit*>(QStringLiteral("PropertiesContextName")));
	QVERIFY(panel.groupIds().isEmpty());
}

void TestPropertiesPanel::showsDocumentContextName() {
	TestCommandController controller;
	auto context = controller.globalContext()->createDocumentContext(QStringLiteral("notes"));

	DocumentWidget document;
	document.setTitle(QStringLiteral("Заметки"));
	document.setContext(context);

	PropertiesPanel panel;
	panel.setDocument(&document);

	auto* name = panel.findChild<QLineEdit*>(QStringLiteral("PropertiesContextName"));
	QVERIFY(name);
	QCOMPARE(name->text(), QStringLiteral(".documents.notes"));
	QVERIFY(name->isReadOnly());

	auto* hint = panel.findChild<QLabel*>(QStringLiteral("EmptyHint"));
	QVERIFY(hint);
	QVERIFY(hint->isHidden());
}

void TestPropertiesPanel::copiesContextName() {
	TestCommandController controller;
	auto context = controller.globalContext()->createDocumentContext(QStringLiteral("editor"));

	DocumentWidget document;
	document.setContext(context);

	PropertiesPanel panel;
	panel.setDocument(&document);

	auto* copy = panel.findChild<QToolButton*>(QStringLiteral("PropertiesContextCopy"));
	QVERIFY(copy);

	copy->click();
	QCOMPARE(QGuiApplication::clipboard()->text(), QStringLiteral(".documents.editor"));
}

void TestPropertiesPanel::createsTopLevelGroups() {
	TestCommandController controller;
	auto context = controller.globalContext()->createDocumentContext(QStringLiteral("notes"));

	GroupDocument document;
	document.setContext(context);

	PropertiesPanel panel;
	panel.setDocument(&document);

	const auto ids = panel.groupIds();
	QVERIFY(ids.contains(QStringLiteral("document")));
	QVERIFY(ids.contains(QStringLiteral("tab")));
	QVERIFY(ids.contains(QStringLiteral("context")));
	QVERIFY(ids.contains(QStringLiteral("context.variables")));
	QVERIFY(ids.contains(QStringLiteral("search")));

	// Все группы верхнего уровня открыты; список переменных — по раскрытию.
	QVERIFY(panel.isGroupExpanded(QStringLiteral("document")));
	QVERIFY(panel.isGroupExpanded(QStringLiteral("tab")));
	QVERIFY(panel.isGroupExpanded(QStringLiteral("context")));
	QVERIFY(!panel.isGroupExpanded(QStringLiteral("context.variables")));

	// Документ отдал свою группу в «Свойства документа».
	QVERIFY(panel.findChild<QLabel*>(QStringLiteral("TestDocumentProperty")));
	QVERIFY(panel.findChild<QLabel*>(QStringLiteral("TestSearchProperty")));
}

void TestPropertiesPanel::groupHeaderTitleIsLeftAligned() {
	TestCommandController controller;
	auto context = controller.globalContext()->createDocumentContext(QStringLiteral("notes"));

	DocumentWidget document;
	document.setContext(context);

	PropertiesPanel panel;
	panel.setDocument(&document);

	auto* title = panel.findChild<QLabel*>(QStringLiteral("PropertiesGroupTitle"));
	QVERIFY(title);
	QVERIFY(title->alignment() & Qt::AlignLeft);
	QVERIFY(!(title->alignment() & Qt::AlignHCenter));
}

void TestPropertiesPanel::revealsDocumentPropertyGroup() {
	TestCommandController controller;
	auto context = controller.globalContext()->createDocumentContext(QStringLiteral("notes"));

	GroupDocument document;
	document.setContext(context);

	PropertiesPanel panel;
	panel.setDocument(&document);

	QVERIFY(!panel.isGroupExpanded(QStringLiteral("search")));

	QSignalSpy spy(&panel, &PropertiesPanel::groupExpanded);
	panel.revealGroup(QStringLiteral("search"));

	QVERIFY(panel.isGroupExpanded(QStringLiteral("search")));
	QVERIFY(panel.isGroupExpanded(QStringLiteral("document")));
	QVERIFY(!spy.isEmpty());
	QCOMPARE(spy.last().at(0).toString(), QStringLiteral("search"));
}

void TestPropertiesPanel::revealsTabPropertiesGroup() {
	TestCommandController controller;
	auto context = controller.globalContext()->createDocumentContext(QStringLiteral("notes"));

	DocumentWidget document;
	document.setTitle(QStringLiteral("Заметки"));
	document.setContext(context);

	PropertiesPanel panel;
	panel.setDocument(&document);

	panel.revealGroup(QStringLiteral("tab"));
	QVERIFY(panel.isGroupExpanded(QStringLiteral("tab")));

	// Редактор цвета вкладки живёт в группе «Свойства вкладки».
	QVERIFY(panel.findChild<TabColorEditor*>(QStringLiteral("DocumentColorEditor")));
}

void TestPropertiesPanel::contextVariablesAreLazy() {
	TestCommandController controller;
	auto context = controller.globalContext()->createDocumentContext(QStringLiteral("notes"));

	DocumentWidget document;
	document.setContext(context);
	QVERIFY(document.setVariable(QStringLiteral("path"), QStringLiteral("C:/tmp/notes.txt")));

	PropertiesPanel panel;
	panel.setDocument(&document);

	// Вложенная группа существует, но список не загружен до раскрытия.
	QVERIFY(panel.groupIds().contains(QStringLiteral("context.variables")));
	QVERIFY(!panel.isGroupExpanded(QStringLiteral("context.variables")));
	QVERIFY(!panel.findChild<PropertiesListWidget*>(
		QStringLiteral("PropertiesContextVariables")));

	panel.revealGroup(QStringLiteral("context.variables"));

	QVERIFY(panel.isGroupExpanded(QStringLiteral("context.variables")));
	auto* variables = panel.findChild<PropertiesListWidget*>(
		QStringLiteral("PropertiesContextVariables"));
	QVERIFY(variables);
	QVERIFY(variables->propertyIds().contains(QStringLiteral("path")));
}

void TestPropertiesPanel::showsContextVariables() {
	TestCommandController controller;
	auto context = controller.globalContext()->createDocumentContext(QStringLiteral("notes"));

	DocumentWidget document;
	document.setTitle(QStringLiteral("Заметки"));
	document.setXtype(QStringLiteral("notes"));
	document.setContext(context);
	QVERIFY(document.setVariable(QStringLiteral("path"), QStringLiteral("C:/tmp/notes.txt")));
	QVERIFY(document.setVariable(QStringLiteral("zoom"), 3));

	PropertiesPanel panel;
	panel.setDocument(&document);
	panel.revealGroup(QStringLiteral("context.variables"));

	auto* variables = panel.findChild<PropertiesListWidget*>(
		QStringLiteral("PropertiesContextVariables"));
	QVERIFY(variables);

	const auto ids = variables->propertyIds();
	QVERIFY(ids.contains(QStringLiteral("title")));
	QVERIFY(ids.contains(QStringLiteral("xtype")));
	QVERIFY(ids.contains(QStringLiteral("path")));
	QVERIFY(ids.contains(QStringLiteral("zoom")));

	auto* pathEditor = findPropertyEditor<QLineEdit>(variables, QStringLiteral("path"));
	QVERIFY(pathEditor);
	QCOMPARE(pathEditor->text(), QStringLiteral("C:/tmp/notes.txt"));
	QVERIFY(pathEditor->isReadOnly());

	// Переменные документа помечены как собственные, их mime-тип — в подсказке.
	QLabel* pathLabel = nullptr;
	for (auto* label : variables->findChildren<QLabel*>()) {
		if (label->text() == QStringLiteral("path")) {
			pathLabel = label;
			break;
		}
	}
	QVERIFY(pathLabel);
	QVERIFY(pathLabel->toolTip().contains(QStringLiteral("собственная переменная")));
}

void TestPropertiesPanel::updatesContextVariablesOnChange() {
	TestCommandController controller;
	auto context = controller.globalContext()->createDocumentContext(QStringLiteral("notes"));

	DocumentWidget document;
	document.setContext(context);

	PropertiesPanel panel;
	panel.setDocument(&document);
	panel.revealGroup(QStringLiteral("context.variables"));

	auto* variables = panel.findChild<PropertiesListWidget*>(
		QStringLiteral("PropertiesContextVariables"));
	QVERIFY(variables);
	QVERIFY(!variables->propertyIds().contains(QStringLiteral("zoom")));

	// Переменная появилась в контексте извне — список свойств обновился.
	QVERIFY(context->setVariable(QStringLiteral("zoom"), 7));
	flushDeferredDeletes();

	QVERIFY(variables->propertyIds().contains(QStringLiteral("zoom")));
	auto* zoomEditor = findPropertyEditor<QLineEdit>(variables, QStringLiteral("zoom"));
	QVERIFY(zoomEditor);
	QCOMPARE(zoomEditor->text(), QStringLiteral("7"));
}

void TestPropertiesPanel::documentWithoutContextHasNoSection() {
	DocumentWidget document;
	document.setTitle(QStringLiteral("Без контекста"));

	PropertiesPanel panel;
	panel.setDocument(&document);

	QVERIFY(!panel.groupIds().contains(QStringLiteral("context")));
	QVERIFY(!panel.groupIds().contains(QStringLiteral("context.variables")));
	QVERIFY(!panel.findChild<QLineEdit*>(QStringLiteral("PropertiesContextName")));
	QVERIFY(!panel.findChild<PropertiesListWidget*>(QStringLiteral("PropertiesContextVariables")));

	// Своих параметров у документа нет: группа «Свойства документа» не нужна,
	// но группа «Свойства вкладки» есть у любого документа.
	QVERIFY(!panel.groupIds().contains(QStringLiteral("document")));
	QVERIFY(panel.groupIds().contains(QStringLiteral("tab")));
}

void TestPropertiesPanel::mainWindowShowsPropertiesGroup() {
	TestCommandController controller;
	auto* registry = controller.widgets();
	registry->registerFactory({ QStringLiteral("Test.Panels.Properties"),
		QStringLiteral("properties"), /*singletone*/ true, QStringLiteral("Свойства") },
		[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			return new PropertiesPanel(handler.parent);
		});

	MainWindow window(controller.globalContext(), nullptr, false);

	// Панель свойств регистрируется как боковая панель (как в описании
	// приложения) и оборачивается в документ.
	auto* panelWidget = window.createWidget(QStringLiteral("properties"),
		MainWindow::WidgetKind::SidePanel, QStringLiteral("Свойства"), QStringLiteral("🛠"),
		[&registry] {
			return registry->build(QStringLiteral("properties"), WidgetBuildHandler());
		},
		QString(), QStringLiteral("properties"), controller.globalContext());
	QVERIFY(panelWidget);

	auto* panel = window.propertiesPanel();
	QVERIFY(panel);

	GroupDocument document;
	document.setContext(controller.globalContext()->createDocumentContext(QStringLiteral("notes")));

	// Так документ открывает свойства действия: панель показывает документ
	// и раскрывает указанную группу.
	QVERIFY(window.showDocumentProperties(&document, QStringLiteral("search")));
	QCOMPARE(panel->document(), &document);
	QVERIFY(panel->isGroupExpanded(QStringLiteral("search")));

	// Без панели свойств запрос отклоняется.
	MainWindow emptyWindow(controller.globalContext(), nullptr, false);
	QVERIFY(!emptyWindow.showDocumentProperties(&document, QStringLiteral("search")));
}

void TestPropertiesPanel::firstShowFillsGroups() {
	TestCommandController controller;
	auto* registry = controller.widgets();
	registry->registerFactory({ QStringLiteral("Test.Panels.Properties"),
		QStringLiteral("properties"), /*singletone*/ true, QStringLiteral("Свойства") },
		[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			return new PropertiesPanel(handler.parent);
		});

	MainWindow window(controller.globalContext(), nullptr, false);
	QVERIFY(window.createWidget(QStringLiteral("properties"),
		MainWindow::WidgetKind::SidePanel, QStringLiteral("Свойства"), QStringLiteral("🛠"),
		[&registry] {
			return registry->build(QStringLiteral("properties"), WidgetBuildHandler());
		},
		QString(), QStringLiteral("properties"), controller.globalContext()));

	auto* panel = window.propertiesPanel();
	QVERIFY(panel);

	GroupDocument document;
	document.setTitle(QStringLiteral("Заметки"));
	document.setContext(controller.globalContext()->createDocumentContext(QStringLiteral("notes")));

	// Панель скрыта, как при первом открытии документа: свойства строятся
	// до того, как панель станет видимой.
	panel->setDocument(&document);
	QVERIFY(!panel->isVisible());

	// Документ может попросить перестроить свойства ещё до первого показа
	// панели (например, редактор кода открывает файл очередью событий).
	emit document.propertiesChanged();
	QCoreApplication::processEvents();

	QVERIFY(window.showDocumentProperties(&document));
	window.show();
	QCoreApplication::processEvents();

	QVERIFY(panel->isVisible());
	QVERIFY(panel->isGroupExpanded(QStringLiteral("document")));

	auto* label = panel->findChild<QLabel*>(QStringLiteral("TestDocumentProperty"));
	QVERIFY(label);
	QVERIFY2(label->isVisible(), "свойства документа не видны при первом показе");
	QVERIFY2(label->height() > 0, "у свойства нулевая высота при первом показе");

	panel->revealGroup(QStringLiteral("tab"));
	QCoreApplication::processEvents();
	auto* colorEditor = panel->findChild<TabColorEditor*>(QStringLiteral("DocumentColorEditor"));
	QVERIFY(colorEditor);
	QVERIFY2(colorEditor->isVisible(), "свойства вкладки не видны при первом показе");
	QVERIFY2(colorEditor->height() > 0, "у свойства вкладки нулевая высота");
}

QTEST_MAIN(TestPropertiesPanel)
#include "test_properties_panel.moc"
