#include "Libs/Engine/ui/properties_panel.h"
#include "Libs/Engine/ui/glyph_tool_button.h"
#include "Libs/Engine/ui/doc_widget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QScrollArea>
#include <QVBoxLayout>

struct PropertiesPanel::Private {
	QPointer<DocumentWidget> doc;
	QLabel* header = nullptr;
	QToolButton* refreshBtn = nullptr;
	QScrollArea* scroll = nullptr;
	QLabel* empty = nullptr;

	void rebuild() {
		if (QWidget* old = scroll->takeWidget())
			old->deleteLater();   // deleteLater: могли прийти из сигнала самого редактора

		QWidget* editor = doc ? doc->createPropertiesEditor(scroll) : nullptr;
		updateHeader();
		refreshBtn->setEnabled(doc != nullptr);

		if (!editor) {
			empty->setText(doc ? PropertiesPanel::tr("У документа нет свойств")
				: PropertiesPanel::tr("Нет выбранного документа"));
			scroll->hide();
			empty->show();
			return;
		}

		scroll->setWidget(editor);
		empty->hide();
		scroll->show();
	}

	void updateHeader() {
		header->setText(doc ? doc->glyph() + QLatin1Char(' ') + doc->title() : QString());
	}
};

PropertiesPanel::PropertiesPanel(QWidget* parent)
	: QWidget(parent), d(std::make_unique<Private>()) {
	auto* root = new QVBoxLayout(this);
	root->setContentsMargins(8, 8, 8, 8);
	root->setSpacing(8);

	auto* top = new QHBoxLayout;
	d->header = new QLabel(this);
	d->header->setStyleSheet(QStringLiteral("font-weight:600;"));
	d->refreshBtn = new GlyphToolButton(QStringLiteral("⟳"), tr("Обновить"), this);
	top->addWidget(d->header, 1);
	top->addWidget(d->refreshBtn);

	d->scroll = new QScrollArea(this);
	d->scroll->setWidgetResizable(true);

	d->empty = new QLabel(this);
	d->empty->setObjectName(QStringLiteral("EmptyHint"));
	d->empty->setAlignment(Qt::AlignCenter);
	d->empty->setWordWrap(true);

	root->addLayout(top);
	root->addWidget(d->scroll, 1);
	root->addWidget(d->empty, 1);

	connect(d->refreshBtn, &QToolButton::clicked, this, [this] {
		if (d->doc) {
			d->doc->refresh(); d->rebuild();
		}
	});

	d->rebuild();
}

PropertiesPanel::~PropertiesPanel() = default;

void PropertiesPanel::setDocument(DocumentWidget* doc) {
	if (d->doc == doc) return;
	if (d->doc) disconnect(d->doc, nullptr, this, nullptr);

	d->doc = doc;
	if (doc) {
		connect(doc, &DocumentWidget::titleChanged, this, [this] { d->updateHeader(); });
		connect(doc, &DocumentWidget::glyphChanged, this, [this] { d->updateHeader(); });
		connect(doc, &DocumentWidget::propertiesChanged, this, [this] { d->rebuild(); });
	}
	d->rebuild();
}

DocumentWidget* PropertiesPanel::document() const {
	return d->doc;
}
