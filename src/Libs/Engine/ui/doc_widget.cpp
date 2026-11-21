#include "Libs/Engine/ui/doc_widget.h"
#include "Libs/Engine/scripting/command_context.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QColorDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPixmap>
#include <QToolButton>

namespace {
/// Общие переменные контекста, в которых документ хранит своё состояние.
const QString kTitleVariable = QStringLiteral("title");
const QString kGlyphVariable = QStringLiteral("glyph");
const QString kModifiedVariable = QStringLiteral("modified");
const QString kColorVariable = QStringLiteral("color");
const QString kXtypeVariable = QStringLiteral("xtype");

/// Кнопка выбора цвета вкладки: показывает текущий цвет и открывает
/// стандартный диалог выбора цвета.
class TabColorButton final : public QToolButton {
public:
	explicit TabColorButton(const QColor& color, QWidget* parent)
		: QToolButton(parent) {
		setObjectName(QStringLiteral("DocumentColorButton"));
		setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		setColor(color);
	}

	void setColor(const QColor& color) {
		_color = color;
		setText(color.isValid() ? color.name() : tr("не задан"));
		QPixmap swatch(14, 14);
		swatch.fill(color.isValid() ? color : Qt::transparent);
		setIcon(QIcon(swatch));
	}

	QColor color() const {
		return _color;
	}

private:
	QColor _color;
};
} // namespace

struct DocumentWidget::Private {
	QString title;
	QString glyph = QStringLiteral("📄");
	bool modified = false;
	QColor color;
	QString xtype;
	std::shared_ptr<CommandContext> context;
	QMetaObject::Connection variableConnection;
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
	if (d->context) {
		d->context->setVariable(kTitleVariable, title);
	}
	emit titleChanged(title);
}

QString DocumentWidget::glyph() const {
	return d->glyph;
}

void DocumentWidget::setGlyph(const QString& glyph) {
	if (d->glyph == glyph)
		return;
	d->glyph = glyph;
	if (d->context) {
		d->context->setVariable(kGlyphVariable, glyph);
	}
	emit glyphChanged(glyph);
}

bool DocumentWidget::isModified() const {
	return d->modified;
}

void DocumentWidget::setModified(bool modified) {
	if (d->modified == modified)
		return;
	d->modified = modified;
	if (d->context) {
		d->context->setVariable(kModifiedVariable, modified);
	}
	emit modifiedChanged(modified);
}

QColor DocumentWidget::color() const {
	return d->color;
}

void DocumentWidget::setColor(const QColor& color) {
	if (d->color == color) {
		return;
	}

	d->color = color;
	if (d->context) {
		// Невалидный цвет означает «цвет не задан»: переменная удаляется,
		// чтобы вкладка вернулась к цвету темы.
		if (color.isValid()) {
			d->context->setVariable(kColorVariable, color);
		}
		else {
			d->context->removeVariable(kColorVariable);
		}
	}

	emit colorChanged(color);
}

QString DocumentWidget::xtype() const {
	return d->xtype;
}

void DocumentWidget::setXtype(const QString& xtype) {
	if (d->xtype == xtype) {
		return;
	}

	d->xtype = xtype;
	if (d->context) {
		d->context->setVariable(kXtypeVariable, xtype);
	}

	emit xtypeChanged(xtype);
}

std::shared_ptr<CommandContext> DocumentWidget::context() const {
	return d->context;
}

