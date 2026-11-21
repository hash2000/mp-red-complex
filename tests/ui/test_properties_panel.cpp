#include "Libs/BaseWidgets/properties/properties_list_widget.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/command_processor.h"
#include "Libs/Engine/services/services_registry.h"
#include "Libs/Engine/ui/doc_widget.h"
#include "Libs/Engine/ui/properties_panel.h"

#include <QClipboard>
#include <QCoreApplication>
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
	void showsContextVariables();
	void updatesContextVariablesOnChange();
	void documentWithoutContextHasNoSection();
};

void TestPropertiesPanel::emptyPanelShowsHint() {
	PropertiesPanel panel;

	auto* hint = panel.findChild<QLabel*>(QStringLiteral("EmptyHint"));
	QVERIFY(hint);
	QCOMPARE(hint->text(), QStringLiteral("Нет выбранного документа"));
	QVERIFY(!panel.findChild<QLineEdit*>(QStringLiteral("PropertiesContextName")));
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
	QVERIFY(panel.findChild<QLabel*>(QStringLiteral("PropertiesContextTitle")));
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

	QVERIFY(!panel.findChild<QLineEdit*>(QStringLiteral("PropertiesContextName")));
	QVERIFY(!panel.findChild<PropertiesListWidget*>(QStringLiteral("PropertiesContextVariables")));
	QVERIFY(!panel.findChild<QLabel*>(QStringLiteral("PropertiesContextTitle")));
}

QTEST_MAIN(TestPropertiesPanel)
#include "test_properties_panel.moc"
