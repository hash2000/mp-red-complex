#include "Content/CodeEditorWidget/documents/code_editor_document.h"
#include "Content/CodeEditorWidget/widgets/code_editor_widget.h"
#include "Content/CodeEditorWidget/widgets/code_editor_search_panel.h"
#include "Content/CodeEditorWidget/language_info.h"
#include "Content/CodeEditorWidget/highlights/highlighter_plugin.h"
#include "Content/CodeEditorWidget/highlights/highlighter_plugin_manager.h"
#include "Content/CodeEditorWidget/formatters/formatter_plugin_manager.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"

#include <QComboBox>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPair>
#include <QSignalBlocker>
#include <QStringConverter>
#include <QTextDocument>
#include <QTextStream>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

namespace {
constexpr int kRibbonButtonSize = 30;
constexpr int kFontSizeComboWidth = 76;
constexpr int kLanguageComboWidth = 150;
}

class CodeEditorDocument::Private {
public:
	struct RibbonGroup {
		QWidget* widget = nullptr;
		QHBoxLayout* content = nullptr;
		QLabel* caption = nullptr;
	};

	Private(CodeEditorDocument* parent) : q(parent) {}
	CodeEditorDocument* q;

	CodeEditorWidget* editor = nullptr;
	HighlightingPluginManager* highlighting = nullptr;
	FormatterPluginManager* formatters = nullptr;

	QWidget* ribbon = nullptr;
	QWidget* ribbonGroups = nullptr;
	QToolButton* ribbonToggle = nullptr;
	QComboBox* fontSizeCombo = nullptr;
	QComboBox* languageCombo = nullptr;

	CodeEditorSearchPanel* searchPanel = nullptr;
	QToolButton* searchButton = nullptr;
	QToolButton* regexButton = nullptr;
	QToolButton* simplifiedButton = nullptr;
	QToolButton* caseButton = nullptr;
	QToolButton* wholeWordButton = nullptr;

	QString path;
	bool dataMode = false;
	bool dataFromFile = false;
	bool settingPath = false;

	void setupUi();
	QWidget* createRibbon();
	RibbonGroup addGroup(QHBoxLayout* ribbonLayout, const QString& title);
	QToolButton* addButton(RibbonGroup& group, const QString& glyph, const QString& tooltip);
	void setupFontSizes();
	void setupLanguages();
	void syncLanguageCombo();
	void setRibbonVisible(bool visible);
	void updateRibbonState();
	QWidget* createSearchPanel();
	void setSearchVisible(bool visible);
	void setSearchOption(CodeEditorSearchPanel::Option option, bool enabled);
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
	editor->setLineWrapMode(QTextEdit::NoWrap);
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
		if (searchButton) {
			searchButton->setChecked(false);
		}
		else {
			setSearchVisible(false);
		}
	});

	return searchPanel;
}

void CodeEditorDocument::Private::setSearchVisible(bool visible) {
	if (!searchPanel) {
		return;
	}

	if (visible) {
		searchPanel->focusSearch();
		return;
	}

	searchPanel->clearSearch();
	searchPanel->hide();
	editor->setFocus();
}

void CodeEditorDocument::Private::setSearchOption(CodeEditorSearchPanel::Option option, bool enabled) {
	if (searchPanel) {
		searchPanel->setOption(option, enabled);
	}
}

