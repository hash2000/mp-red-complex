#include "Libs/Engine/ui/properties_panel.h"
#include "Libs/Engine/ui/glyph_tool_button.h"
#include "Libs/Engine/ui/doc_widget.h"
#include "Libs/BaseWidgets/properties/properties_group_widget.h"
#include "Libs/BaseWidgets/properties/properties_list_widget.h"
#include "Libs/Engine/scripting/command_context.h"

#include <QClipboard>
#include <QColor>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHash>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QScrollArea>
#include <QSet>
#include <QShowEvent>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

/// Идентификаторы групп верхнего уровня.
const QString kDocumentGroup = QStringLiteral("document");
const QString kTabGroup = QStringLiteral("tab");
const QString kContextGroup = QStringLiteral("context");
const QString kContextVariablesGroup = QStringLiteral("context.variables");

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

/// Пуст ли редактор свойств: у документа без собственных параметров
/// показывать пустую группу не нужно.
bool editorIsEmpty(QWidget* editor) {
	if (!editor) {
		return true;
	}

	if (auto* form = qobject_cast<QFormLayout*>(editor->layout())) {
		return form->rowCount() == 0;
	}

	auto* layout = editor->layout();
	return !layout || layout->count() == 0;
}

} // namespace

struct PropertiesPanel::Private {
	PropertiesPanel* q = nullptr;
	QPointer<DocumentWidget> doc;

	QLabel* header = nullptr;
	QToolButton* refreshBtn = nullptr;
	QScrollArea* scroll = nullptr;
	QLabel* empty = nullptr;

	/// Группы текущего документа: идентификатор → виджет группы.
	QHash<QString, PropertiesGroupWidget*> groups;
	/// Родительская группа для вложенных групп (нужна для раскрытия ветки).
	QHash<QString, QString> groupParents;
	/// Состояние раскрытия переживает перестройку панели.
	QHash<QString, bool> expandedStates;

	/// Панель перестроена, пока была скрыта: при первом показе строим
	/// содержимое ещё раз уже с реальной геометрией.
	bool rebuildOnShow = false;
	/// Группа, к которой нужно прокрутить панель после показа.
	QString pendingReveal;

	// Содержимое группы «Параметры контекста».
	QLineEdit* contextName = nullptr;
	QToolButton* contextCopy = nullptr;
	/// Список переменных создаётся при первом раскрытии вложенной группы.
	QPointer<PropertiesListWidget> contextVariables = nullptr;
	QMetaObject::Connection contextConnection;

	void rebuild();
	/// Строит содержимое панели без учёта видимости.
	void buildNow();
	void updateHeader();
	/// Перечитывает имя контекста и его переменные (после их изменения).
	void updateContext();
	void connectContext();

	PropertiesGroupWidget* addGroup(QWidget* parent, QVBoxLayout* layout,
		const QString& id, const QString& title, bool expanded,
		const QString& parentId = QString());
	QWidget* createContextSection(QWidget* parent);
	QWidget* createContextVariables(QWidget* parent);
	/// Видимые переменные контекста документа в виде свойств только для чтения.
	QList<PropertyData> contextProperties() const;
};

void PropertiesPanel::Private::rebuild() {
	rebuildOnShow = q && !q->isVisible();
	buildNow();
}

