#include "Content/TextEditorModule/documents/paged_text_document.h"
#include "Content/TextEditorModule/widgets/code_editor_widget.h"
#include "Content/TextEditorModule/widgets/encoding_dialog.h"
#include "Content/TextEditorModule/widgets/paged_text_editor.h"
#include "Content/TextEditorModule/widgets/search_bar.h"
#include "Libs/BaseWidgets/ribbon/document_ribbon.h"
#include "Libs/BaseWidgets/ribbon/ribbon_button.h"

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
#include <QPushButton>
#include <QSignalBlocker>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

struct PagedTextDocument::Private {
	PagedTextDocument* q = nullptr;
	PagedTextEditor* editor = nullptr;
	DocumentRibbon* ribbon = nullptr;
	RibbonButton* searchButton = nullptr;

	QWidget* searchBar = nullptr;
	SearchBar* search = nullptr;

	QLabel* linesLabel = nullptr;
	QLabel* statusLabel = nullptr;
	QLabel* cursorLabel = nullptr;
	QLabel* encodingLabel = nullptr;
	QCheckBox* caseSensitiveCheck = nullptr;

	QString path;

	void createRibbon();
	QWidget* createSearchBar();
	void setSearchVisible(bool visible);
	void syncEncodingLabel();
	void showEncodingDialog();
};

void PagedTextDocument::Private::createRibbon() {
	ribbon = new DocumentRibbon(q);

	auto* fileGroup = ribbon->addGroup(PagedTextDocument::tr("Файл"));
	connect(ribbon->addButton(fileGroup, QStringLiteral("🗋"), PagedTextDocument::tr("Новый файл")),
		&RibbonButton::clicked, q, [this] {
			q->newFile();
		});
	connect(ribbon->addButton(fileGroup, QStringLiteral("📂"), PagedTextDocument::tr("Открыть файл")),
		&RibbonButton::clicked, q, [this] {
			const auto path = QFileDialog::getOpenFileName(q, PagedTextDocument::tr("Открыть файл"),
				QDir::currentPath(), PagedTextDocument::tr("Все файлы (*.*)"));
			if (!path.isEmpty()) {
				q->openFile(path);
			}
		});
	connect(ribbon->addButton(fileGroup, QStringLiteral("⟳"), PagedTextDocument::tr("Перечитать файл")),
		&RibbonButton::clicked, q, [this] {
			if (!path.isEmpty()) {
				q->openFile(path);
			}
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

	editor->setEncoding(encoding);
	syncEncodingLabel();
}

PagedTextDocument::PagedTextDocument(QWidget* parent)
	: DocumentWidget(parent)
	, d(std::make_unique<Private>()) {
	d->q = this;
	d->editor = new PagedTextEditor(this);

	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	d->createRibbon();
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
		});
	connect(d->editor, &PagedTextEditor::cursorPositionChanged, this,
		[this](int line, int column) {
			if (d->cursorLabel) {
				d->cursorLabel->setText(QStringLiteral("%1:%2")
					.arg(line + 1).arg(column + 1));
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
	connect(d->editor, &PagedTextEditor::searchCurrentChanged, this,
		[this](int current, int total) {
			if (d->search) {
				d->search->setCounter(current, total);
			}
		});

	setFocusProxy(d->editor);
	setTitle(tr("Просмотр файла"));
}

PagedTextDocument::~PagedTextDocument() = default;

QWidget* PagedTextDocument::createPropertiesEditor(QWidget* parent) {
	auto* widget = new QWidget(parent);
	auto* form = new QFormLayout(widget);

	auto* pathLabel = new QLabel(
		d->path.isEmpty() ? tr("<не задан>") : QDir::toNativeSeparators(d->path), widget);
	pathLabel->setWordWrap(true);
	pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

	d->linesLabel = new QLabel(QString::number(d->editor->lineCount()), widget);
	d->statusLabel = new QLabel(d->editor->isIndexing() ? tr("Индексация…")
		: tr("Готово"), widget);
	d->cursorLabel = new QLabel(QStringLiteral("%1:%2")
		.arg(d->editor->cursorLine() + 1).arg(d->editor->cursorColumn() + 1), widget);

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

	form->addRow(tr("Файл:"), pathLabel);
	form->addRow(tr("Строк:"), d->linesLabel);
	form->addRow(tr("Кодировка:"), encodingRow);
	form->addRow(tr("Курсор:"), d->cursorLabel);
	form->addRow(tr("Состояние:"), d->statusLabel);
	form->addRow(tr("Режим:"), new QLabel(tr("Просмотр (прототип)"), widget));
	return widget;
}

QList<DocumentPropertyGroup> PagedTextDocument::createPropertyGroups(QWidget* parent) {
	DocumentPropertyGroup search;
	search.id = QStringLiteral("search");
	search.title = tr("Параметры поиска");
	search.expanded = true;

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
	return { search };
}

void PagedTextDocument::newFile() {
	d->path.clear();
	setVariable(QStringLiteral("path"), QString());
	d->editor->closeFile();
	setTitle(tr("Просмотр файла"));
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
	emit propertiesChanged();
}

void PagedTextDocument::refresh() {
	if (!d->path.isEmpty() && !d->editor->isIndexing()) {
		d->editor->openFile(d->path, d->editor->encoding());
	}
}

void PagedTextDocument::onContextAttached() {
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

	openFile(path);
}

QList<DocumentVariable> PagedTextDocumentMetadata::variables() const {
	return {
		{ QStringLiteral("path"), QString::fromLatin1(DocumentVariableMime::kFilePath) }
	};
}

QString PagedTextDocumentMetadata::tooltipVariable() const {
	return QStringLiteral("path");
}