QWidget* CodeEditorDocument::Private::createRibbon() {
	ribbon = new QWidget;
	ribbon->setObjectName(QStringLiteral("CodeEditorRibbon"));
	ribbon->setAttribute(Qt::WA_StyledBackground);

	auto* row = new QHBoxLayout(ribbon);
	row->setContentsMargins(6, 4, 4, 4);
	row->setSpacing(6);

	ribbonGroups = new QWidget(ribbon);
	auto* groupsLayout = new QHBoxLayout(ribbonGroups);
	groupsLayout->setContentsMargins(0, 0, 0, 0);
	groupsLayout->setSpacing(6);

	auto fileGroup = addGroup(groupsLayout, q->tr("Файл"));
	connect(addButton(fileGroup, QStringLiteral("🗋"), q->tr("Новый файл")),
		&QToolButton::clicked, q, [this] {
			q->newFile();
		});
	connect(addButton(fileGroup, QStringLiteral("📂"), q->tr("Открыть файл")),
		&QToolButton::clicked, q, [this] {
			const auto path = QFileDialog::getOpenFileName(q, q->tr("Открыть файл"),
				QDir::currentPath(), q->tr("Все файлы (*.*)"));
			if (!path.isEmpty()) {
				q->openFile(path);
			}
		});
	connect(addButton(fileGroup, QStringLiteral("💾"), q->tr("Сохранить файл")),
		&QToolButton::clicked, q, [this] {
			q->save();
		});

	auto textGroup = addGroup(groupsLayout, q->tr("Текст"));
	fontSizeCombo = new QComboBox(textGroup.widget);
	fontSizeCombo->setObjectName(QStringLiteral("CodeEditorRibbonCombo"));
	fontSizeCombo->setFixedWidth(kFontSizeComboWidth);
	setupFontSizes();
	connect(fontSizeCombo, &QComboBox::currentIndexChanged, q, [this](int index) {
		if (index >= 0) {
			editor->setFontSize(fontSizeCombo->itemData(index).toInt());
		}
	});
	textGroup.content->addWidget(fontSizeCombo);

	auto languageGroup = addGroup(groupsLayout, q->tr("Язык"));
	languageCombo = new QComboBox(languageGroup.widget);
	languageCombo->setObjectName(QStringLiteral("CodeEditorRibbonCombo"));
	languageCombo->setMinimumWidth(kLanguageComboWidth);
	languageCombo->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	setupLanguages();
	connect(languageCombo, &QComboBox::currentIndexChanged, q, [this](int index) {
		if (index < 0) {
			return;
		}

		const auto mimeType = languageCombo->itemData(index).toString();
		if (!mimeType.isEmpty() && !editor->setContentType(mimeType)) {
			qWarning() << "CodeEditorDocument: highlighting plugin is not found for" << mimeType;
		}
		updateRibbonState();
		emit q->propertiesChanged();
	});
	languageGroup.content->addWidget(languageCombo);

	auto formatGroup = addGroup(groupsLayout, q->tr("Формат"));
	connect(addButton(formatGroup, QStringLiteral("✨"), q->tr("Форматировать документ")),
		&QToolButton::clicked, q, [this] {
			if (!editor->formatDocument()) {
				printWarning(q->tr("Для текущего языка форматирование недоступно."));
			}
		});

	auto searchGroup = addGroup(groupsLayout, q->tr("Поиск"));
	searchButton = addButton(searchGroup, QStringLiteral("🔍"), q->tr("Поиск (показать/скрыть)"));
	searchButton->setCheckable(true);
	connect(searchButton, &QToolButton::toggled, q, [this](bool checked) {
		setSearchVisible(checked);
	});

	auto optionsGroup = addGroup(groupsLayout, q->tr("Опции поиска"));

	regexButton = addButton(optionsGroup, QStringLiteral(".*"),
		q->tr("Регулярное выражение"));
	regexButton->setCheckable(true);
	connect(regexButton, &QToolButton::toggled, q, [this](bool checked) {
		setSearchOption(CodeEditorSearchPanel::Option::RegularExpression, checked);
		// Регулярное выражение и упрощённый режим взаимоисключающие.
		if (checked && simplifiedButton && simplifiedButton->isChecked()) {
			simplifiedButton->setChecked(false);
		}
	});

	simplifiedButton = addButton(optionsGroup, QStringLiteral("\\n"),
		q->tr("Упрощённое выражение: \\n, \\t, \\r"));
	simplifiedButton->setCheckable(true);
	connect(simplifiedButton, &QToolButton::toggled, q, [this](bool checked) {
		setSearchOption(CodeEditorSearchPanel::Option::SimplifiedExpression, checked);
		if (checked && regexButton && regexButton->isChecked()) {
			regexButton->setChecked(false);
		}
	});

	caseButton = addButton(optionsGroup, QStringLiteral("Aa"), q->tr("Учитывать регистр"));
	caseButton->setCheckable(true);
	connect(caseButton, &QToolButton::toggled, q, [this](bool checked) {
		setSearchOption(CodeEditorSearchPanel::Option::CaseSensitive, checked);
	});

	wholeWordButton = addButton(optionsGroup, QStringLiteral("ab"), q->tr("Слово целиком"));
	wholeWordButton->setCheckable(true);
	connect(wholeWordButton, &QToolButton::toggled, q, [this](bool checked) {
		setSearchOption(CodeEditorSearchPanel::Option::WholeWord, checked);
	});

	ribbonGroups->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
	row->addWidget(ribbonGroups, 0, Qt::AlignLeft | Qt::AlignTop);
	row->addStretch(1);

	// Небольшая кнопка в правом углу ленты: свернуть или показать её.
	ribbonToggle = new QToolButton(ribbon);
	ribbonToggle->setObjectName(QStringLiteral("CodeEditorRibbonToggle"));
	ribbonToggle->setFixedSize(20, 20);
	ribbonToggle->setCursor(Qt::PointingHandCursor);
	ribbonToggle->setFocusPolicy(Qt::NoFocus);
	connect(ribbonToggle, &QToolButton::clicked, q, [this] {
		setRibbonVisible(!ribbonGroups->isVisible());
	});
	row->addWidget(ribbonToggle, 0, Qt::AlignRight | Qt::AlignTop);

	setRibbonVisible(true);
	return ribbon;
}

