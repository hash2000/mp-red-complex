#include "Libs/Engine/ui/doc_widget.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>

struct DocumentWidget::Private {
	QString title;
	QString glyph = QStringLiteral("📄");
	bool modified = false;
};

DocumentWidget::DocumentWidget(QWidget* parent)
	: QWidget(parent)
	, d(std::make_unique<Private>()) {
}

DocumentWidget::~DocumentWidget() = default;

QString DocumentWidget::title() const {
	return d->title;
}

void DocumentWidget::setTitle(const QString& title) {
	if (d->title == title)
		return;
	d->title = title;
	emit titleChanged(title);
}

QString DocumentWidget::glyph() const {
	return d->glyph;
}

void DocumentWidget::setGlyph(const QString& glyph) {
	if (d->glyph == glyph)
		return;
	d->glyph = glyph;
	emit glyphChanged(glyph);
}

bool DocumentWidget::isModified() const {
	return d->modified;
}

void DocumentWidget::setModified(bool modified) {
	if (d->modified == modified)
		return;
	d->modified = modified;
	emit modifiedChanged(modified);
}

QWidget* DocumentWidget::createPropertiesEditor(QWidget* parent) {
	auto* w = new QWidget(parent);
	auto* form = new QFormLayout(w);
	form->setLabelAlignment(Qt::AlignRight);

	auto* titleEdit = new QLineEdit(title(), w);
	connect(titleEdit, &QLineEdit::textEdited, this, &DocumentWidget::setTitle);

	auto* glyphEdit = new QLineEdit(glyph(), w);
	glyphEdit->setMaxLength(4);
	connect(glyphEdit, &QLineEdit::textEdited, this, &DocumentWidget::setGlyph);

	auto* modified = new QCheckBox(w);
	modified->setChecked(isModified());
	modified->setEnabled(false);
	connect(this, &DocumentWidget::modifiedChanged, modified, &QCheckBox::setChecked);

	form->addRow(tr("Заголовок:"), titleEdit);
	form->addRow(tr("Значок:"), glyphEdit);
	form->addRow(tr("Изменён:"), modified);
	return w;
}

bool DocumentWidget::maybeClose() {
	if (!isModified())
		return true;

	const auto r = QMessageBox::question(
		this, tr("Несохранённые изменения"),
		tr("Сохранить изменения в «%1»?").arg(title()),
		QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
		QMessageBox::Save);

	if (r == QMessageBox::Cancel)
		return false;

	if (r == QMessageBox::Save)
		return save();

	return true;
}

void DocumentWidget::closeEvent(QCloseEvent* e) {
	if (maybeClose())
		e->accept();
	else
		e->ignore();
}
