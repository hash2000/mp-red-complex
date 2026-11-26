#include "Content/TextEditorModule/documents/paged_text_document.h"
#include "Content/TextEditorModule/documents/text_document_options.h"
#include "Libs/BaseWidgets/editor/formatters/formatter_plugin_manager.h"
#include "Libs/BaseWidgets/editor/highlights/highlighter_plugin.h"
#include "Libs/BaseWidgets/editor/highlights/highlighter_plugin_manager.h"
#include "Libs/BaseWidgets/editor/language_info.h"
#include "Libs/BaseWidgets/editor/navigation/text_navigation.h"
#include "Libs/BaseWidgets/editor/widgets/code_editor_widget.h"
#include "Libs/BaseWidgets/editor/widgets/editor_settings_builder.h"
#include "Libs/BaseWidgets/editor/widgets/encoding_dialog.h"
#include "Libs/BaseWidgets/editor/widgets/paged_text_editor.h"
#include "Libs/BaseWidgets/editor/widgets/search_bar.h"
#include "Libs/BaseWidgets/ribbon/document_ribbon.h"
#include "Libs/BaseWidgets/ribbon/ribbon_button.h"
#include "Libs/Engine/scripting/command_context.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCompleter>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>

struct PagedTextDocument::Private {
	PagedTextDocument* q = nullptr;
	PagedTextEditor* editor = nullptr;
	DocumentRibbon* ribbon = nullptr;
	RibbonButton* searchButton = nullptr;
	RibbonButton* formatButton = nullptr;
	RibbonButton* highlightButton = nullptr;
	RibbonButton* saveButton = nullptr;
	RibbonButton* undoButton = nullptr;
	RibbonButton* redoButton = nullptr;
	RibbonButton* cutButton = nullptr;
	RibbonButton* copyButton = nullptr;
	RibbonButton* pasteButton = nullptr;
	HighlightingPluginManager* highlighting = nullptr;
	FormatterPluginManager* formatters = nullptr;

	QWidget* searchBar = nullptr;
	SearchBar* search = nullptr;

	QLabel* linesLabel = nullptr;
	QLabel* statusLabel = nullptr;
	QLabel* cursorLabel = nullptr;
	QLabel* encodingLabel = nullptr;
	QLabel* maxLineLabel = nullptr;
	QLabel* sizeLabel = nullptr;
	QLabel* dirtyPagesLabel = nullptr;
	QLabel* bufferLabel = nullptr;
	QLabel* spilledLabel = nullptr;
	QCheckBox* caseSensitiveCheck = nullptr;
	QCheckBox* lineNumbersCheck = nullptr;
	QComboBox* fontSizeCombo = nullptr;
	QComboBox* wordWrapCombo = nullptr;
	QComboBox* highlightingCombo = nullptr;

	QString path;

	/// Реестр стратегий Ctrl+Left/Right: хранит экземпляр стратегии, пока
	/// жив документ, — редактор держит только указатель.
	TextNavigationStrategyRegistry navigationStrategies;

	void createRibbon();
	QWidget* createSearchBar();
	void setSearchVisible(bool visible);
	void syncEncodingLabel();
	void syncFontSize(int size);
	void syncWordWrap(bool enabled);
	void syncHighlightingCombo();
	void updateFormatButton();
	void updateHighlightButton();
	/// Доступность кнопок правки (сохранение, отмена, повтор, выделение).
	void updateEditButtons();
	void updateInfoLabels();
	QWidget* createInfoWidget(QWidget* parent);
	void formatDocument();
	void saveDocument();
	/// Спросить подтверждение, если правки будут потеряны.
	bool confirmDiscard();
	/// Применить глобальные параметры текстовых документов как значения
	/// по умолчанию (шрифт, перенос строк, кодировка, предел буфера).
	void applyTextDefaults();
	void showEncodingDialog();
};