void DocumentWidget::setContext(const std::shared_ptr<CommandContext>& context) {
	if (d->context == context) {
		return;
	}

	if (d->context) {
		QObject::disconnect(d->variableConnection);
	}

	d->context = context;
	if (context) {
		// Собственные переменные контекста — источник состояния документа.
		// Если их ещё нет, в контекст записываются текущие значения.
		const auto own = context->ownVariables();

		if (own.contains(kTitleVariable)) {
			const auto value = own.value(kTitleVariable).toString();
			if (d->title != value) {
				d->title = value;
				emit titleChanged(value);
			}
		}
		else {
			context->setVariable(kTitleVariable, d->title);
		}

		if (own.contains(kGlyphVariable)) {
			const auto value = own.value(kGlyphVariable).toString();
			if (d->glyph != value) {
				d->glyph = value;
				emit glyphChanged(value);
			}
		}
		else {
			context->setVariable(kGlyphVariable, d->glyph);
		}

		if (own.contains(kModifiedVariable)) {
			const auto value = own.value(kModifiedVariable).toBool();
			if (d->modified != value) {
				d->modified = value;
				emit modifiedChanged(value);
			}
		}
		else {
			context->setVariable(kModifiedVariable, d->modified);
		}

		if (own.contains(kColorVariable)) {
			const auto value = own.value(kColorVariable).value<QColor>();
			if (d->color != value) {
				d->color = value;
				emit colorChanged(value);
			}
		}
		else if (d->color.isValid()) {
			context->setVariable(kColorVariable, d->color);
		}

		if (own.contains(kXtypeVariable)) {
			const auto value = own.value(kXtypeVariable).toString();
			if (d->xtype != value) {
				d->xtype = value;
				emit xtypeChanged(value);
			}
		}
		else if (!d->xtype.isEmpty()) {
			context->setVariable(kXtypeVariable, d->xtype);
		}

		// Изменения переменных контекста извне подхватываются документом.
		d->variableConnection = QObject::connect(context.get(), &CommandContext::variableChanged,
			this,
			[this](const QString& contextName, const QString& name, const QVariant& oldValue,
				const QVariant& newValue) {
				Q_UNUSED(oldValue);
				if (!d->context || contextName != d->context->nameSpace()) {
					return;
				}

				if (name == kTitleVariable) {
					const auto value = newValue.toString();
					if (d->title != value) {
						d->title = value;
						emit titleChanged(value);
					}
				}
				else if (name == kGlyphVariable) {
					const auto value = newValue.toString();
					if (d->glyph != value) {
						d->glyph = value;
						emit glyphChanged(value);
					}
				}
				else if (name == kModifiedVariable) {
					const auto value = newValue.toBool();
					if (d->modified != value) {
						d->modified = value;
						emit modifiedChanged(value);
					}
				}
				else if (name == kColorVariable) {
					const auto value = newValue.value<QColor>();
					if (d->color != value) {
						d->color = value;
						emit colorChanged(value);
					}
				}
				else if (name == kXtypeVariable) {
					const auto value = newValue.toString();
					if (d->xtype != value) {
						d->xtype = value;
						emit xtypeChanged(value);
					}
				}

				onContextVariableChanged(name, newValue);
			});
	}

	onContextAttached();
}

QVariant DocumentWidget::variable(const QString& name, const QVariant& defaultValue) {
	if (!d->context || name.isEmpty()) {
		return defaultValue;
	}

	if (d->context->hasVariable(name)) {
		return d->context->variable(name);
	}

	// Значения нет ни в собственном, ни в базовом контексте:
	// создаём переменную со значением по умолчанию.
	d->context->setVariable(name, defaultValue);
	return defaultValue;
}

bool DocumentWidget::setVariable(const QString& name, const QVariant& value) {
	if (!d->context || name.isEmpty()) {
		return false;
	}

	return d->context->setVariable(name, value);
}

bool DocumentWidget::hasVariable(const QString& name) const {
	return d->context && !name.isEmpty() && d->context->hasVariable(name);
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

	// Цвет вкладки: кнопка с текущим цветом и сброс к цвету темы.
	auto* colorRow = new QWidget(w);
	auto* colorLayout = new QHBoxLayout(colorRow);
	colorLayout->setContentsMargins(0, 0, 0, 0);
	colorLayout->setSpacing(4);

	auto* colorButton = new TabColorButton(color(), colorRow);
	auto* colorReset = new QToolButton(colorRow);
	colorReset->setObjectName(QStringLiteral("DocumentColorResetButton"));
	colorReset->setText(QStringLiteral("✕"));
	colorReset->setToolTip(tr("Сбросить цвет вкладки"));
	colorReset->setEnabled(color().isValid());

	connect(colorButton, &QToolButton::clicked, this, [this, colorButton] {
		const auto initial = colorButton->color().isValid()
			? colorButton->color() : palette().color(QPalette::Highlight);
		const auto picked = QColorDialog::getColor(initial, this, tr("Цвет вкладки"));
		if (picked.isValid()) {
			setColor(picked);
		}
	});
	connect(colorReset, &QToolButton::clicked, this, [this] {
		setColor(QColor());
	});
	connect(this, &DocumentWidget::colorChanged, colorButton, &TabColorButton::setColor);
	connect(this, &DocumentWidget::colorChanged, colorReset, [colorReset](const QColor& value) {
		colorReset->setEnabled(value.isValid());
	});

	colorLayout->addWidget(colorButton, 1);
	colorLayout->addWidget(colorReset);

	auto* modified = new QCheckBox(w);
	modified->setChecked(isModified());
	modified->setEnabled(false);
	connect(this, &DocumentWidget::modifiedChanged, modified, &QCheckBox::setChecked);

	auto* xtypeLabel = new QLabel(xtype().isEmpty() ? QStringLiteral("—") : xtype(), w);
	xtypeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	connect(this, &DocumentWidget::xtypeChanged, xtypeLabel, [xtypeLabel](const QString& value) {
		xtypeLabel->setText(value.isEmpty() ? QStringLiteral("—") : value);
	});

	form->addRow(tr("Заголовок:"), titleEdit);
	form->addRow(tr("Значок:"), glyphEdit);
	form->addRow(tr("Цвет вкладки:"), colorRow);
	form->addRow(tr("Тип:"), xtypeLabel);
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
