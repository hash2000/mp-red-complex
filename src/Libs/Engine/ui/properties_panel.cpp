#include "Libs/Engine/ui/properties_panel.h"
#include "Libs/Engine/ui/glyph_tool_button.h"
#include "Libs/Engine/ui/doc_widget.h"
#include "Libs/BaseWidgets/properties/properties_list_widget.h"
#include "Libs/Engine/scripting/command_context.h"

#include <QClipboard>
#include <QColor>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QScrollArea>
#include <QStringList>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

/// Текстовое представление значения переменной контекста для списка свойств.
QString variableValueText(const QVariant& value) {
	if (!value.isValid()) {
		return QStringLiteral("—");
	}

	// Цвет показываем в обычной для темы форме #rrggbb.
	if (value.canConvert<QColor>()) {
		const auto color = value.value<QColor>();
		if (color.isValid()) {
			return color.name();
		}
	}

	if (value.typeId() == QMetaType::QStringList) {
		return value.toStringList().join(QStringLiteral(", "));
	}

	if (value.typeId() == QMetaType::QVariantList) {
		QStringList parts;
		for (const auto& item : value.toList()) {
			parts << variableValueText(item);
		}

		return parts.join(QStringLiteral(", "));
	}

	const auto text = value.toString();
	if (!text.isEmpty()) {
		return text;
	}

	const auto* typeName = value.typeName();
	return typeName && *typeName
		? QStringLiteral("(%1)").arg(QString::fromLatin1(typeName))
		: QStringLiteral("—");
}

} // namespace

struct PropertiesPanel::Private {
	PropertiesPanel* q = nullptr;
	QPointer<DocumentWidget> doc;
	QLabel* header = nullptr;
	QToolButton* refreshBtn = nullptr;
	QScrollArea* scroll = nullptr;
	QLabel* empty = nullptr;

	// Секция контекста: пересоздаётся при каждой перестройке панели.
	QLineEdit* contextName = nullptr;
	QToolButton* contextCopy = nullptr;
	PropertiesListWidget* contextVariables = nullptr;
	QMetaObject::Connection contextConnection;

	void rebuild();
	void updateHeader();
	/// Перечитывает имя контекста и его переменные (после их изменения).
	void updateContext();
	void connectContext();

	void buildContextSection(QWidget* parent, QVBoxLayout* layout);
	/// Видимые переменные контекста документа в виде свойств только для чтения.
	QList<PropertyData> contextProperties() const;
};

void PropertiesPanel::Private::rebuild() {
	if (QWidget* old = scroll->takeWidget()) {
		old->deleteLater();   // deleteLater: могли прийти из сигнала самого редактора
	}

	contextName = nullptr;
	contextCopy = nullptr;
	contextVariables = nullptr;

	QWidget* editor = doc ? doc->createPropertiesEditor(scroll) : nullptr;
	const auto context = doc ? doc->context() : nullptr;
	updateHeader();
	refreshBtn->setEnabled(doc != nullptr);

	if (!editor && !context) {
		empty->setText(doc ? PropertiesPanel::tr("У документа нет свойств")
			: PropertiesPanel::tr("Нет выбранного документа"));
		scroll->hide();
		empty->show();
		return;
	}

	auto* content = new QWidget(scroll);
	auto* layout = new QVBoxLayout(content);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(8);

	if (editor) {
		layout->addWidget(editor);
	}

	if (context) {
		buildContextSection(content, layout);
	}

	layout->addStretch(1);
	scroll->setWidget(content);
	empty->hide();
	scroll->show();
}

void PropertiesPanel::Private::updateHeader() {
	header->setText(doc ? doc->glyph() + QLatin1Char(' ') + doc->title() : QString());
}

void PropertiesPanel::Private::buildContextSection(QWidget* parent, QVBoxLayout* layout) {
	const auto context = doc ? doc->context() : nullptr;
	if (!context) {
		return;
	}

	auto* title = new QLabel(PropertiesPanel::tr("Контекст документа"), parent);
	title->setObjectName(QStringLiteral("PropertiesContextTitle"));
	layout->addWidget(title);

	// Полное имя контекста, в котором выполняется документ: только для чтения
	// и с кнопкой копирования.
	auto* nameRow = new QWidget(parent);
	auto* nameLayout = new QHBoxLayout(nameRow);
	nameLayout->setContentsMargins(0, 0, 0, 0);
	nameLayout->setSpacing(4);

	contextName = new QLineEdit(context->nameSpace(), nameRow);
	contextName->setObjectName(QStringLiteral("PropertiesContextName"));
	contextName->setReadOnly(true);
	contextName->setCursorPosition(0);
	contextName->setToolTip(PropertiesPanel::tr(
		"Полное имя контекста, в котором выполняется документ"));

	contextCopy = new GlyphToolButton(QStringLiteral("⧉"),
		PropertiesPanel::tr("Копировать имя контекста"), nameRow);
	contextCopy->setObjectName(QStringLiteral("PropertiesContextCopy"));

	QObject::connect(contextCopy, &QToolButton::clicked, contextName, [name = contextName] {
		if (auto* clipboard = QGuiApplication::clipboard()) {
			clipboard->setText(name->text());
		}
	});

	nameLayout->addWidget(contextName, 1);
	nameLayout->addWidget(contextCopy);
	layout->addWidget(nameRow);

	auto* variablesTitle = new QLabel(PropertiesPanel::tr("Переменные контекста"), parent);
	variablesTitle->setObjectName(QStringLiteral("PropertiesContextVariablesTitle"));
	layout->addWidget(variablesTitle);

	contextVariables = new PropertiesListWidget(parent);
	contextVariables->setObjectName(QStringLiteral("PropertiesContextVariables"));
	contextVariables->setProperties(contextProperties());
	layout->addWidget(contextVariables);
}

