#include "Libs/Engine/ui/doc_tab_panel.h"
#include "Libs/Engine/ui/glyph_tool_button.h"
#include "Libs/Engine/ui/doc_widget.h"

#include <QEnterEvent>
#include <QHBoxLayout>
#include <QHash>
#include <QLabel>
#include <QMouseEvent>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QStyle>
#include <functional>

namespace {

/// Одна «вкладка» слева: [глиф] [заголовок........] [⚙] [✕]
/// Внутренняя деталь реализации, наружу не видна.
class DocumentTabItem final : public QFrame {
public:
	DocumentTabItem(DocumentWidget* doc, QWidget* parent)
		: QFrame(parent), _doc(doc) {
		setObjectName(QStringLiteral("DocumentTabItem"));
		setAttribute(Qt::WA_Hover);
		setMinimumHeight(28);

		auto* lay = new QHBoxLayout(this);
		lay->setContentsMargins(8, 2, 4, 2);
		lay->setSpacing(6);

		_glyph = new QLabel(this);
		_title = new QLabel(this);
		_title->setObjectName(QStringLiteral("DocTabTitle"));
		_title->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

		_props = new GlyphToolButton(QStringLiteral("⚙"),
			DocumentTabsPanel::tr("Свойства"), this, "DocTabButton");
		_close = new GlyphToolButton(QStringLiteral("✕"),
			DocumentTabsPanel::tr("Закрыть (средняя кнопка мыши)"), this, "DocTabButton");

		for (QToolButton* b : { _props, _close }) {
			QSizePolicy sp = b->sizePolicy();
			sp.setRetainSizeWhenHidden(true);   // без «прыжков» разметки
			b->setSizePolicy(sp);
		}

		lay->addWidget(_glyph);
		lay->addWidget(_title, 1);
		lay->addWidget(_props);
		lay->addWidget(_close);

		connect(_props, &QToolButton::clicked, this, [this] { if (onProperties) onProperties(); });
		connect(_close, &QToolButton::clicked, this, [this] { if (onClose) onClose(); });

		updateFromDocument();
		updateButtons();
	}

	std::function<void()> onActivate;
	std::function<void()> onClose;
	std::function<void()> onProperties;

	void setActive(bool active) {
		_active = active;
		setProperty("active", active);
		style()->unpolish(this);
		style()->polish(this);
		update();
		updateButtons();
	}

	void updateFromDocument() {
		_glyph->setText(_doc->glyph());
		_fullTitle = _doc->title() + (_doc->isModified() ? QStringLiteral(" ●") : QString());
		setToolTip(_doc->toolTip().isEmpty() ? _doc->title() : _doc->toolTip());
		_title->setProperty("modified", _doc->isModified());
		style()->unpolish(this);
		style()->polish(this);
		update();
		updateElided();
	}

protected:
	void mousePressEvent(QMouseEvent* e) override {
		if (e->button() == Qt::LeftButton && onActivate)   onActivate();
		else if (e->button() == Qt::MiddleButton && onClose) onClose();
		e->accept();
	}
	void enterEvent(QEnterEvent* e) override {
		_hover = true;  updateButtons(); QFrame::enterEvent(e);
	}
	void leaveEvent(QEvent* e)      override {
		_hover = false; updateButtons(); QFrame::leaveEvent(e);
	}
	void resizeEvent(QResizeEvent* e) override {
		QFrame::resizeEvent(e); updateElided();
	}

private:
	void updateButtons() {
		const bool show = _active || _hover;   // как в VS: кнопки у активной/под мышью
		_props->setVisible(show);
		_close->setVisible(show);
	}
	void updateElided() {
		_title->setText(_title->fontMetrics().elidedText(
			_fullTitle, Qt::ElideRight, qMax(0, _title->width())));
	}

	DocumentWidget* _doc;
	QLabel* _glyph;
	QLabel* _title;
	QToolButton* _props;
	QToolButton* _close;
	QString _fullTitle;
	bool _active = false;
	bool _hover = false;
};

} // namespace