void PropertiesPanel::Private::buildNow() {
	if (QWidget* old = scroll->takeWidget()) {
		old->deleteLater();   // deleteLater: могли прийти из сигнала самого редактора
	}

	groups.clear();
	groupParents.clear();
	contextName = nullptr;
	contextCopy = nullptr;
	contextVariables = nullptr;

	updateHeader();
	refreshBtn->setEnabled(doc != nullptr);

	if (!doc) {
		empty->setText(PropertiesPanel::tr("Нет выбранного документа"));
		scroll->hide();
		empty->show();
		return;
	}

	const auto context = doc->context();

	auto* content = new QWidget(scroll);
	content->setObjectName(QStringLiteral("PropertiesPanelContent"));
	auto* layout = new QVBoxLayout(content);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(6);

	// 1. Свойства документа: редактор документа и его дополнительные группы.
	QWidget* documentEditor = doc->createPropertiesEditor(content);
	const auto documentGroups = doc->createPropertyGroups(content);
	if (!editorIsEmpty(documentEditor) || !documentGroups.isEmpty()) {
		auto* documentGroup = addGroup(content, layout, kDocumentGroup,
			PropertiesPanel::tr("Свойства документа"), /*expanded*/ true);

		auto* groupContent = new QWidget(documentGroup);
		auto* groupLayout = new QVBoxLayout(groupContent);
		groupLayout->setContentsMargins(0, 0, 0, 0);
		groupLayout->setSpacing(4);

		if (documentEditor) {
			groupLayout->addWidget(documentEditor);
		}

		for (const auto& description : documentGroups) {
			auto* group = addGroup(groupContent, groupLayout, description.id,
				description.title, description.expanded, kDocumentGroup);
			if (description.editor) {
				group->setContent(description.editor);
			}
		}

		documentGroup->setContent(groupContent);
	}
	else if (documentEditor) {
		// У документа нет собственных параметров: пустой редактор не нужен.
		documentEditor->deleteLater();
	}

	// 2. Свойства вкладки: заголовок, значок, цвет, тип, изменённость.
	auto* tabEditor = doc->createTabPropertiesEditor(content);
	auto* tabGroup = addGroup(content, layout, kTabGroup,
		PropertiesPanel::tr("Свойства вкладки"), /*expanded*/ true);
	if (tabEditor) {
		tabGroup->setContent(tabEditor);
	}

	// 3. Параметры контекста: имя и (лениво) список переменных.
	if (context) {
		auto* contextGroup = addGroup(content, layout, kContextGroup,
			PropertiesPanel::tr("Параметры контекста"), /*expanded*/ true);
		contextGroup->setContent(createContextSection(contextGroup));
	}

	layout->addStretch(1);
	scroll->setWidget(content);
	empty->hide();
	scroll->show();
}

PropertiesGroupWidget* PropertiesPanel::Private::addGroup(QWidget* parent,
	QVBoxLayout* layout, const QString& id, const QString& title, bool expanded,
	const QString& parentId) {
	auto* group = new PropertiesGroupWidget(title, parent);
	group->setGroupId(id);
	group->setProperty("groupId", id);

	const bool initialState = expandedStates.value(id, expanded);
	group->setExpanded(initialState);

	QObject::connect(group, &PropertiesGroupWidget::expandedChanged, q,
		[this, id](bool value) {
			expandedStates.insert(id, value);
			if (value) {
				emit q->groupExpanded(id);
			}
		});

	layout->addWidget(group);
	groups.insert(id, group);
	if (!parentId.isEmpty()) {
		groupParents.insert(id, parentId);
	}

	return group;
}

void PropertiesPanel::Private::updateHeader() {
	header->setText(doc ? doc->glyph() + QLatin1Char(' ') + doc->title() : QString());
}

QWidget* PropertiesPanel::Private::createContextSection(QWidget* parent) {
	const auto context = doc ? doc->context() : nullptr;
	if (!context) {
		return nullptr;
	}

	auto* content = new QWidget(parent);
	auto* layout = new QVBoxLayout(content);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(4);

	// Полное имя контекста, в котором выполняется документ: только для чтения
	// и с кнопкой копирования.
	auto* nameRow = new QWidget(content);
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

	// Список переменных не загружается заранее: он строится при раскрытии.
	auto* variablesGroup = addGroup(content, layout, kContextVariablesGroup,
		PropertiesPanel::tr("Переменные контекста"), /*expanded*/ false, kContextGroup);
	variablesGroup->setContentFactory([this] {
		return createContextVariables(groups.value(kContextVariablesGroup));
	});

	return content;
}

QWidget* PropertiesPanel::Private::createContextVariables(QWidget* parent) {
	auto* variables = new PropertiesListWidget(parent);
	variables->setObjectName(QStringLiteral("PropertiesContextVariables"));
	variables->setHeaderVisible(false);
	variables->setScrollable(false);
	variables->setProperties(contextProperties());
	contextVariables = variables;
	return variables;
}

