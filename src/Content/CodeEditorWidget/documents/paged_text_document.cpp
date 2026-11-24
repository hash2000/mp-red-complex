#include "Content/CodeEditorWidget/documents/paged_text_document.h"
#include "Content/CodeEditorWidget/widgets/paged_text_editor.h"

#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

struct PagedTextDocument::Private {
	PagedTextEditor* editor = nullptr;
	QLabel* linesLabel = nullptr;
	QLabel* statusLabel = nullptr;
	QString path;
};

PagedTextDocument::PagedTextDocument(QWidget* parent)
	: DocumentWidget(parent)
	, d(std::make_unique<Private>()) {
	d->editor = new PagedTextEditor(this);

	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(d->editor, 1);

	connect(d->editor, &PagedTextEditor::lineCountChanged, this, [this](int lines) {
		if (d->linesLabel) {
			d->linesLabel->setNum(lines);
		}
	});
	connect(d->editor, &PagedTextEditor::indexingFinished, this,
		[this](bool success, const QString& errorMessage) {
			if (d->statusLabel) {
				d->statusLabel->setText(success
					? tr("Готово") : errorMessage);
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

	form->addRow(tr("Файл:"), pathLabel);
	form->addRow(tr("Строк:"), d->linesLabel);
	form->addRow(tr("Состояние:"), d->statusLabel);
	form->addRow(tr("Режим:"), new QLabel(tr("Просмотр (прототип)"), widget));
	return widget;
}

void PagedTextDocument::openFile(const QString& path) {
	if (path.isEmpty()) {
		return;
	}

	d->path = path;
	setVariable(QStringLiteral("path"), path);
	setTitle(QFileInfo(path).fileName());
	setToolTip(QString()); // подсказка вкладки берётся из path в метаданных
	d->editor->openFile(path);
	emit propertiesChanged();
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
