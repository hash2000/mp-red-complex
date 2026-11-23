#include "Content/CodeEditorWidget/documents/code_editor_document.h"
#include "Content/CodeEditorWidget/widgets/code_editor_widget.h"
#include "Content/CodeEditorWidget/widgets/code_editor_search_panel.h"
#include "Content/CodeEditorWidget/language_info.h"
#include "Content/CodeEditorWidget/highlights/highlighter_plugin.h"
#include "Content/CodeEditorWidget/highlights/highlighter_plugin_manager.h"
#include "Content/CodeEditorWidget/formatters/formatter_plugin_manager.h"
#include "Libs/BaseWidgets/properties/properties_list_widget.h"
#include "Libs/BaseWidgets/ribbon/document_ribbon.h"
#include "Libs/BaseWidgets/ribbon/ribbon_button.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QPair>
#include <QPointer>
#include <QSignalBlocker>
#include <QStringConverter>
#include <QTextDocument>
#include <QTextStream>
#include <QVBoxLayout>

#include <algorithm>

namespace {
constexpr int kFontSizeComboWidth = 76;

/// Идентификаторы свойств поиска в группе «Параметры поиска».
const QString kSearchRegularExpression = QStringLiteral("search.regex");
const QString kSearchSimplifiedExpression = QStringLiteral("search.simplified");
const QString kSearchCaseSensitive = QStringLiteral("search.case-sensitive");
const QString kSearchWholeWord = QStringLiteral("search.whole-word");
}

class CodeEditorDocument::Private {
public:
	Private(CodeEditorDocument* parent) : q(parent) {}
	CodeEditorDocument* q;

	CodeEditorWidget* editor = nullptr;
	HighlightingPluginManager* highlighting = nullptr;
	FormatterPluginManager* formatters = nullptr;

	DocumentRibbon* ribbon = nullptr;
	RibbonButton* searchButton = nullptr;
	RibbonButton* formatButton = nullptr;

	CodeEditorSearchPanel* searchPanel = nullptr;

	/// Редакторы параметров живут в панели «Свойства» и пересоздаются
	/// при каждой её перестройке, поэтому храним только слабые ссылки.
	QPointer<QComboBox> fontSizeCombo;
	QPointer<QComboBox> languageCombo;
	QPointer<PropertiesListWidget> searchProperties;

	QString path;
	bool dataMode = false;
	bool dataFromFile = false;
	bool settingPath = false;

	void setupUi();
	QWidget* createRibbon();
	QWidget* createSearchPanel();
	void setSearchVisible(bool visible);

	QWidget* createSearchPropertiesEditor(QWidget* parent);
	QList<PropertyData> searchPropertyData() const;
	void applySearchProperty(const QString& id, bool enabled);
	void setSearchOption(CodeEditorSearchPanel::Option option, bool enabled);

	void setupFontSizes();
	void setupLanguages();
	void syncLanguageCombo();
	void syncFontSizeCombo(int size);

	QString currentMimeType() const;
	QString currentLanguageName() const;
	void setPath(const QString& path);
	CommandController* controller() const;
	void printError(const QString& message) const;
	void printWarning(const QString& message) const;
	bool writeFile(const QString& path, const QString& text, QString* error) const;
	void applyDataText(const QString& data);
};

void CodeEditorDocument::Private::setupUi() {
	editor = new CodeEditorWidget(highlighting, formatters, q);
	editor->setLineWrapMode(QPlainTextEdit::NoWrap);
	editor->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	auto* layout = new QVBoxLayout(q);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(createRibbon(), 0);
	// Панель поиска живёт между лентой и редактором, чтобы не закрывать текст.
	layout->addWidget(createSearchPanel(), 0);
	layout->addWidget(editor, 1);

	q->setFocusProxy(editor);
}

QWidget* CodeEditorDocument::Private::createSearchPanel() {
	searchPanel = new CodeEditorSearchPanel(editor, q);
	searchPanel->hide();

	connect(searchPanel, &CodeEditorSearchPanel::closed, q, [this] {
		setSearchVisible(false);
	});

	return searchPanel;
}