void PagedTextDocument::Private::createRibbon() {
	ribbon = new DocumentRibbon(q);

	auto* fileGroup = ribbon->addGroup(PagedTextDocument::tr("Файл"));
	connect(ribbon->addButton(fileGroup, QStringLiteral("🗋"), PagedTextDocument::tr("Новый файл")),
		&RibbonButton::clicked, q, [this] {
			if (confirmDiscard()) {
				q->newFile();
			}
		});
	connect(ribbon->addButton(fileGroup, QStringLiteral("📂"), PagedTextDocument::tr("Открыть файл")),
		&RibbonButton::clicked, q, [this] {
			const auto path = QFileDialog::getOpenFileName(q, PagedTextDocument::tr("Открыть файл"),
				QDir::currentPath(), PagedTextDocument::tr("Все файлы (*.*)"));
			if (!path.isEmpty() && confirmDiscard()) {
				q->openFile(path);
			}
		});
	connect(ribbon->addButton(fileGroup, QStringLiteral("⟳"), PagedTextDocument::tr("Перечитать файл")),
		&RibbonButton::clicked, q, [this] {
			if (!path.isEmpty() && confirmDiscard()) {
				q->openFile(path);
			}
		});
	saveButton = ribbon->addButton(fileGroup, QStringLiteral("💾"),
		PagedTextDocument::tr("Сохранить файл"));
	saveButton->setObjectName(QStringLiteral("PagedSaveButton"));
	connect(saveButton, &RibbonButton::clicked, q, [this] {
		saveDocument();
	});

	auto* editGroup = ribbon->addGroup(PagedTextDocument::tr("Правка"));
	undoButton = ribbon->addButton(editGroup, QStringLiteral("↶"),
		PagedTextDocument::tr("Отменить правку"));
	undoButton->setObjectName(QStringLiteral("PagedUndoButton"));
	connect(undoButton, &RibbonButton::clicked, q, [this] {
		editor->undo();
	});
	redoButton = ribbon->addButton(editGroup, QStringLiteral("↷"),
		PagedTextDocument::tr("Повторить отменённую правку"));
	redoButton->setObjectName(QStringLiteral("PagedRedoButton"));
	connect(redoButton, &RibbonButton::clicked, q, [this] {
		editor->redo();
	});
	cutButton = ribbon->addButton(editGroup, QStringLiteral("✂️"),
		PagedTextDocument::tr("Вырезать выделенное в буфер обмена"));
	cutButton->setObjectName(QStringLiteral("PagedCutButton"));
	connect(cutButton, &RibbonButton::clicked, q, [this] {
		editor->cut();
	});
	copyButton = ribbon->addButton(editGroup, QStringLiteral("⧉"),
		PagedTextDocument::tr("Копировать выделенное в буфер обмена"));
	copyButton->setObjectName(QStringLiteral("PagedCopyButton"));
	connect(copyButton, &RibbonButton::clicked, q, [this] {
		editor->copy();
	});
	pasteButton = ribbon->addButton(editGroup, QStringLiteral("📋"),
		PagedTextDocument::tr("Вставить из буфера обмена"));
	pasteButton->setObjectName(QStringLiteral("PagedPasteButton"));
	connect(pasteButton, &RibbonButton::clicked, q, [this] {
		editor->paste();
	});

	formatButton = ribbon->addButton(editGroup, QStringLiteral("✨"),
		PagedTextDocument::tr("Форматировать файл (результат записывается в файл)"));
	formatButton->setObjectName(QStringLiteral("PagedFormatButton"));
	connect(formatButton, &RibbonButton::clicked, q, [this] {
		formatDocument();
	});

	highlightButton = ribbon->addButton(editGroup, QStringLiteral("🎨"),
		PagedTextDocument::tr("Подсветка текста (включить/выключить)"));
	highlightButton->setObjectName(QStringLiteral("PagedHighlightButton"));
	highlightButton->setCheckable(true);
	highlightButton->setChecked(editor->isHighlightingEnabled());
	connect(highlightButton, &RibbonButton::toggled, q, [this](bool checked) {
		editor->setHighlightingEnabled(checked);
	});

	auto* searchGroup = ribbon->addGroup(PagedTextDocument::tr("Поиск"));
	searchButton = ribbon->addButton(searchGroup, QStringLiteral("🔍"),
		PagedTextDocument::tr("Поиск (показать/скрыть)"), QStringLiteral("search"));
	searchButton->setCheckable(true);
	connect(searchButton, &RibbonButton::toggled, q, [this](bool checked) {
		setSearchVisible(checked);
	});

	connect(ribbon, &DocumentRibbon::propertiesRequested, q,
		[this](const QString& groupId) {
			q->showProperties(groupId);
		});
}

