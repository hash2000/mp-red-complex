#include "Content/TextEditorModule/documents/code_editor_document.h"
#include "Content/TextEditorModule/documents/text_document_options.h"
#include "Libs/BaseWidgets/editor/widgets/code_editor_widget.h"
#include "Libs/BaseWidgets/editor/widgets/code_editor_search_panel.h"
#include "Libs/BaseWidgets/editor/widgets/encoding_dialog.h"
#include "Libs/BaseWidgets/editor/widgets/editor_settings_builder.h"
#include "Libs/BaseWidgets/editor/language_info.h"
#include "Libs/BaseWidgets/editor/highlights/highlighter_plugin.h"
#include "Libs/BaseWidgets/editor/highlights/highlighter_plugin_manager.h"
#include "Libs/BaseWidgets/editor/formatters/formatter_plugin_manager.h"
#include "Libs/BaseWidgets/properties/properties_list_widget.h"
#include "Libs/BaseWidgets/ribbon/document_ribbon.h"
#include "Libs/BaseWidgets/ribbon/ribbon_button.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"

#include <QComboBox>
#include <QCompleter>
#include <QDebug>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPair>
#include <QPointer>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTextBlock>
#include <QTextCodec>
#include <QTextDocument>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

namespace {
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
	RibbonButton* highlightButton = nullptr;

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
	QWidget* createInfoWidget(QWidget* parent);

	/// Диалог выбора кодировки: переоткрыть или пересохранить файл.
	void showEncodingDialog();
	/// Подпись кодировки для пользователя по имени кодека.
	QString encodingTitle(const QString& encoding) const;

	/// Перекодировать данные документа (не файла): из data-bytes либо из текста.
	bool applyDataEncoding(const QString& encoding);
	/// Записать перекодированный текст в data (и data-bytes), если он есть.
	bool persistData(const QString& encoding);
	QList<PropertyData> searchPropertyData() const;
	void applySearchProperty(const QString& id, bool enabled);
	void setSearchOption(CodeEditorSearchPanel::Option option, bool enabled);

	void setupLanguages();
	void syncLanguageCombo();
	void syncFontSizeCombo(int size);

	QString currentMimeType() const;
	QString currentLanguageName() const;
	void setPath(const QString& path);
	CommandController* controller() const;
	void printError(const QString& message) const;
	void printWarning(const QString& message) const;
	void printInfo(const QString& message) const;
	bool writeFile(const QString& path, const QString& text, QString* error) const;
	void applyDataText(const QString& data);
	/// Применить глобальные параметры текстовых документов как значения
	/// по умолчанию (шрифт, перенос строк, кодировка).
	void applyTextDefaults();
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