void CodeEditorDocument::Private::setSearchVisible(bool visible) {
	if (!searchPanel) {
		return;
	}

	if (visible) {
		searchPanel->focusSearch();
		// Состояние кнопки ленты повторяет видимость панели.
		if (searchButton) {
			const QSignalBlocker blocker(searchButton);
			searchButton->setChecked(true);
		}
		return;
	}

	searchPanel->clearSearch();
	searchPanel->hide();
	if (searchButton) {
		const QSignalBlocker blocker(searchButton);
		searchButton->setChecked(false);
	}
	editor->setFocus();
}

QWidget* CodeEditorDocument::Private::createRibbon() {
	ribbon = new DocumentRibbon(q);

	// Лента содержит только функциональные кнопки: параметры действий
	// редактируются в панели «Свойства».
	auto* fileGroup = ribbon->addGroup(q->tr("Файл"));
	connect(ribbon->addButton(fileGroup, QStringLiteral("🗋"), q->tr("Новый файл")),
		&RibbonButton::clicked, q, [this] {
			q->newFile();
		});
	connect(ribbon->addButton(fileGroup, QStringLiteral("📂"), q->tr("Открыть файл")),
		&RibbonButton::clicked, q, [this] {
			const auto path = QFileDialog::getOpenFileName(q, q->tr("Открыть файл"),
				QDir::currentPath(), q->tr("Все файлы (*.*)"));
			if (!path.isEmpty()) {
				q->openFile(path);
			}
		});
	connect(ribbon->addButton(fileGroup, QStringLiteral("💾"), q->tr("Сохранить файл")),
		&RibbonButton::clicked, q, [this] {
			q->save();
		});

	auto* editGroup = ribbon->addGroup(q->tr("Правка"));
	formatButton = ribbon->addButton(editGroup, QStringLiteral("✨"), q->tr("Форматировать документ"));
	connect(formatButton, &RibbonButton::clicked, q, [this] {
		if (!editor->formatDocument()) {
			printWarning(q->tr("Для текущего языка форматирование недоступно."));
		}
	});

	// Пока внешний форматтер работает, повторный запуск не нужен:
	// кнопка блокируется до завершения (успешного или с ошибкой).
	connect(editor, &CodeEditorWidget::formattingStarted, q, [this] {
		if (formatButton) {
			formatButton->setEnabled(false);
		}
	});
	connect(editor, &CodeEditorWidget::formattingCompleted, q, [this] {
		if (formatButton) {
			formatButton->setEnabled(true);
		}
	});

	auto* searchGroup = ribbon->addGroup(q->tr("Поиск"));
	searchButton = ribbon->addButton(searchGroup, QStringLiteral("🔍"),
		q->tr("Поиск (показать/скрыть)"), QStringLiteral("search"));
	searchButton->setCheckable(true);
	connect(searchButton, &RibbonButton::toggled, q, [this](bool checked) {
		setSearchVisible(checked);
	});

	// Значок свойств у кнопки поиска раскрывает «Параметры поиска» в панели
	// «Свойства».
	connect(ribbon, &DocumentRibbon::propertiesRequested, q,
		[this](const QString& groupId) {
			q->showProperties(groupId);
		});

	return ribbon;
}

QWidget* CodeEditorDocument::Private::createSearchPropertiesEditor(QWidget* parent) {
	auto* properties = new PropertiesListWidget(parent);
	properties->setObjectName(QStringLiteral("CodeEditorSearchProperties"));
	properties->setHeaderVisible(false);
	properties->setScrollable(false);
	properties->setProperties(searchPropertyData());
	searchProperties = properties;

	connect(properties, &PropertiesListWidget::propertyChanged, q,
		[this](const QString& id, const QVariant& value) {
			applySearchProperty(id, value.toBool());
		});

	return properties;
}

QList<PropertyData> CodeEditorDocument::Private::searchPropertyData() const {
	QList<PropertyData> result;

	auto add = [this, &result](const QString& id, const QString& label,
		CodeEditorSearchPanel::Option option, const QString& tooltip) {
		PropertyData data;
		data.id = id;
		data.label = label;
		data.type = PropertyType::Boolean;
		data.value = searchPanel && searchPanel->option(option);
		data.tooltip = tooltip;
		result << data;
	};

	add(kSearchRegularExpression, q->tr(".*  Регулярное выражение"),
		CodeEditorSearchPanel::Option::RegularExpression,
		q->tr("Искать по регулярному выражению"));
	add(kSearchSimplifiedExpression, q->tr("\\n  Упрощённое выражение"),
		CodeEditorSearchPanel::Option::SimplifiedExpression,
		q->tr("Обычный текст, но \\n, \\t и \\r означают переводы"));
	add(kSearchCaseSensitive, q->tr("Aa  Учитывать регистр"),
		CodeEditorSearchPanel::Option::CaseSensitive,
		q->tr("Учитывать регистр символов при поиске"));
	add(kSearchWholeWord, q->tr("ab  Слово целиком"),
		CodeEditorSearchPanel::Option::WholeWord,
		q->tr("Искать только целые слова"));

	return result;
}