QWidget* PagedTextDocument::Private::createSearchBar() {
	search = new SearchBar(q);
	search->hide();

	connect(search, &SearchBar::queryChanged, q, [this](const QString& text) {
		editor->setSearchQuery(text);
	});
	connect(search, &SearchBar::previousRequested, q, [this] {
		editor->searchPrevious();
	});
	connect(search, &SearchBar::nextRequested, q, [this] {
		editor->searchNext();
	});
	connect(search, &SearchBar::closeRequested, q, [this] {
		if (searchButton) {
			searchButton->setChecked(false);
		}
	});

	return search;
}

void PagedTextDocument::Private::setSearchVisible(bool visible) {
	if (!searchBar) {
		return;
	}

	if (visible) {
		searchBar->show();
		search->focusSearch();
		if (searchButton) {
			const QSignalBlocker blocker(searchButton);
			searchButton->setChecked(true);
		}
		return;
	}

	searchBar->hide();
	editor->setSearchQuery(QString());
	editor->setFocus();
	if (searchButton) {
		const QSignalBlocker blocker(searchButton);
		searchButton->setChecked(false);
	}
}

void PagedTextDocument::Private::syncHighlightingCombo() {
	if (!highlightingCombo) {
		return;
	}

	const auto* current = editor->highlightingPlugin();
	const QSignalBlocker blocker(highlightingCombo);
	if (!current) {
		highlightingCombo->setCurrentIndex(-1);
		return;
	}
	for (int i = 0; i < highlightingCombo->count(); i++) {
		const auto pointer = reinterpret_cast<HighlightingPlugin*>(
			highlightingCombo->itemData(i).value<qulonglong>());
		if (pointer == current) {
			highlightingCombo->setCurrentIndex(i);
			return;
		}
	}
	highlightingCombo->setCurrentIndex(-1);
}

void PagedTextDocument::Private::updateFormatButton() {
	if (formatButton) {
		formatButton->setEnabled(!editor->isFormatting());
	}
}

void PagedTextDocument::Private::updateHighlightButton() {
	if (!highlightButton) {
		return;
	}
	const QSignalBlocker blocker(highlightButton);
	highlightButton->setEnabled(editor->highlightingPlugin() != nullptr);
	highlightButton->setChecked(editor->isHighlightingEnabled());
}

void PagedTextDocument::Private::updateEditButtons() {
	const bool readOnly = editor->isReadOnly();
	const bool ready = editor->lineCount() > 0;
	const bool hasSelection = editor->hasSelection();

	if (saveButton) {
		saveButton->setEnabled(editor->isModified());
	}
	if (undoButton) {
		undoButton->setEnabled(editor->canUndo() && !readOnly);
	}
	if (redoButton) {
		redoButton->setEnabled(editor->canRedo() && !readOnly);
	}
	if (cutButton) {
		cutButton->setEnabled(ready && !readOnly && hasSelection);
	}
	if (copyButton) {
		copyButton->setEnabled(ready);
	}
	if (pasteButton) {
		pasteButton->setEnabled(ready && !readOnly);
	}
}

void PagedTextDocument::Private::updateInfoLabels() {
	if (linesLabel) {
		linesLabel->setNum(editor->lineCount());
	}
	if (dirtyPagesLabel) {
		dirtyPagesLabel->setNum(editor->modifiedPageCount());
	}
	if (maxLineLabel) {
		maxLineLabel->setNum(editor->maxLineLength());
	}
	if (bufferLabel) {
		const auto megabytes = [](qint64 bytes) {
			return QString::number(bytes / (1024.0 * 1024.0), 'f', 1);
		};
		const QString limit = editor->memoryBudget() > 0
			? QStringLiteral("%1 МБ").arg(megabytes(editor->memoryBudget()))
			: PagedTextDocument::tr("без предела");
		bufferLabel->setText(QStringLiteral("%1 / %2")
			.arg(megabytes(editor->bufferedBytes()), limit));
	}
	if (spilledLabel) {
		spilledLabel->setNum(editor->spilledPageCount());
	}
	if (statusLabel && !editor->isIndexing()) {
		statusLabel->setText(editor->isModified()
			? PagedTextDocument::tr("Есть несохранённые изменения")
			: PagedTextDocument::tr("Готово"));
	}
}

void PagedTextDocument::Private::saveDocument() {
	q->save();
}

