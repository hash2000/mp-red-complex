#include "Content/CodeEditorWidget/documents/code_editor_document.h"
#include "Content/CodeEditorWidget/widgets/code_editor_widget.h"
#include "Content/CodeEditorWidget/language_info.h"
#include "Content/CodeEditorWidget/highlights/highlighter_plugin.h"
#include "Content/CodeEditorWidget/highlights/highlighter_plugin_manager.h"
#include "Content/CodeEditorWidget/formatters/formatter_plugin_manager.h"

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
#include <QMessageBox>
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

	QString path;

	void setupUi();
	QWidget* createRibbon();
	RibbonGroup addGroup(QHBoxLayout* ribbonLayout, const QString& title);
	QToolButton* addButton(RibbonGroup& group, const QString& glyph, const QString& tooltip);
	void setupFontSizes();
	void setupLanguages();
	void syncLanguageCombo();
	void setRibbonVisible(bool visible);
	void updateRibbonState();
	QString currentMimeType() const;
	QString currentLanguageName() const;
};

void CodeEditorDocument::Private::setupUi() {
	editor = new CodeEditorWidget(highlighting, formatters, q);
	editor->setLineWrapMode(QTextEdit::NoWrap);
	editor->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	auto* layout = new QVBoxLayout(q);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(createRibbon(), 0);
	layout->addWidget(editor, 1);

	q->setFocusProxy(editor);
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
				QMessageBox::information(q, q->tr("Форматирование"),
					q->tr("Для текущего языка форматирование недоступно."));
			}
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
			QMessageBox::warning(this, tr("Открытие файла"),
				tr("Не удалось открыть файл:\n%1").arg(QDir::toNativeSeparators(path)));
			d->path.clear();
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
		QMessageBox::warning(this, tr("Ошибка форматирования"), message);
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
	d->path.clear();
	d->editor->setText(QString());
	d->editor->document()->setModified(false);
	setModified(false);
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
		QMessageBox::warning(this, tr("Открытие файла"),
			tr("Не удалось открыть файл:\n%1").arg(QDir::toNativeSeparators(path)));
		return false;
	}

	d->path = path;
	setTitle(info.fileName());
	setToolTip(QDir::toNativeSeparators(path));
	d->editor->openFile(path);
	d->syncLanguageCombo();
	d->updateRibbonState();
	emit propertiesChanged();
	return true;
}

bool CodeEditorDocument::save() {
	QString path = d->path;
	if (path.isEmpty()) {
		path = QFileDialog::getSaveFileName(this, tr("Сохранить файл"),
			QDir::current().filePath(title() + QStringLiteral(".txt")),
			tr("Все файлы (*.*)"));
		if (path.isEmpty()) {
			return false;
		}
	}

	QFile file(path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QMessageBox::warning(this, tr("Сохранение файла"),
			tr("Не удалось записать файл:\n%1\n%2")
				.arg(QDir::toNativeSeparators(path), file.errorString()));
		return false;
	}

	QTextStream stream(&file);
	stream.setEncoding(QStringConverter::Utf8);
	stream << d->editor->toPlainText();
	stream.flush();

	if (stream.status() != QTextStream::Ok) {
		QMessageBox::warning(this, tr("Сохранение файла"),
			tr("Не удалось записать файл:\n%1").arg(QDir::toNativeSeparators(path)));
		return false;
	}

	file.close();

	d->path = path;
	d->editor->setHoghlighterByPath(path);
	d->editor->document()->setModified(false);
	setModified(false);
	setTitle(QFileInfo(path).fileName());
	setToolTip(QDir::toNativeSeparators(path));
	d->syncLanguageCombo();
	d->updateRibbonState();
	emit propertiesChanged();
	return true;
}

void CodeEditorDocument::refresh() {
	if (d->path.isEmpty() || isModified()) {
		return;
	}

	if (!QFileInfo::exists(d->path)) {
		QMessageBox::warning(this, tr("Обновление документа"),
			tr("Файл не найден:\n%1").arg(QDir::toNativeSeparators(d->path)));
		return;
	}

	d->editor->openFile(d->path);
	d->syncLanguageCombo();
	d->updateRibbonState();
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