void CodeEditorDocument::Private::setSearchOption(CodeEditorSearchPanel::Option option,
	bool enabled) {
	if (!searchPanel) {
		return;
	}

	searchPanel->setOption(option, enabled);

	// Регулярное выражение и упрощённый режим взаимоисключающие.
	if (enabled) {
		if (option == CodeEditorSearchPanel::Option::RegularExpression) {
			searchPanel->setOption(CodeEditorSearchPanel::Option::SimplifiedExpression, false);
		}
		else if (option == CodeEditorSearchPanel::Option::SimplifiedExpression) {
			searchPanel->setOption(CodeEditorSearchPanel::Option::RegularExpression, false);
		}
	}
}

void CodeEditorDocument::Private::applySearchProperty(const QString& id, bool enabled) {
	CodeEditorSearchPanel::Option option = CodeEditorSearchPanel::Option::CaseSensitive;
	if (id == kSearchRegularExpression) {
		option = CodeEditorSearchPanel::Option::RegularExpression;
	}
	else if (id == kSearchSimplifiedExpression) {
		option = CodeEditorSearchPanel::Option::SimplifiedExpression;
	}
	else if (id == kSearchCaseSensitive) {
		option = CodeEditorSearchPanel::Option::CaseSensitive;
	}
	else if (id == kSearchWholeWord) {
		option = CodeEditorSearchPanel::Option::WholeWord;
	}
	else {
		return;
	}

	setSearchOption(option, enabled);

	// Взаимоисключающие режимы могли измениться: обновляем флажки группы.
	if (searchProperties) {
		searchProperties->setProperties(searchPropertyData());
	}
}

void CodeEditorDocument::Private::setupFontSizes() {
	if (!fontSizeCombo || !editor) {
		return;
	}

	const QList<int> sizes = {
		6, 7, 8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 28, 32, 36, 48, 72
	};

	const QSignalBlocker blocker(fontSizeCombo);
	fontSizeCombo->clear();
	for (const int size : sizes) {
		fontSizeCombo->addItem(QString::number(size), size);
	}

	const int index = fontSizeCombo->findData(editor->fontSize());
	fontSizeCombo->setCurrentIndex(index >= 0 ? index : fontSizeCombo->findData(10));
}

void CodeEditorDocument::Private::setupLanguages() {
	if (!languageCombo) {
		return;
	}

	QList<QPair<QString, QString>> items;
	if (highlighting) {
		for (const auto* plugin : highlighting->allPlugins()) {
			const auto info = plugin->languageInfo();
			if (info.mimeType.isEmpty()) {
				continue;
			}

			const auto name = info.name.isEmpty() ? info.mimeType : info.name;
			items.append({ name, info.mimeType });
		}
	}

	std::sort(items.begin(), items.end(), [](const auto& left, const auto& right) {
		return left.first.localeAwareCompare(right.first) < 0;
	});

	{
		const QSignalBlocker blocker(languageCombo);
		languageCombo->clear();
		for (const auto& item : items) {
			languageCombo->addItem(item.first, item.second);
		}
	}

	syncLanguageCombo();
}

void CodeEditorDocument::Private::syncLanguageCombo() {
	if (!languageCombo) {
		return;
	}

	const int index = languageCombo->findData(currentMimeType());
	if (index < 0 || index == languageCombo->currentIndex()) {
		return;
	}

	const QSignalBlocker blocker(languageCombo);
	languageCombo->setCurrentIndex(index);
}