QList<PropertyData> PropertiesPanel::Private::contextProperties() const {
	QList<PropertyData> result;
	const auto context = doc ? doc->context() : nullptr;
	if (!context) {
		return result;
	}

	const auto own = context->ownVariables();
	const auto visible = context->variables();
	for (auto it = visible.cbegin(); it != visible.cend(); it++) {
		PropertyData data;
		data.id = it.key();
		data.label = it.key();
		data.type = PropertyType::Text;
		data.value = variableValueText(it.value());
		data.readOnly = true;

		QStringList hints;
		hints << (own.contains(it.key())
			? PropertiesPanel::tr("собственная переменная")
			: PropertiesPanel::tr("унаследована"));

		const auto mimeType = context->variableMimeType(it.key());
		if (!mimeType.isEmpty()) {
			hints << mimeType;
		}

		data.tooltip = hints.join(QStringLiteral(", "));
		result << data;
	}

	return result;
}

void PropertiesPanel::Private::updateContext() {
	const auto context = doc ? doc->context() : nullptr;
	if (!context) {
		return;
	}

	if (contextName) {
		contextName->setText(context->nameSpace());
		contextName->setCursorPosition(0);
	}

	if (contextVariables) {
		contextVariables->setProperties(contextProperties());
	}
}

void PropertiesPanel::Private::connectContext() {
	if (contextConnection) {
		QObject::disconnect(contextConnection);
		contextConnection = QMetaObject::Connection();
	}

	const auto context = doc ? doc->context() : nullptr;
	if (!context || !q) {
		return;
	}

	// Переменные контекста меняются и снаружи (другие документы, команды):
	// список обновляется по сигналу контекста.
	contextConnection = QObject::connect(context.get(), &CommandContext::variableChanged,
		q, [this](const QString&, const QString&, const QVariant&, const QVariant&) {
			updateContext();
		});
}

PropertiesPanel::PropertiesPanel(QWidget* parent)
	: QWidget(parent), d(std::make_unique<Private>()) {
	d->q = this;

	auto* root = new QVBoxLayout(this);
	root->setContentsMargins(8, 8, 8, 8);
	root->setSpacing(8);

	auto* top = new QHBoxLayout;
	d->header = new QLabel(this);
	d->header->setObjectName(QStringLiteral("PropertiesPanelHeader"));
	d->refreshBtn = new GlyphToolButton(QStringLiteral("⟳"), tr("Обновить"), this);
	top->addWidget(d->header, 1);
	top->addWidget(d->refreshBtn);

	d->scroll = new QScrollArea(this);
	d->scroll->setObjectName(QStringLiteral("PropertiesScroll"));
	d->scroll->setWidgetResizable(true);

	d->empty = new QLabel(this);
	d->empty->setObjectName(QStringLiteral("EmptyHint"));
	d->empty->setAlignment(Qt::AlignCenter);
	d->empty->setWordWrap(true);

	root->addLayout(top);
	root->addWidget(d->scroll, 1);
	root->addWidget(d->empty, 1);

	connect(d->refreshBtn, &QToolButton::clicked, this, [this] {
		if (d->doc) {
			d->doc->refresh(); d->rebuild();
		}
	});

	d->rebuild();
}

PropertiesPanel::~PropertiesPanel() = default;

void PropertiesPanel::setDocument(DocumentWidget* doc) {
	if (d->doc == doc) return;
	if (d->doc) disconnect(d->doc, nullptr, this, nullptr);

	d->doc = doc;
	if (doc) {
		connect(doc, &DocumentWidget::titleChanged, this, [this] { d->updateHeader(); });
		connect(doc, &DocumentWidget::glyphChanged, this, [this] { d->updateHeader(); });
		connect(doc, &DocumentWidget::propertiesChanged, this, [this] { d->rebuild(); });
	}
	d->connectContext();
	d->rebuild();
}

DocumentWidget* PropertiesPanel::document() const {
	return d->doc;
}
