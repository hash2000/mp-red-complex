#include "Libs/Engine/ui/doc_widget.h"
#include "Libs/Engine/ui/main_window.h"
#include "Libs/BaseWidgets/properties/tab_color_editor.h"
#include "Libs/Engine/scripting/command_context.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>

namespace {
/// Общие переменные контекста, в которых документ хранит своё состояние.
const QString kTitleVariable = QStringLiteral("title");
const QString kGlyphVariable = QStringLiteral("glyph");
const QString kModifiedVariable = QStringLiteral("modified");
const QString kColorVariable = QStringLiteral("color");
const QString kXtypeVariable = QStringLiteral("xtype");
const QString kClassNameVariable = QStringLiteral("className");

/// Глиф вкладки, если ни документ, ни тема его не задали.
const QString kDefaultGlyph = QStringLiteral("📄");
} // namespace

struct DocumentWidget::Private {
	QString title;
	/// Явный глиф документа; пустой — глиф берётся из темы.
	QString glyph;
	/// Стандартный глиф из темы (привязка к классу/xtype).
	QString themeGlyph;
	bool modified = false;
	QColor color;
	QString xtype;
	QString className;
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

const QString& DocumentWidget::defaultGlyph() {
	return kDefaultGlyph;
}

QString DocumentWidget::glyph() const {
	if (!d->glyph.isEmpty()) {
		return d->glyph;
	}

	return d->themeGlyph.isEmpty() ? kDefaultGlyph : d->themeGlyph;
}

void DocumentWidget::setGlyph(const QString& glyph) {
	if (d->glyph == glyph)
		return;
	d->glyph = glyph;
	if (d->context) {
		// Пустой глиф означает «не задан»: переменная удаляется, и вкладка
		// возвращается к глифу из темы.
		if (glyph.isEmpty()) {
			d->context->removeVariable(kGlyphVariable);
		}
		else {
			d->context->setVariable(kGlyphVariable, glyph);
		}
	}
	emit glyphChanged(this->glyph());
}

QString DocumentWidget::themeGlyph() const {
	return d->themeGlyph;
}

void DocumentWidget::setThemeGlyph(const QString& glyph) {
	if (d->themeGlyph == glyph) {
		return;
	}

	d->themeGlyph = glyph;
	// Действующий глиф мог не измениться (явный глиф документа важнее),
	// но подписчикам важно знать о смене оформления.
	emit glyphChanged(this->glyph());
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

QString DocumentWidget::className() const {
	return d->className;
}

void DocumentWidget::setClassName(const QString& className) {
	if (d->className == className) {
		return;
	}

	d->className = className;
	if (d->context) {
		d->context->setVariable(kClassNameVariable, className);
	}
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
				emit glyphChanged(glyph());
			}
		}
		else if (!d->glyph.isEmpty()) {
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

		if (own.contains(kClassNameVariable)) {
			d->className = own.value(kClassNameVariable).toString();
		}
		else if (!d->className.isEmpty()) {
			context->setVariable(kClassNameVariable, d->className);
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
						emit glyphChanged(glyph());
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
				else if (name == kClassNameVariable) {
					d->className = newValue.toString();
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
	return w;
}

QList<DocumentPropertyGroup> DocumentWidget::createPropertyGroups(QWidget* parent) {
	Q_UNUSED(parent);
	return { };
}

QWidget* DocumentWidget::createTabPropertiesEditor(QWidget* parent) {
	auto* w = new QWidget(parent);
	auto* form = new QFormLayout(w);
	form->setLabelAlignment(Qt::AlignRight);

	auto* titleEdit = new QLineEdit(title(), w);
	connect(titleEdit, &QLineEdit::textEdited, this, &DocumentWidget::setTitle);

	auto* glyphEdit = new QLineEdit(glyph(), w);
	glyphEdit->setMaxLength(4);
	connect(glyphEdit, &QLineEdit::textEdited, this, &DocumentWidget::setGlyph);

	// Цвет вкладки: образец цвета, выбор и сброс к цвету темы.
	auto* colorEditor = new TabColorEditor(w);
	colorEditor->setObjectName(QStringLiteral("DocumentColorEditor"));
	colorEditor->setColor(color());
	colorEditor->setDialogTitle(tr("Цвет вкладки"));
	connect(colorEditor, &TabColorEditor::colorChanged, this, &DocumentWidget::setColor);
	connect(this, &DocumentWidget::colorChanged, colorEditor, &TabColorEditor::setColor);

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
	form->addRow(tr("Цвет вкладки:"), colorEditor);
	form->addRow(tr("Тип:"), xtypeLabel);
	form->addRow(tr("Изменён:"), modified);
	return w;
}

bool DocumentWidget::showProperties(const QString& groupId) {
	// Документы создаются как дочерние виджеты окна, поэтому оболочка
	// находится по цепочке родителей. Без окна показывать панель некому.
	for (QWidget* parent = parentWidget(); parent; parent = parent->parentWidget()) {
		if (auto* window = qobject_cast<MainWindow*>(parent)) {
			return window->showDocumentProperties(this, groupId);
		}
	}

	return false;
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