void CodeEditorDocument::Private::syncFontSizeCombo(int size) {
	if (!fontSizeCombo) {
		return;
	}

	const QSignalBlocker blocker(fontSizeCombo);
	int index = fontSizeCombo->findData(size);
	if (index < 0) {
		// Размер получен колесом мыши и может отсутствовать в списке:
		// вставляем его, сохраняя числовой порядок.
		int insertAt = 0;
		for (int i = 0; i < fontSizeCombo->count(); i++) {
			if (fontSizeCombo->itemData(i).toInt() < size) {
				insertAt = i + 1;
			}
			else {
				break;
			}
		}
		fontSizeCombo->insertItem(insertAt, QString::number(size), size);
		index = insertAt;
	}

	fontSizeCombo->setCurrentIndex(index);
}

QString CodeEditorDocument::Private::currentMimeType() const {
	const auto* plugin = editor ? editor->highlightingPlugin() : nullptr;
	if (!plugin) {
		return QStringLiteral("text/plane");
	}

	const auto info = plugin->languageInfo();
	return info.mimeType.isEmpty() ? QStringLiteral("text/plane") : info.mimeType;
}

QString CodeEditorDocument::Private::currentLanguageName() const {
	const auto* plugin = editor ? editor->highlightingPlugin() : nullptr;
	if (!plugin) {
		return {};
	}

	const auto info = plugin->languageInfo();
	return info.name.isEmpty() ? info.mimeType : info.name;
}

CommandController* CodeEditorDocument::Private::controller() const {
	auto context = q->context();
	return context ? context->commandController() : nullptr;
}

void CodeEditorDocument::Private::printError(const QString& message) const {
	if (auto* c = controller()) {
		c->printError(message);
	}
}

void CodeEditorDocument::Private::printWarning(const QString& message) const {
	if (auto* c = controller()) {
		c->printWarning(message);
	}
}

bool CodeEditorDocument::Private::writeFile(const QString& path, const QString& text,
	QString* error) const {
	QFile file(path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		if (error) {
			*error = q->tr("Не удалось записать файл:\n%1\n%2")
				.arg(QDir::toNativeSeparators(path), file.errorString());
		}
		return false;
	}

	QTextStream stream(&file);
	stream.setEncoding(QStringConverter::Utf8);
	stream << text;
	stream.flush();
	if (stream.status() != QTextStream::Ok) {
		if (error) {
			*error = q->tr("Не удалось записать файл:\n%1")
				.arg(QDir::toNativeSeparators(path));
		}
		return false;
	}

	return true;
}

void CodeEditorDocument::Private::applyDataText(const QString& data) {
	editor->setText(data);
	editor->document()->setModified(false);
	q->setModified(false);
}

void CodeEditorDocument::Private::setPath(const QString& value) {
	if (path == value) {
		return;
	}

	path = value;
	// Путь — состояние документа, оно живёт в переменной контекста.
	settingPath = true;
	q->setVariable(QStringLiteral("path"), value);
	settingPath = false;
}

CodeEditorDocument::CodeEditorDocument(
	HighlightingPluginManager* highlighting,
	FormatterPluginManager* formatters,
	QWidget* parent)
	: DocumentWidget(parent)
	, d(std::make_unique<Private>(this)) {
	d->highlighting = highlighting;
	d->formatters = formatters;

	setGlyph(QStringLiteral("📝"));
	setTitle(tr("Без имени"));
	d->setupUi();

	connect(d->editor->document(), &QTextDocument::modificationChanged,
		this, &DocumentWidget::setModified);
	connect(d->editor, &CodeEditorWidget::fileOpened, this, [this](const QString& path, bool success) {
		if (!success) {
			d->printError(tr("Не удалось открыть файл:\n%1")
				.arg(QDir::toNativeSeparators(path)));
			d->setPath(QString());
			d->dataFromFile = false;
			setTitle(tr("Без имени"));
			setToolTip(QString());
			emit propertiesChanged();
			return;
		}

		d->editor->document()->setModified(false);
		setModified(false);
		d->syncLanguageCombo();
		emit propertiesChanged();
	});
	connect(d->editor, &CodeEditorWidget::formattedFailed, this, [this](const QString& message) {
		d->printWarning(message);
	});
	connect(d->editor, &CodeEditorWidget::fontSizeChanged, this, [this](int size) {
		d->syncFontSizeCombo(size);
	});
}

CodeEditorDocument::~CodeEditorDocument() = default;

CodeEditorWidget* CodeEditorDocument::editor() const {
	return d->editor;
}