QList<PropertyData> PropertiesPanel::Private::contextProperties() const {
	QList<PropertyData> result;
	const auto context = doc ? doc->context() : nullptr;
	if (!context) {
		return result;
	}

	QSet<QString> seen;
	const auto append = [this, &context, &result, &seen](
		const std::shared_ptr<CommandContext>& source, bool own) {
		if (!source) {
			return;
		}

		const QVariantMap variables = source->ownVariables();
		for (auto it = variables.cbegin(); it != variables.cend(); it++) {
			if (seen.contains(it.key())) {
				continue;
			}
			seen.insert(it.key());

			PropertyData data;
			data.id = it.key();
			data.label = it.key();
			data.type = PropertyType::Text;
			data.value = variableValueText(it.value());
			data.readOnly = true;

			QStringList hints;
			hints << (own
				? PropertiesPanel::tr("собственная переменная")
				: PropertiesPanel::tr("унаследована"));

			const auto mimeType = context->variableMimeType(it.key());
			if (!mimeType.isEmpty()) {
				hints << mimeType;
			}

			data.tooltip = hints.join(QStringLiteral(", "));
			result << data;
		}
	};

	// Сначала собственные переменные текущего контекста, затем — по цепочке
	// родителей, от ближайшего к глобальному.
	append(context, true);
	for (auto parent = context->parentContext(); parent;
		parent = parent->parentContext()) {
		append(parent, false);
	}

	// Значения, которые видны, но не являются собственными ни у одного
	// контекста цепочки (например, подставленные interceptor'ом).
	const auto visible = context->variables();
	for (auto it = visible.cbegin(); it != visible.cend(); it++) {
		if (seen.contains(it.key())) {
			continue;
		}
		seen.insert(it.key());

		PropertyData data;
		data.id = it.key();
		data.label = it.key();
		data.type = PropertyType::Text;
		data.value = variableValueText(it.value());
		data.readOnly = true;
		data.tooltip = PropertiesPanel::tr("унаследована");
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
			d->doc->refresh();
			d->rebuild();
		}
	});

	d->rebuild();
}

PropertiesPanel::~PropertiesPanel() = default;

void PropertiesPanel::showEvent(QShowEvent* event) {
	QWidget::showEvent(event);

	if (!d->scroll) {
		return;
	}

	// Свойства строились, пока панель была скрыта (страница стека боковых
	// панелей): при первом показе перестраиваем их с реальной геометрией.
	if (d->rebuildOnShow) {
		d->rebuildOnShow = false;
		d->buildNow();
	}

	if (QWidget* content = d->scroll->widget()) {
		content->updateGeometry();
		if (auto* layout = content->layout()) {
			layout->activate();
		}
	}

	// Группа, раскрытая до показа панели, прокручивается к себе уже сейчас.
	if (!d->pendingReveal.isEmpty()) {
		const auto groupId = d->pendingReveal;
		d->pendingReveal.clear();
		if (auto* group = d->groups.value(groupId)) {
			QPointer<PropertiesGroupWidget> guard(group);
			QTimer::singleShot(0, this, [this, guard] {
				if (guard && d->scroll) {
					d->scroll->ensureWidgetVisible(guard);
				}
			});
		}
	}
}

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

void PropertiesPanel::revealGroup(const QString& groupId) {
	if (groupId.isEmpty()) {
		return;
	}

	if (!d->groups.contains(groupId)) {
		// Групп ещё нет (панель только что привязана к документу) — перестроим.
		d->rebuild();
	}

	if (!d->groups.contains(groupId)) {
		return;
	}

	// Раскрываем ветку сверху вниз: вложенная группа видна только внутри
	// раскрытой родительской.
	QStringList branch;
	for (QString current = groupId; !current.isEmpty();
		current = d->groupParents.value(current)) {
		branch.prepend(current);
	}

	for (const auto& id : branch) {
		if (auto* group = d->groups.value(id)) {
			group->setExpanded(true);
		}
	}

	if (!isVisible()) {
		// Панель ещё не показана: прокрутим к группе после её появления.
		d->pendingReveal = groupId;
		return;
	}

	if (auto* group = d->groups.value(groupId)) {
		QPointer<PropertiesGroupWidget> guard(group);
		QTimer::singleShot(0, this, [this, guard] {
			if (guard && d->scroll) {
				d->scroll->ensureWidgetVisible(guard);
			}
		});
	}
}

bool PropertiesPanel::isGroupExpanded(const QString& groupId) const {
	auto* group = d->groups.value(groupId);
	return group && group->isExpanded();
}

QStringList PropertiesPanel::groupIds() const {
	auto ids = d->groups.keys();
	ids.sort();
	return ids;
}