struct DocumentTabsPanel::Private {
	QScrollArea* scroll = nullptr;
	QWidget* list = nullptr;
	QVBoxLayout* listLayout = nullptr;
	QLabel* countLabel = nullptr;
	QHash<DocumentWidget*, DocumentTabItem*> items;
	DocumentWidget* active = nullptr;

	void updateCount() {
		countLabel->setText(items.isEmpty() ? QString() : QString::number(items.size()));
	}
};

DocumentTabsPanel::DocumentTabsPanel(QWidget* parent)
	: QWidget(parent), d(std::make_unique<Private>()) {
	setObjectName(QStringLiteral("DocumentTabsPanel"));
	setAttribute(Qt::WA_StyledBackground);
	setMinimumWidth(160);

	auto* root = new QVBoxLayout(this);
	root->setContentsMargins(0, 0, 0, 0);
	root->setSpacing(0);

	// Заголовок
	auto* header = new QWidget(this);
	header->setObjectName(QStringLiteral("PanelHeader"));
	header->setAttribute(Qt::WA_StyledBackground);
	auto* hl = new QHBoxLayout(header);
	hl->setContentsMargins(10, 4, 4, 4);
	auto* title = new QLabel(tr("ДОКУМЕНТЫ"), header);
	title->setObjectName(QStringLiteral("PanelTitle"));
	d->countLabel = new QLabel(header);
	d->countLabel->setObjectName(QStringLiteral("PanelCounter"));
	auto* newBtn = new GlyphToolButton(QStringLiteral("＋"), tr("Новый документ (Ctrl+N)"), header);
	connect(newBtn, &QToolButton::clicked, this, &DocumentTabsPanel::newDocumentRequested);
	hl->addWidget(title);
	hl->addWidget(d->countLabel);
	hl->addStretch();
	hl->addWidget(newBtn);

	// Список вкладок
	d->scroll = new QScrollArea(this);
	d->scroll->setWidgetResizable(true);
	d->scroll->setFrameShape(QFrame::NoFrame);
	d->scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	d->list = new QWidget;
	d->list->setObjectName(QStringLiteral("DocumentTabsList"));
	d->list->setAttribute(Qt::WA_StyledBackground);
	d->listLayout = new QVBoxLayout(d->list);
	d->listLayout->setContentsMargins(4, 4, 4, 4);
	d->listLayout->setSpacing(2);
	d->listLayout->addStretch(1);
	d->scroll->setWidget(d->list);

	root->addWidget(header);
	root->addWidget(d->scroll, 1);

	d->updateCount();
}

DocumentTabsPanel::~DocumentTabsPanel() = default;

void DocumentTabsPanel::addDocument(DocumentWidget* doc) {
	if (!doc || d->items.contains(doc))
		return;

	auto* item = new DocumentTabItem(doc, d->list);
	item->onActivate = [this, doc] { emit activateRequested(doc); };
	item->onClose = [this, doc] { emit closeRequested(doc); };
	item->onProperties = [this, doc] { emit propertiesRequested(doc); };

	auto refresh = [item] { item->updateFromDocument(); };
	connect(doc, &DocumentWidget::titleChanged, item, refresh);
	connect(doc, &DocumentWidget::glyphChanged, item, refresh);
	connect(doc, &DocumentWidget::modifiedChanged, item, refresh);

	d->listLayout->insertWidget(d->listLayout->count() - 1, item);  // перед stretch
	d->items.insert(doc, item);
	d->updateCount();
}

void DocumentTabsPanel::removeDocument(DocumentWidget* doc) {
	if (auto* item = d->items.take(doc)) {
		item->hide();
		item->deleteLater();   // мы можем находиться внутри клика по кнопке этого item
	}
	if (d->active == doc) d->active = nullptr;
	d->updateCount();
}

void DocumentTabsPanel::setActiveDocument(DocumentWidget* doc) {
	if (auto* prev = d->items.value(d->active)) prev->setActive(false);
	d->active = doc;
	if (auto* cur = d->items.value(doc)) {
		cur->setActive(true);
		d->scroll->ensureWidgetVisible(cur);
	}
}