QString CodeEditorDocument::filePath() const {
	return d->path;
}

void CodeEditorDocument::newFile() {
	d->setPath(QString());
	d->editor->setText(QString());
	d->editor->document()->setModified(false);
	setModified(false);
	d->dataFromFile = false;
	if (d->dataMode) {
		setVariable(QStringLiteral("data"), QString());
	}
	setTitle(tr("Без имени"));
	setToolTip(QString());
	d->syncLanguageCombo();
	emit propertiesChanged();
}

bool CodeEditorDocument::openFile(const QString& path) {
	if (path.isEmpty()) {
		return false;
	}

	const QFileInfo info(path);
	if (!info.exists() || !info.isFile() || !info.isReadable()) {
		d->printError(tr("Не удалось открыть файл:\n%1")
			.arg(QDir::toNativeSeparators(path)));
		return false;
	}

	d->setPath(path);
	d->dataFromFile = d->dataMode;
	setTitle(info.fileName());
	setToolTip(QDir::toNativeSeparators(path));
	d->editor->openFile(path);
	d->syncLanguageCombo();
	emit propertiesChanged();
	return true;
}

bool CodeEditorDocument::save() {
	const auto text = d->editor->toPlainText();
	QString path = d->path;

	// В режиме data файл не обязателен: данные сохраняются в контекст.
	if (!d->dataMode && path.isEmpty()) {
		path = QFileDialog::getSaveFileName(this, tr("Сохранить файл"),
			QDir::current().filePath(title() + QStringLiteral(".txt")),
			tr("Все файлы (*.*)"));
		if (path.isEmpty()) {
			return false;
		}
	}

	if (!path.isEmpty()) {
		QString error;
		if (!d->writeFile(path, text, &error)) {
			d->printError(error);
			return false;
		}

		d->setPath(path);
		d->editor->setHoghlighterByPath(path);
		setTitle(QFileInfo(path).fileName());
		setToolTip(QDir::toNativeSeparators(path));
	}

	if (d->dataMode && !d->dataFromFile) {
		setVariable(QStringLiteral("data"), text);
	}

	d->editor->document()->setModified(false);
	setModified(false);
	d->syncLanguageCombo();

	if (auto context = this->context()) {
		context->fireEvent(CommandContext::documentSaveCompleteEvent());
	}
	emit propertiesChanged();
	return true;
}

void CodeEditorDocument::refresh() {
	if (d->dataMode) {
		return;
	}

	if (d->path.isEmpty() || isModified()) {
		return;
	}

	if (!QFileInfo::exists(d->path)) {
		d->printWarning(tr("Файл не найден:\n%1")
			.arg(QDir::toNativeSeparators(d->path)));
		return;
	}

	d->editor->openFile(d->path);
	d->syncLanguageCombo();
}

void CodeEditorDocument::onContextAttached() {
	const auto path = variable(QStringLiteral("path"), QString()).toString();
	const bool hasData = hasVariable(QStringLiteral("data"));
	d->dataMode = hasData;

	if (hasData) {
		const auto data = variable(QStringLiteral("data")).toString();
		if (path.isEmpty()) {
			d->dataFromFile = false;
			d->applyDataText(data);
			setTitle(tr("Данные"));
			setToolTip(QString());
			d->syncLanguageCombo();
			emit propertiesChanged();
			return;
		}

		const QFileInfo info(path);
		if (info.exists() && info.isFile()) {
			// Источник истины — файл; data не трогаем, но предупреждаем.
			d->dataFromFile = true;
			d->printWarning(tr("Есть и файл, и data: данные открыты из файла; "
				"переменная data не изменяется"));
			QMetaObject::invokeMethod(this, [this, path] {
				openFile(path);
			}, Qt::QueuedConnection);
			return;
		}

		// Файла ещё нет: данные берём из data, путь нужен для сохранения.
		d->dataFromFile = false;
		d->applyDataText(data);
		d->setPath(path);
		setTitle(info.fileName());
		setToolTip(QDir::toNativeSeparators(path));
		d->syncLanguageCombo();
		emit propertiesChanged();
		return;
	}

	d->dataFromFile = false;
	if (path.isEmpty()) {
		return;
	}

	// Заголовок задаётся при создании уже после привязки контекста,
	// поэтому открытие отложено: openFile() уточнит заголовок по файлу.
	QMetaObject::invokeMethod(this, [this, path] {
		openFile(path);
	}, Qt::QueuedConnection);
}