bool PagedTextDocument::Private::confirmDiscard() {
	if (!editor->isModified()) {
		return true;
	}

	const auto answer = QMessageBox::question(q,
		PagedTextDocument::tr("Несохранённые изменения"),
		PagedTextDocument::tr("В документе есть несохранённые изменения. "
			"Они будут потеряны. Продолжить?"),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
	return answer == QMessageBox::Yes;
}

void PagedTextDocument::Private::applyTextDefaults() {
	const TextDocumentOptions options = TextDocumentOptions::fromContext(q->context());

	QFont font = editor->font();
	if (!options.fontFamily.isEmpty()) {
		font.setFamily(options.fontFamily);
		editor->setFont(font);
	}
	if (options.fontSize > 0) {
		editor->setFontSize(options.fontSize);
	}

	editor->setWordWrap(options.wordWrap);
	editor->setMemoryBudget(qint64(options.bufferLimitMb) * 1024 * 1024);
	editor->setTabWidth(options.tabSpaces);
	editor->setTabMode(options.tabEdit);
	editor->setSaveTabMode(options.tabSave);
	editor->setNavigationStrategy(
		navigationStrategies.strategy(options.navigation));

	if (!options.encoding.isEmpty()) {
		editor->setEncoding(options.encoding);
	}

	syncWordWrap(editor->wordWrap());
	syncFontSize(editor->fontSize());
	syncEncodingLabel();
	updateInfoLabels();
}

void PagedTextDocument::Private::formatDocument() {
	if (!editor->canFormat()) {
		QMessageBox::warning(q, PagedTextDocument::tr("Форматирование"),
			PagedTextDocument::tr("Для текущего языка форматирование недоступно."));
		return;
	}

	// Форматирование читает и перезаписывает файл целиком: для больших
	// файлов предупреждаем до запуска.
	constexpr qint64 kLargeFileBytes = 5 * 1024 * 1024;
	if (editor->fileSize() > kLargeFileBytes) {
		QMessageBox box(q);
		box.setIcon(QMessageBox::Warning);
		box.setWindowTitle(PagedTextDocument::tr("Форматирование"));
		box.setText(PagedTextDocument::tr(
			"Файл имеет размер %1 МБ. Форматирование прочитает его целиком "
			"и может занять заметное время.")
			.arg(QString::number(editor->fileSize() / (1024.0 * 1024.0), 'f', 1)));
		box.setInformativeText(PagedTextDocument::tr("Продолжить?"));
		auto* ok = box.addButton(PagedTextDocument::tr("Ok"), QMessageBox::AcceptRole);
		auto* cancel = box.addButton(PagedTextDocument::tr("Отмена"),
			QMessageBox::RejectRole);
		box.setDefaultButton(cancel);
		box.exec();
		if (box.clickedButton() != ok) {
			return;
		}
	}

	editor->formatDocument();
}

void PagedTextDocument::Private::syncEncodingLabel() {
	if (!encodingLabel) {
		return;
	}

	QString title = editor->encoding();
	for (const auto& info : CodeEditorWidget::availableEncodings()) {
		if (info.name == editor->encoding()) {
			title = info.title;
			break;
		}
	}
	encodingLabel->setText(title);
}

void PagedTextDocument::Private::syncFontSize(int size) {
	if (!fontSizeCombo) {
		return;
	}

	int index = fontSizeCombo->findData(size);
	if (index < 0) {
		int insertAt = 0;
		for (int i = 0; i < fontSizeCombo->count(); i++) {
			if (fontSizeCombo->itemData(i).toInt() < size) {
				insertAt = i + 1;
			}
		}
		fontSizeCombo->insertItem(insertAt, QString::number(size), size);
		index = insertAt;
	}

	const QSignalBlocker blocker(fontSizeCombo);
	fontSizeCombo->setCurrentIndex(index);
}

void PagedTextDocument::Private::syncWordWrap(bool enabled) {
	if (wordWrapCombo) {
		const QSignalBlocker blocker(wordWrapCombo);
		wordWrapCombo->setCurrentIndex(wordWrapCombo->findData(enabled ? 1 : 0));
	}
}


void PagedTextDocument::Private::showEncodingDialog() {
	const QString hint = path.isEmpty()
		? PagedTextDocument::tr("Файл не задан.")
		: PagedTextDocument::tr("Файл: %1").arg(QDir::toNativeSeparators(path));

	EncodingDialog dialog(editor->encoding(), hint, /*allowResave*/ false, q);
	if (dialog.exec() != QDialog::Accepted) {
		return;
	}

	const QString encoding = dialog.selectedEncoding();
	if (encoding.isEmpty()) {
		return;
	}

	if (editor->isModified()) {
		QMessageBox::warning(q, PagedTextDocument::tr("Кодировка"),
			PagedTextDocument::tr("Есть несохранённые изменения: сохраните файл, "
				"чтобы переоткрыть его в другой кодировке."));
		return;
	}

	editor->setEncoding(encoding);
	syncEncodingLabel();
}

PagedTextDocument::PagedTextDocument(QWidget* parent)
	: PagedTextDocument(nullptr, nullptr, parent) {
}

PagedTextDocument::PagedTextDocument(HighlightingPluginManager* highlighting,
	FormatterPluginManager* formatters, QWidget* parent)
	: DocumentWidget(parent)
	, d(std::make_unique<Private>()) {
	d->q = this;
	d->highlighting = highlighting;
	d->formatters = formatters;
	d->editor = new PagedTextEditor(this);
	d->editor->setHighlightingManager(highlighting);
	d->editor->setFormatterManager(formatters);

	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	d->createRibbon();
	d->updateHighlightButton();
	d->updateFormatButton();
	d->updateEditButtons();
	layout->addWidget(d->ribbon, 0);
	d->searchBar = d->createSearchBar();
	layout->addWidget(d->searchBar, 0);
	layout->addWidget(d->editor, 1);

	connect(d->editor, &PagedTextEditor::lineCountChanged, this, [this](int lines) {
		if (d->linesLabel) {
			d->linesLabel->setNum(lines);
		}
	});
	connect(d->editor, &PagedTextEditor::indexingFinished, this,
		[this](bool success, const QString& errorMessage) {
			if (d->statusLabel) {
				d->statusLabel->setText(success ? tr("Готово") : errorMessage);
			}
			if (success && !d->path.isEmpty()) {
				setTitle(QFileInfo(d->path).fileName());
			}
			if (d->linesLabel) {
				d->linesLabel->setNum(d->editor->lineCount());
			}
			if (d->maxLineLabel) {
				d->maxLineLabel->setNum(d->editor->maxLineLength());
			}
			if (d->sizeLabel) {
				d->sizeLabel->setText(QStringLiteral("%1 МБ").arg(
					QString::number(d->editor->fileSize() / (1024.0 * 1024.0), 'f', 1)));
			}
			d->updateFormatButton();
			d->updateHighlightButton();
			d->updateEditButtons();
			d->updateInfoLabels();
		});
	connect(d->editor, &PagedTextEditor::cursorPositionChanged, this,
		[this](int line, int column) {
			if (d->cursorLabel) {
				d->cursorLabel->setText(QStringLiteral("%1:%2")
					.arg(line + 1).arg(column + 1));
			}
			// Кнопки «Вырезать»/«Копировать» зависят от выделения.
			d->updateEditButtons();
		});
	connect(d->editor, &PagedTextEditor::modifiedChanged, this, [this](bool modified) {
		setModified(modified);
		d->updateEditButtons();
		d->updateInfoLabels();
	});
	connect(d->editor, &PagedTextEditor::undoAvailableChanged, this, [this](bool) {
		d->updateEditButtons();
	});
	connect(d->editor, &PagedTextEditor::redoAvailableChanged, this, [this](bool) {
		d->updateEditButtons();
	});
	connect(d->editor, &PagedTextEditor::saved, this,
		[this](const QString&, bool success, const QString& errorMessage) {
			if (!success) {
				QMessageBox::warning(this, tr("Сохранение файла"), errorMessage);
			}
			d->updateEditButtons();
			emit propertiesChanged();
		});
	connect(d->editor, &PagedTextEditor::bufferSpilled, this, [this](int pages) {
		d->updateInfoLabels();
		if (d->statusLabel && pages > 0) {
			d->statusLabel->setText(tr("Буфер выгружен во временный файл: %1 стр.")
				.arg(pages));
		}
	});
	connect(d->editor, &PagedTextEditor::searchCountChanged, this, [this](int count) {
		if (!d->search) {
			return;
		}

		if (count < 0) {
			d->search->setBusy();
			return;
		}
		d->search->setCounter(d->editor->currentSearchIndex(), count);
	});
	connect(d->editor, &PagedTextEditor::searchProgressChanged, this,
		[this](int count) {
			if (d->search) {
				d->search->setCounter(-1, count);
			}
		});
	connect(d->editor, &PagedTextEditor::searchCurrentChanged, this,
		[this](int current, int total) {
			if (d->search) {
				d->search->setCounter(current, total);
			}
		});
	connect(d->editor, &PagedTextEditor::fontSizeChanged, this, [this](int size) {
		d->syncFontSize(size);
	});
	connect(d->editor, &PagedTextEditor::wordWrapChanged, this, [this](bool enabled) {
		d->syncWordWrap(enabled);
	});
	connect(d->editor, &PagedTextEditor::lineNumbersVisibleChanged, this,
		[this](bool visible) {
			if (d->lineNumbersCheck) {
				const QSignalBlocker blocker(d->lineNumbersCheck);
				d->lineNumbersCheck->setChecked(visible);
			}
		});
	connect(d->editor, &PagedTextEditor::highlightingChanged, this, [this] {
		d->syncHighlightingCombo();
		d->updateFormatButton();
		d->updateHighlightButton();
	});
	connect(d->editor, &PagedTextEditor::highlightingEnabledChanged, this,
		[this](bool) {
			d->updateHighlightButton();
		});
	connect(d->editor, &PagedTextEditor::formattingStarted, this, [this] {
		d->updateFormatButton();
		if (d->statusLabel) {
			d->statusLabel->setText(tr("Форматирование…"));
		}
	});
	connect(d->editor, &PagedTextEditor::formattingFinished, this, [this](bool) {
		d->updateFormatButton();
		if (d->statusLabel) {
			d->statusLabel->setText(tr("Готово"));
		}
	});
	connect(d->editor, &PagedTextEditor::formattingFailed, this,
		[this](const QString& errorMessage) {
			d->updateFormatButton();
			if (d->statusLabel) {
				d->statusLabel->setText(errorMessage);
			}
			QMessageBox::warning(this, tr("Форматирование"), errorMessage);
		});

	setFocusProxy(d->editor);
	setTitle(tr("Без имени"));
}

PagedTextDocument::~PagedTextDocument() = default;

PagedTextEditor* PagedTextDocument::editor() const {
	return d->editor;
}

QWidget* PagedTextDocument::createPropertiesEditor(QWidget* parent) {
	auto* widget = new QWidget(parent);
	auto* form = new QFormLayout(widget);

	auto* pathLabel = new QLabel(
		d->path.isEmpty() ? tr("<не задан>") : QDir::toNativeSeparators(d->path), widget);
	pathLabel->setWordWrap(true);
	pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

	d->encodingLabel = new QLabel(widget);
	d->syncEncodingLabel();

	auto* encodingRow = new QWidget(widget);
	auto* encodingLayout = new QHBoxLayout(encodingRow);
	encodingLayout->setContentsMargins(0, 0, 0, 0);
	encodingLayout->setSpacing(4);
	encodingLayout->addWidget(d->encodingLabel, 1);

	auto* encodingButton = new QToolButton(encodingRow);
	encodingButton->setObjectName(QStringLiteral("PagedEncodingButton"));
	encodingButton->setText(QStringLiteral("…"));
	encodingButton->setToolTip(tr("Выбрать кодировку"));
	encodingLayout->addWidget(encodingButton);
	connect(encodingButton, &QToolButton::clicked, this, [this] {
		d->showEncodingDialog();
	});

	if (d->highlighting) {
		d->highlightingCombo = new QComboBox(widget);
		d->highlightingCombo->setObjectName(QStringLiteral("PagedHighlightingCombo"));
		d->highlightingCombo->setToolTip(tr("Язык подсветки документа"));

		QList<QPair<QString, HighlightingPlugin*>> items;
		for (auto* plugin : d->highlighting->allPlugins()) {
			const auto info = plugin->languageInfo();
			if (info.mimeType.isEmpty()) {
				continue;
			}
			items.append({ info.name.isEmpty() ? info.mimeType : info.name, plugin });
		}
		std::sort(items.begin(), items.end(), [](const auto& left, const auto& right) {
			return left.first.localeAwareCompare(right.first) < 0;
		});
		for (const auto& item : items) {
			d->highlightingCombo->addItem(item.first,
				QVariant::fromValue<qulonglong>(
					static_cast<qulonglong>(reinterpret_cast<quintptr>(item.second))));
		}

		connect(d->highlightingCombo, &QComboBox::currentIndexChanged, this,
			[this](int index) {
				if (!d->highlightingCombo || index < 0) {
					return;
				}
				auto* plugin = reinterpret_cast<HighlightingPlugin*>(
					d->highlightingCombo->itemData(index).value<qulonglong>());
				d->editor->setHighlightingPlugin(plugin);
				d->editor->setHighlightingEnabled(plugin != nullptr);
				d->updateHighlightButton();
				d->updateFormatButton();
			});
		d->syncHighlightingCombo();
	}

	form->addRow(tr("Файл:"), pathLabel);
	if (d->highlightingCombo) {
		form->addRow(tr("Язык:"), d->highlightingCombo);
	}
	form->addRow(tr("Кодировка:"), encodingRow);

	// Общие параметры редактора строит единый билдер: одинаковые подписи,
	// глифы и внешний вид у редактора кода и просмотрщика.
	EditorSettingsBuilder settings(widget);
	d->fontSizeCombo = settings.addFontSizeRow(form, d->editor->fontSize(),
		[this](int size) {
			d->editor->setFontSize(size);
		});
	d->wordWrapCombo = settings.addLineWrapRow(form,
		d->editor->wordWrap() ? 1 : 0, /*allowAuto*/ false,
		[this](int value) {
			d->editor->setWordWrap(value == 1);
		});
	d->lineNumbersCheck = settings.addLineNumbersRow(form,
		QStringLiteral("PagedLineNumbers"), d->editor->lineNumbersVisible(),
		[this](bool checked) {
			d->editor->setLineNumbersVisible(checked);
		});

	// Предел памяти под изменённые страницы: при превышении они уходят во
	// временный файл в системном каталоге temp.
	auto* bufferLimit = new QSpinBox(widget);
	bufferLimit->setObjectName(QStringLiteral("PagedBufferLimit"));
	bufferLimit->setRange(0, 65536);
	bufferLimit->setSpecialValueText(tr("без предела"));
	bufferLimit->setSuffix(tr(" МБ"));
	bufferLimit->setToolTip(tr("Сколько памяти отводится под изменённые страницы. "
		"При превышении они выгружаются во временный файл, который удаляется "
		"вместе с документом"));
	bufferLimit->setValue(d->editor->memoryBudget() > 0
		? int(d->editor->memoryBudget() / (1024 * 1024)) : 0);
	connect(bufferLimit, &QSpinBox::valueChanged, this, [this](int megabytes) {
		d->editor->setMemoryBudget(megabytes <= 0
			? 0 : qint64(megabytes) * 1024 * 1024);
		d->updateInfoLabels();
	});
	form->addRow(tr("Предел буфера:"), bufferLimit);
	return widget;
}
QWidget* PagedTextDocument::Private::createInfoWidget(QWidget* parent) {
	auto* widget = new QWidget(parent);
	auto* form = new QFormLayout(widget);
	form->setContentsMargins(0, 0, 0, 0);

	linesLabel = new QLabel(QString::number(editor->lineCount()), widget);
	cursorLabel = new QLabel(QStringLiteral("%1:%2")
		.arg(editor->cursorLine() + 1).arg(editor->cursorColumn() + 1), widget);
	maxLineLabel = new QLabel(QString::number(editor->maxLineLength()), widget);
	sizeLabel = new QLabel(QStringLiteral("%1 МБ")
		.arg(QString::number(editor->fileSize() / (1024.0 * 1024.0), 'f', 1)), widget);
	statusLabel = new QLabel(editor->isIndexing() ? PagedTextDocument::tr("Индексация…")
		: PagedTextDocument::tr("Готово"), widget);
	dirtyPagesLabel = new QLabel(QString::number(editor->modifiedPageCount()), widget);
	bufferLabel = new QLabel(widget);
	spilledLabel = new QLabel(QString::number(editor->spilledPageCount()), widget);

	form->addRow(PagedTextDocument::tr("Строк:"), linesLabel);
	form->addRow(PagedTextDocument::tr("Самая длинная строка:"), maxLineLabel);
	form->addRow(PagedTextDocument::tr("Размер файла:"), sizeLabel);
	form->addRow(PagedTextDocument::tr("Курсор:"), cursorLabel);
	form->addRow(PagedTextDocument::tr("Изменённых страниц:"), dirtyPagesLabel);
	form->addRow(PagedTextDocument::tr("Буфер памяти:"), bufferLabel);
	form->addRow(PagedTextDocument::tr("Во временном файле:"), spilledLabel);
	form->addRow(PagedTextDocument::tr("Состояние:"), statusLabel);
	updateInfoLabels();
	return widget;
}

QList<DocumentPropertyGroup> PagedTextDocument::createPropertyGroups(QWidget* parent) {
	DocumentPropertyGroup info;
	info.id = QStringLiteral("info");
	info.title = tr("Информация о документе");
	info.expanded = true;
	info.editor = d->createInfoWidget(parent);

	DocumentPropertyGroup search;
	search.id = QStringLiteral("search");
	search.title = tr("Параметры поиска");
	search.expanded = false;

	auto* editor = new QWidget(parent);
	auto* layout = new QVBoxLayout(editor);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(4);

	d->caseSensitiveCheck = new QCheckBox(tr("Учитывать регистр"), editor);
	d->caseSensitiveCheck->setObjectName(QStringLiteral("PagedSearchCaseSensitive"));
	d->caseSensitiveCheck->setChecked(d->editor->searchCaseSensitive());
	connect(d->caseSensitiveCheck, &QCheckBox::toggled, this, [this](bool checked) {
		d->editor->setSearchCaseSensitive(checked);
	});
	layout->addWidget(d->caseSensitiveCheck);

	auto* hint = new QLabel(tr("Поиск по подстроке, только по видимым строкам "
		"подсвечиваются совпадения; счётчик считается по всему файлу."), editor);
	hint->setWordWrap(true);
	hint->setObjectName(QStringLiteral("PagedSearchHint"));
	layout->addWidget(hint);
	layout->addStretch();

	search.editor = editor;
	return { info, search };
}

void PagedTextDocument::newFile() {
	d->path.clear();
	setVariable(QStringLiteral("path"), QString());
	d->editor->newDocument();
	setTitle(tr("Без имени"));
	setModified(false);
	d->syncEncodingLabel();
	d->updateFormatButton();
	d->updateHighlightButton();
	d->updateEditButtons();
	d->updateInfoLabels();
	emit propertiesChanged();
}

void PagedTextDocument::openFile(const QString& path) {
	if (path.isEmpty()) {
		return;
	}

	d->path = path;
	setVariable(QStringLiteral("path"), path);
	setTitle(QFileInfo(path).fileName());
	d->editor->openFile(path, d->editor->encoding().isEmpty()
		? QStringLiteral("UTF-8") : d->editor->encoding());
	d->syncEncodingLabel();
	d->syncHighlightingCombo();
	d->updateFormatButton();
	d->updateEditButtons();
	emit propertiesChanged();
}

bool PagedTextDocument::save() {
	QString path = d->path;
	if (path.isEmpty()) {
		path = QFileDialog::getSaveFileName(this, tr("Сохранить файл"),
			QDir::current().filePath(title() + QStringLiteral(".txt")),
			tr("Все файлы (*.*)"));
		if (path.isEmpty()) {
			return false;
		}
	}

	// Запись выполняет редактор: он владеет изменёнными страницами.
	// При ошибке он сообщает об этом сигналом saved().
	if (!d->editor->saveAs(path)) {
		return false;
	}

	d->path = path;
	setVariable(QStringLiteral("path"), path);
	setTitle(QFileInfo(path).fileName());
	setModified(false);
	d->syncEncodingLabel();
	d->updateEditButtons();
	d->updateInfoLabels();
	emit propertiesChanged();

	if (auto context = this->context()) {
		context->fireEvent(CommandContext::documentSaveCompleteEvent());
	}
	return true;
}

void PagedTextDocument::refresh() {
	if (d->path.isEmpty() || d->editor->isIndexing()) {
		return;
	}

	// Перечитывание файла потеряло бы несохранённые правки.
	if (d->editor->isModified()) {
		if (d->statusLabel) {
			d->statusLabel->setText(tr("Есть несохранённые изменения"));
		}
		return;
	}

	d->editor->openFile(d->path, d->editor->encoding());
}

void PagedTextDocument::onContextAttached() {
	// Глобальные параметры текста — значения по умолчанию для нового
	// документа: шрифт, перенос строк, кодировка и предел буфера.
	d->applyTextDefaults();

	const auto path = variable(QStringLiteral("path"), QString()).toString();
	if (path.isEmpty()) {
		return;
	}

	openFile(path);
}

void PagedTextDocument::onContextVariableChanged(const QString& name, const QVariant& value) {
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

QList<DocumentVariable> PagedTextDocumentMetadata::variables() const {
	return {
		{ QStringLiteral("path"), { QString::fromLatin1(DocumentVariableMime::kFilePath) } }
	};
}

QString PagedTextDocumentMetadata::tooltipVariable() const {
	return QStringLiteral("path");
}