	// Ручное включение/выключение подсветки. Приоритет у кнопки: даже при
	// длинных строках подсветка включается (с предупреждением о тормозах),
	// пока пользователь не сменит документ или сам режим.
	highlightButton = ribbon->addButton(editGroup, QStringLiteral("🎨"),
		q->tr("Подсветка текста (включить/выключить)"));
	highlightButton->setObjectName(QStringLiteral("CodeEditorHighlightButton"));
	highlightButton->setCheckable(true);
	highlightButton->setChecked(editor->isHighlightingEnabled());
	connect(highlightButton, &RibbonButton::toggled, q, [this](bool checked) {
		editor->setHighlightingPolicy(checked
			? CodeEditorWidget::HighlightingPolicy::On
			: CodeEditorWidget::HighlightingPolicy::Off);
	});
	// Авто-режим мог выключить подсветку из-за длинной строки: кнопка
	// должна показывать фактическое состояние.
	connect(editor, &CodeEditorWidget::highlightingEnabledChanged,
		q, [this](bool enabled) {
			if (!highlightButton) {
				return;
			}

			const QSignalBlocker blocker(highlightButton);
			highlightButton->setChecked(enabled);
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

void CodeEditorDocument::Private::printInfo(const QString& message) const {
	if (auto* c = controller()) {
		c->print(message);
	}
}

bool CodeEditorDocument::Private::writeFile(const QString& path, const QString& text,
	QString* error) const {
	QTextCodec* codec = QTextCodec::codecForName(editor->encoding().toLatin1());
	if (!codec) {
		codec = QTextCodec::codecForName("UTF-8");
	}
	if (!codec) {
		if (error) {
			*error = q->tr("Не удалось подобрать кодировку для сохранения файла:\n%1")
				.arg(QDir::toNativeSeparators(path));
		}
		return false;
	}

	QFile file(path);
	if (!file.open(QIODevice::WriteOnly)) {
		if (error) {
			*error = q->tr("Не удалось записать файл:\n%1\n%2")
				.arg(QDir::toNativeSeparators(path), file.errorString());
		}
		return false;
	}

	// Кодировка задаётся QTextCodec: QTextStream в Qt6 больше не принимает кодеки.
	// Как и раньше, на Windows сохраняем с CRLF.
	QString content = text;
#ifdef Q_OS_WIN
	content.replace(QLatin1String("\n"), QLatin1String("\r\n"));
#endif

	const QByteArray encoded = codec->fromUnicode(content);
	if (file.write(encoded) != encoded.size()) {
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

void CodeEditorDocument::Private::applyTextDefaults() {
	const TextDocumentOptions options = TextDocumentOptions::fromContext(q->context());

	QFont font = editor->font();
	if (!options.fontFamily.isEmpty()) {
		font.setFamily(options.fontFamily);
	}
	if (options.fontSize > 0) {
		font.setPointSize(options.fontSize);
	}
	editor->setFont(font);

	editor->setLineWrapPolicy(options.wordWrap
		? CodeEditorWidget::LineWrapPolicy::On
		: CodeEditorWidget::LineWrapPolicy::Off);

	if (!options.encoding.isEmpty()) {
		editor->setEncoding(options.encoding);
	}
}

bool CodeEditorDocument::Private::applyDataEncoding(const QString& encoding) {
	QTextCodec* target = QTextCodec::codecForName(encoding.toLatin1());
	if (!target) {
		printWarning(q->tr("Неизвестная кодировка: %1").arg(encoding));
		return false;
	}

	// Если в контексте сохранены исходные байты (data-bytes), декодируем их:
	// это единственный способ без потерь переоткрыть данные в другой кодировке.
	const QVariant bytes = q->variable(QStringLiteral("data-bytes"), QVariant());
	QString text;
	if (bytes.isValid() && !bytes.toByteArray().isEmpty()) {
		text = target->toUnicode(bytes.toByteArray());
	}
	else {
		QTextCodec* source = QTextCodec::codecForName(editor->encoding().toLatin1());
		if (!source) {
			source = QTextCodec::codecForName("UTF-8");
		}
		if (encoding == editor->encoding()) {
			return true;
		}

		// Байтов нет: переинтерпретируем текст из текущей кодировки в выбранную.
		text = target->toUnicode(source->fromUnicode(editor->toPlainText()));
	}

	editor->setEncoding(encoding);
	applyDataText(text);
	emit q->propertiesChanged();
	return true;
}

bool CodeEditorDocument::Private::persistData(const QString& encoding) {
	if (!applyDataEncoding(encoding)) {
		return false;
	}

	QTextCodec* codec = QTextCodec::codecForName(encoding.toLatin1());
	const QString text = editor->toPlainText();
	q->setVariable(QStringLiteral("data"), text);
	if (codec) {
		q->setVariable(QStringLiteral("data-bytes"), codec->fromUnicode(text));
	}

	if (auto context = q->context()) {
		context->fireEvent(CommandContext::documentSaveCompleteEvent());
	}
	emit q->propertiesChanged();
	return true;
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

	// Подсветка включается и выключается автоматически (длинная строка)
	// или вручную кнопкой ленты. Пользователь должен знать о причинах.
	connect(d->editor, &CodeEditorWidget::highlightingEnabledChanged,
		this, [this](bool enabled) {
			const auto policy = d->editor->highlightingPolicy();
			const bool longLines = d->editor->hasLongLines();

			if (!enabled) {
				// Ручное выключение — это осознанное действие, молчим.
				if (policy == CodeEditorWidget::HighlightingPolicy::Auto && longLines) {
					d->printWarning(tr("Подсветка отключена: в документе есть строка длиннее "
						"%1 символов, подсветка таких строк подвешивает редактор. "
						"Она включится снова, когда строки станут короче")
						.arg(CodeEditorWidget::maxHighlightedLineLength()));
				}
				return;
			}

			if (policy == CodeEditorWidget::HighlightingPolicy::On && longLines) {
				d->printWarning(tr("Подсветка включена вручную. Строки длиннее %1 символов "
					"не подсвечиваются (иначе редактор может зависнуть), на длинных "
					"строках возможны замедления")
					.arg(CodeEditorWidget::maxHighlightedLineLength()));
				return;
			}

			d->printInfo(tr("Подсветка снова включена."));
		});

	// Пользователь выбрал «без переноса», а в документе есть длинные строки:
	// уважаем выбор, но предупреждаем о возможных тормозах.
	connect(d->editor, &CodeEditorWidget::longLinesChanged, this, [this](bool longLines) {
		if (!longLines
			|| d->editor->lineWrapPolicy() != CodeEditorWidget::LineWrapPolicy::Off) {
			return;
		}

		d->printWarning(tr("В документе есть строка длиннее %1 символов, а перенос строк "
			"выключен: редактор может заметно тормозить. Включите «Авто» или «Включён», "
			"если это произойдёт")
			.arg(CodeEditorWidget::autoWrapLineLimit()));
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

bool CodeEditorDocument::reopenWithEncoding(const QString& encoding) {
	if (!d->editor) {
		return false;
	}

	// Файл — источник истины: перечитываем его байты в выбранной кодировке.
	if (!d->path.isEmpty() && QFileInfo::exists(d->path)) {
		d->editor->setEncoding(encoding);
		return openFile(d->path);
	}

	// Файла нет (данные из контекста или буфер): перекодируем содержимое.
	return d->applyDataEncoding(encoding);
}

bool CodeEditorDocument::saveWithEncoding(const QString& encoding) {
	if (!d->editor) {
		return false;
	}

	if (!d->path.isEmpty() && QFileInfo::exists(d->path)) {
		d->editor->setEncoding(encoding);
		return save();
	}

	if (d->dataMode) {
		// Путь может быть задан, но файла ещё нет — save() создаст его.
		if (!d->path.isEmpty()) {
			d->editor->setEncoding(encoding);
			return save();
		}

		// Чистые данные: результат перекодирования сохраняем в контекст.
		return d->persistData(encoding);
	}

	d->printWarning(tr("Пересохранение в выбранной кодировке доступно только для файла: "
		"сначала сохраните файл."));
	return false;
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
	// Глобальные параметры текста — значения по умолчанию для нового
	// документа: шрифт, перенос строк и кодировка применяются до чтения файла.
	d->applyTextDefaults();

	const auto path = variable(QStringLiteral("path"), QString()).toString();
	const bool hasData = hasVariable(QStringLiteral("data"));
	d->dataMode = hasData;

	if (hasData) {
		const auto data = variable(QStringLiteral("data")).toString();
		if (path.isEmpty()) {
			d->dataFromFile = false;
			d->applyDataText(data);
			setTitle(tr("Данные"));
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

	// Текущая кодировка и кнопка выбора: сам выбор — в отдельном диалоге,
	// где можно переоткрыть файл или пересохранить его в новой кодировке.
	auto* encodingRow = new QWidget(widget);
	auto* encodingLayout = new QHBoxLayout(encodingRow);
	encodingLayout->setContentsMargins(0, 0, 0, 0);

	auto* encodingLabel = new QLabel(d->encodingTitle(d->editor->encoding()), encodingRow);
	encodingLabel->setObjectName(QStringLiteral("CodeEditorEncodingLabel"));
	auto* encodingButton = new QToolButton(encodingRow);
	encodingButton->setObjectName(QStringLiteral("CodeEditorEncodingButton"));
	encodingButton->setText(QStringLiteral("…"));
	encodingButton->setToolTip(tr("Выбрать кодировку документа"));
	encodingLayout->addWidget(encodingLabel, 1);
	encodingLayout->addWidget(encodingButton);

	connect(encodingButton, &QToolButton::clicked, this, [this] {
		d->showEncodingDialog();
	});
	connect(d->editor, &CodeEditorWidget::encodingChanged, encodingLabel,
		[this, encodingLabel](const QString& encoding) {
			encodingLabel->setText(d->encodingTitle(encoding));
		});

	form->insertRow(0, tr("Файл:"), pathLabel);
	form->addRow(tr("Язык:"), languageCombo);
	form->addRow(tr("Кодировка:"), encodingRow);

	// Общие параметры редактора (шрифт, перенос) строит общий билдер.
	EditorSettingsBuilder settings(widget);
	d->fontSizeCombo = settings.addFontSizeRow(form, d->editor->fontSize(),
		[this](int size) {
			d->editor->setFontSize(size);
		});
	settings.addLineWrapRow(form,
		static_cast<int>(d->editor->lineWrapPolicy()), /*allowAuto*/ true,
		[this](int value) {
			const auto policy = static_cast<CodeEditorWidget::LineWrapPolicy>(value);
			d->editor->setLineWrapPolicy(policy);

			// Выбор пользователя важнее: перенос не включаем насильно,
			// но предупреждаем о возможных тормозах.
			if (policy == CodeEditorWidget::LineWrapPolicy::Off
				&& d->editor->hasLongLines()) {
				d->printWarning(tr("Перенос строк выключен. На строках длиннее %1 символов "
					"редактор может заметно тормозить; включите «Авто» или «Включён», "
					"если появятся зависания")
					.arg(CodeEditorWidget::autoWrapLineLimit()));
			}
		});
	return widget;
}

QWidget* CodeEditorDocument::Private::createInfoWidget(QWidget* parent) {
	auto* widget = new QWidget(parent);
	auto* form = new QFormLayout(widget);
	form->setContentsMargins(0, 0, 0, 0);

	auto* lines = new QLabel(widget);
	lines->setObjectName(QStringLiteral("CodeEditorInfoLines"));
	auto* characters = new QLabel(widget);
	characters->setObjectName(QStringLiteral("CodeEditorInfoChars"));
	auto* longest = new QLabel(widget);
	longest->setObjectName(QStringLiteral("CodeEditorInfoLongest"));

	form->addRow(tr("Строк:"), lines);
	form->addRow(tr("Символов:"), characters);
	form->addRow(tr("Самая длинная строка:"), longest);

	// Считаем через QPointer: панель свойств может пережить документ.
	QPointer<CodeEditorWidget> editorPtr(editor);
	auto update = [editorPtr, lines, characters, longest] {
		if (!editorPtr) {
			return;
		}

		QTextDocument* document = editorPtr->document();
		lines->setNum(document->blockCount());
		characters->setText(QString::number(
			qMax<qint64>(0, document->characterCount() - 1)));
		// Длина строки берётся из метрик: для больших файлов они посчитаны
		// при чтении, обход всех блоков на каждую правку не нужен.
		longest->setNum(editorPtr->documentMetrics().maxLineLength);
	};

	update();

	if (auto* document = editor->document()) {
		connect(document, &QTextDocument::blockCountChanged, widget,
			[update](int) { update(); });

		// Содержимое меняется на каждое нажатие — пересчитываем с задержкой.
		auto* timer = new QTimer(widget);
		timer->setSingleShot(true);
		timer->setInterval(150);
		connect(timer, &QTimer::timeout, widget, update);
		connect(document, &QTextDocument::contentsChanged, widget, [timer] {
			timer->start();
		});
	}

	return widget;
}

QString CodeEditorDocument::Private::encodingTitle(const QString& encoding) const {
	for (const auto& info : CodeEditorWidget::availableEncodings()) {
		if (info.name == encoding) {
			return info.title;
		}
	}
	return encoding;
}

void CodeEditorDocument::Private::showEncodingDialog() {
	const bool hasFile = !path.isEmpty() && QFileInfo::exists(path);
	const bool hasBytes = q->variable(QStringLiteral("data-bytes"), QVariant())
		.toByteArray().size() > 0;
	QString hint;
	if (hasFile) {
		hint = q->tr("Файл: %1").arg(QDir::toNativeSeparators(path));
	}
	else if (hasBytes) {
		hint = q->tr("Данные будут декодированы из сохранённых байтов (data-bytes). "
			"«Пересохранить» запишет результат в переменную data.");
	}
	else if (dataMode) {
		hint = q->tr("Файл не задан: текст будет перекодирован из текущей кодировки. "
			"«Пересохранить» запишет результат в переменную data.");
	}
	else {
		hint = q->tr("Файл не задан: текст будет перекодирован из текущей кодировки "
			"в выбранную.");
	}

	EncodingDialog dialog(editor->encoding(), hint, /*allowResave*/ true, q);
	if (dialog.exec() != QDialog::Accepted) {
		return;
	}

	const QString encoding = dialog.selectedEncoding();
	if (encoding.isEmpty()) {
		return;
	}

	if (dialog.action() == EncodingDialog::Action::Resave) {
		q->saveWithEncoding(encoding);
		return;
	}

	if (q->isModified()) {
		const auto answer = QMessageBox::question(q, q->tr("Переоткрыть файл"),
			q->tr("В файле есть несохранённые изменения: при перечитывании они будут "
				"потеряны.\nПереоткрыть файл в выбранной кодировке?"),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
		if (answer != QMessageBox::Yes) {
			return;
		}
	}

	q->reopenWithEncoding(encoding);
}

QList<DocumentPropertyGroup> CodeEditorDocument::createPropertyGroups(QWidget* parent) {
	DocumentPropertyGroup info;
	info.id = QStringLiteral("info");
	info.title = tr("Информация о документе");
	info.expanded = true;
	info.editor = d->createInfoWidget(parent);

	DocumentPropertyGroup search;
	search.id = QStringLiteral("search");
	search.title = tr("Параметры поиска");
	search.editor = d->createSearchPropertiesEditor(parent);

	return { info, search };
}