void CodeEditorDocument::onContextVariableChanged(const QString& name, const QVariant& value) {
	if (d->settingPath) {
		return;
	}

	if (name == QLatin1String("data") && d->dataMode) {
		if (isModified()) {
			return;
		}

		const auto data = value.toString();
		if (d->editor->toPlainText() == data) {
			return;
		}

		d->dataFromFile = false;
		d->applyDataText(data);
		emit propertiesChanged();
		return;
	}

	if (name != QLatin1String("path")) {
		return;
	}

	const auto path = value.toString();
	if (path == d->path) {
		return;
	}

	if (path.isEmpty()) {
		newFile();
		return;
	}

	openFile(path);
}

QWidget* CodeEditorDocument::createPropertiesEditor(QWidget* parent) {
	QWidget* widget = DocumentWidget::createPropertiesEditor(parent);
	auto* form = qobject_cast<QFormLayout*>(widget->layout());
	if (!form) {
		return widget;
	}

	auto* pathLabel = new QLabel(d->path.isEmpty() ? tr("<не сохранён>")
		: QDir::toNativeSeparators(d->path), widget);
	pathLabel->setWordWrap(true);
	pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

	auto* languageCombo = new QComboBox(widget);
	languageCombo->setObjectName(QStringLiteral("CodeEditorLanguageCombo"));
	languageCombo->setToolTip(tr("Язык подсветки документа"));
	d->languageCombo = languageCombo;
	d->setupLanguages();
	connect(languageCombo, &QComboBox::currentIndexChanged, this,
		[this, languageCombo](int index) {
			if (index < 0) {
				return;
			}

			const auto mimeType = languageCombo->itemData(index).toString();
			if (!mimeType.isEmpty() && !d->editor->setContentType(mimeType)) {
				qWarning() << "CodeEditorDocument: highlighting plugin is not found for"
					<< mimeType;
			}
			d->syncLanguageCombo();
		});

	auto* fontSizeCombo = new QComboBox(widget);
	fontSizeCombo->setObjectName(QStringLiteral("CodeEditorFontSizeCombo"));
	fontSizeCombo->setToolTip(tr("Размер шрифта редактора"));
	fontSizeCombo->setFixedWidth(kFontSizeComboWidth);
	d->fontSizeCombo = fontSizeCombo;
	d->setupFontSizes();
	connect(fontSizeCombo, &QComboBox::currentIndexChanged, this,
		[this, fontSizeCombo](int index) {
			if (index >= 0) {
				d->editor->setFontSize(fontSizeCombo->itemData(index).toInt());
			}
		});

	auto* linesLabel = new QLabel(QString::number(d->editor->document()->blockCount()), widget);
	connect(d->editor->document(), &QTextDocument::blockCountChanged,
		linesLabel, [linesLabel](int count) {
			linesLabel->setNum(count);
		});

	auto* autoWrap = new QCheckBox(tr("Автоперенос длинных строк"), widget);
	autoWrap->setObjectName(QStringLiteral("CodeEditorAutoWrapCheck"));
	autoWrap->setToolTip(tr("Длинные строки (от %1 символов) переносятся по ширине окна: "
		"без переноса редактор заметно тормозит на них при изменении размера и прокрутке")
		.arg(CodeEditorWidget::autoWrapLineLimit()));
	autoWrap->setChecked(d->editor->isAutoWrapEnabled());
	connect(autoWrap, &QCheckBox::toggled, d->editor, &CodeEditorWidget::setAutoWrapEnabled);

	form->insertRow(0, tr("Файл:"), pathLabel);
	form->addRow(tr("Язык:"), languageCombo);
	form->addRow(tr("Размер шрифта:"), fontSizeCombo);
	form->addRow(tr("Перенос строк:"), autoWrap);
	form->addRow(tr("Строк:"), linesLabel);
	return widget;
}

QList<DocumentPropertyGroup> CodeEditorDocument::createPropertyGroups(QWidget* parent) {
	DocumentPropertyGroup search;
	search.id = QStringLiteral("search");
	search.title = tr("Параметры поиска");
	search.editor = d->createSearchPropertiesEditor(parent);
	return { search };
}