void CodeEditorDocument::Private::setRibbonVisible(bool visible) {
	if (!ribbonGroups || !ribbonToggle) {
		return;
	}

	ribbonGroups->setVisible(visible);
	ribbonToggle->setText(visible ? QStringLiteral("⌃") : QStringLiteral("⌄"));
	ribbonToggle->setToolTip(visible ? q->tr("Скрыть панель") : q->tr("Показать панель"));
}

CodeEditorDocument::Private::RibbonGroup CodeEditorDocument::Private::addGroup(
	QHBoxLayout* ribbonLayout, const QString& title) {
	RibbonGroup group;

	group.widget = new QFrame;
	group.widget->setObjectName(QStringLiteral("CodeEditorRibbonGroup"));
	group.widget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);

	auto* layout = new QVBoxLayout(group.widget);
	layout->setContentsMargins(6, 4, 6, 2);
	layout->setSpacing(2);

	group.content = new QHBoxLayout;
	group.content->setContentsMargins(0, 0, 0, 0);
	group.content->setSpacing(4);

	group.caption = new QLabel(title.toUpper(), group.widget);
	group.caption->setObjectName(QStringLiteral("CodeEditorRibbonCaption"));
	group.caption->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

	layout->addLayout(group.content, 1);
	layout->addWidget(group.caption, 0, Qt::AlignHCenter);
	ribbonLayout->addWidget(group.widget);

	return group;
}

QToolButton* CodeEditorDocument::Private::addButton(RibbonGroup& group, const QString& glyph,
	const QString& tooltip) {
	auto* button = new QToolButton(group.widget);
	button->setObjectName(QStringLiteral("CodeEditorRibbonButton"));
	button->setText(glyph);
	button->setToolTip(tooltip);
	button->setToolButtonStyle(Qt::ToolButtonTextOnly);
	button->setFixedSize(kRibbonButtonSize, kRibbonButtonSize);
	button->setCursor(Qt::PointingHandCursor);
	button->setFocusPolicy(Qt::NoFocus);
	group.content->addWidget(button);
	return button;
}

void CodeEditorDocument::Private::setupFontSizes() {
	const QList<int> sizes = {
		6, 7, 8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 28, 32, 36, 48, 72
	};

	const QSignalBlocker blocker(fontSizeCombo);
	for (const int size : sizes) {
		fontSizeCombo->addItem(QString::number(size), size);
	}

	const int index = fontSizeCombo->findData(editor->fontSize());
	fontSizeCombo->setCurrentIndex(index >= 0 ? index : fontSizeCombo->findData(10));
}

void CodeEditorDocument::Private::setupLanguages() {
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

	const QSignalBlocker blocker(languageCombo);
	for (const auto& item : items) {
		languageCombo->addItem(item.first, item.second);
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

void CodeEditorDocument::Private::updateRibbonState() {
	if (fontSizeCombo) {
		fontSizeCombo->setToolTip(q->tr("Размер шрифта: %1 pt").arg(editor->fontSize()));
	}

	if (languageCombo) {
		languageCombo->setToolTip(q->tr("Язык подсветки: %1").arg(currentLanguageName()));
	}
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
		d->updateRibbonState();
		emit propertiesChanged();
	});
	connect(d->editor, &CodeEditorWidget::formattedFailed, this, [this](const QString& message) {
		d->printWarning(message);
	});
	connect(d->editor, &CodeEditorWidget::fontSizeChanged, this, [this](int size) {
		const QSignalBlocker blocker(d->fontSizeCombo);
		int index = d->fontSizeCombo->findData(size);
		if (index < 0) {
			// Размер получен колесом мыши и может отсутствовать в списке:
			// вставляем его, сохраняя числовой порядок.
			int insertAt = 0;
			for (int i = 0; i < d->fontSizeCombo->count(); i++) {
				if (d->fontSizeCombo->itemData(i).toInt() < size) {
					insertAt = i + 1;
				}
				else {
					break;
				}
			}
			d->fontSizeCombo->insertItem(insertAt, QString::number(size), size);
			index = insertAt;
		}
		d->fontSizeCombo->setCurrentIndex(index);
		d->updateRibbonState();
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
	d->updateRibbonState();
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
	d->updateRibbonState();
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
	d->updateRibbonState();

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
	d->updateRibbonState();
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
			d->updateRibbonState();
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
		d->updateRibbonState();
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

	auto* languageLabel = new QLabel(d->currentLanguageName(), widget);
	auto* linesLabel = new QLabel(QString::number(d->editor->document()->blockCount()), widget);
	connect(d->editor->document(), &QTextDocument::blockCountChanged,
		linesLabel, [linesLabel](int count) {
			linesLabel->setNum(count);
		});

	form->insertRow(0, tr("Файл:"), pathLabel);
	form->addRow(tr("Язык:"), languageLabel);
	form->addRow(tr("Строк:"), linesLabel);
	return widget;
}
