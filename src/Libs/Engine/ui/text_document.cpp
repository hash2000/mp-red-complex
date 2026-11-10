#include "Libs/Engine/ui/text_document.h"

#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFormLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QVBoxLayout>

struct TextDocument::Private {
	QPlainTextEdit* editor = nullptr;
	QString path;
};

TextDocument::TextDocument(QWidget* parent)
	: DocumentWidget(parent), d(std::make_unique<Private>()) {
	setGlyph(QStringLiteral("📝"));

	auto* lay = new QVBoxLayout(this);
	lay->setContentsMargins(0, 0, 0, 0);

	d->editor = new QPlainTextEdit(this);
	d->editor->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
	d->editor->setLineWrapMode(QPlainTextEdit::NoWrap);
	lay->addWidget(d->editor);
	setFocusProxy(d->editor);

	connect(d->editor->document(), &QTextDocument::modificationChanged,
		this, &DocumentWidget::setModified);
}

TextDocument::~TextDocument() = default;

bool TextDocument::loadFile(const QString& path) {
	QFile f(path);
	if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;

	d->editor->setPlainText(QString::fromUtf8(f.readAll()));
	d->editor->document()->setModified(false);
	d->path = path;
	setToolTip(QDir::toNativeSeparators(path));
	setTitle(QFileInfo(path).fileName());
	emit propertiesChanged();
	return true;
}

QString TextDocument::filePath() const {
	return d->path;
}

bool TextDocument::save() {
	if (d->path.isEmpty()) {
		const QString p = QFileDialog::getSaveFileName(this, tr("Сохранить как"),
			title() + QStringLiteral(".txt"));
		if (p.isEmpty()) return false;
		d->path = p;
	}
	QFile f(d->path);
	if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
		return false;
	f.write(d->editor->toPlainText().toUtf8());
	d->editor->document()->setModified(false);
	setToolTip(QDir::toNativeSeparators(d->path));
	setTitle(QFileInfo(d->path).fileName());
	emit propertiesChanged();
	return true;
}

void TextDocument::refresh() {
	if (!d->path.isEmpty() && !isModified())
		loadFile(d->path);
}

QWidget* TextDocument::createPropertiesEditor(QWidget* parent) {
	QWidget* w = DocumentWidget::createPropertiesEditor(parent);
	auto* form = qobject_cast<QFormLayout*>(w->layout());
	if (!form) return w;

	auto* pathLabel = new QLabel(d->path.isEmpty() ? tr("<не сохранён>")
		: QDir::toNativeSeparators(d->path), w);
	pathLabel->setWordWrap(true);
	pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

	auto* fontSize = new QSpinBox(w);
	fontSize->setRange(6, 48);
	fontSize->setValue(d->editor->font().pointSize() > 0 ? d->editor->font().pointSize() : 10);
	connect(fontSize, &QSpinBox::valueChanged, this, [this](int v) {
		QFont f = d->editor->font();
		f.setPointSize(v);
		d->editor->setFont(f);
	});

	auto* readOnly = new QCheckBox(w);
	readOnly->setChecked(d->editor->isReadOnly());
	connect(readOnly, &QCheckBox::toggled, d->editor, &QPlainTextEdit::setReadOnly);

	auto* wrap = new QCheckBox(w);
	wrap->setChecked(d->editor->lineWrapMode() == QPlainTextEdit::WidgetWidth);
	connect(wrap, &QCheckBox::toggled, this, [this](bool on) {
		d->editor->setLineWrapMode(on ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
	});

	auto* lines = new QLabel(QString::number(d->editor->blockCount()), w);
	connect(d->editor, &QPlainTextEdit::blockCountChanged, lines, [lines](int n) { lines->setNum(n); });

	form->insertRow(0, tr("Файл:"), pathLabel);
	form->addRow(tr("Размер шрифта:"), fontSize);
	form->addRow(tr("Только чтение:"), readOnly);
	form->addRow(tr("Перенос строк:"), wrap);
	form->addRow(tr("Строк:"), lines);
	return w;
}
